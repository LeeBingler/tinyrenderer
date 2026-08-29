#include "geometry.h"
#include "model.h"
#include "our_gl.hpp"
#include "shaders.hpp"
#include "tgaimage.h"
#include <vector>

// "OpenGL" state matrices and the depth buffer
extern mat<4, 4> ModelView, Perspective, Viewport;
extern std::vector<double> zbuffer;

constexpr int width = 800;  // width of image render
constexpr int height = 800; // height of image render

constexpr vec3 eye{-1, 0, 2};   // camera position
constexpr vec3 center{0, 0, 0}; // camera direction
constexpr vec3 up{0, 1, 0};     // camera up vector

std::vector<bool> make_shadowmap(int shadoww, int shadowh, vec3 position,
                                 int argc, char **argv, mat<4, 4> M,
                                 std::vector<double> original_zbuffer) {

  lookat(position, center, up);         // build the ModelView matrix
  init_perspective(norm(eye - center)); // build the Perspective matrix
  init_viewport(shadoww / 16, shadowh / 16, shadoww * 7 / 8, shadowh * 7 / 8);
  init_zbuffer(shadoww, shadowh);
  TGAImage trash(shadoww, shadowh, TGAImage::GRAYSCALE);

  for (int i = 1; i < argc; i++) {
    Model m(argv[i]);
    m.load();

    for (int f = 0; f < m.nfaces(); f++) {
      BlankShader shader(m);
      // assemble the primitive
      Triangle clip = {shader.vertex(f, 0), shader.vertex(f, 1),
                       shader.vertex(f, 2)};
      rasterize(clip, shader, trash); // rasterize the primitive
    }
  }

  mat<4, 4> N = Viewport * Perspective * ModelView;
  std::vector<bool> isLit(width * height, false);

  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      vec<4> frag = M * vec<4>{x, y, original_zbuffer[x + y * width], 1};
      vec<4> a = N * frag;
      vec<3> w = {a.x / a.w, a.y / a.w, a.z / a.w};
      bool lit = frag.z < -100 || w.x < 0 || w.x > shadoww || w.y < 0 ||
                 w.y > shadowh ||
                 w.z > zbuffer[int(w.x) + int(w.y) * shadoww] - .04;

      isLit[x + y * width] = lit;
    }
  }

  return isLit;
}

int main(int argc, char **argv) {
  constexpr int shadoww = 1600;
  constexpr int shadowh = 1600;

  constexpr vec3 light{1, 1, 1}; // light source

  // First pass
  lookat(eye, center, up);
  init_perspective(norm(eye - center));
  init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
  init_zbuffer(width, height);
  TGAImage framebuffer(width, height, TGAImage::RGB);

  for (int i = 1; i < argc; i++) {
    Model m(argv[i]);
    m.load();

    for (int f = 0; f < m.nfaces(); f++) {
      PhongShader shader(light, m);
      // assemble the primitive
      Triangle clip = {shader.vertex(f, 0), shader.vertex(f, 1),
                       shader.vertex(f, 2)};
      // rasterize the primitive
      rasterize(clip, shader, framebuffer);
    }
  }

  // Shadow mapping
  mat<4, 4> M = (Viewport * Perspective * ModelView).invert();
  std::vector<double> original_zbuffer = zbuffer;
  std::vector<bool> isLit =
      make_shadowmap(shadoww, shadowh, light, argc, argv, M, original_zbuffer);

  // Test mask
#ifdef test
#include "test.hpp"
  test_shadow_mask(width, height, trash, isLit);
#endif

  // apply the shade
  for (double x = 0; x < width; x++) {
    for (double y = 0; y < height; y++) {
      if (isLit[x + y * width])
        continue;

      TGAColor color = framebuffer.get(x, y);
      vec<3> a = {color[0], color[1], color[2]};
      if (norm(a) < 80)
        continue;
      a = normalized(a) * 80;
      framebuffer.set(x, y, {a[0], a[1], a[2], 255});
    }
  }

  framebuffer.write_tga_file("framebuffer.tga");
  return 0;
}
