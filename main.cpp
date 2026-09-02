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

mat<4, 4> make_zbuffer_matrix(int argc, char **argv, int shadoww, int shadowh,
                              vec3 position) {
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

  return Viewport * Perspective * ModelView; // Make the N matrix from lesson
}

std::vector<double> make_shadowmap(int shadoww, int shadowh, mat<4, 4> N,
                                   int argc, char **argv, mat<4, 4> M,
                                   std::vector<double> original_zbuffer) {
  std::vector<double> isLit(width * height, false);

#pragma omp parallel for
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      vec<4> frag = M * vec<4>{x, y, original_zbuffer[x + y * width], 1};
      vec<4> a = N * frag;
      vec<3> w = {a.x / a.w, a.y / a.w, a.z / a.w};
      double lit = frag.z < -100 || w.x < 0 || w.x > shadoww || w.y < 0 ||
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

  // Shadow mapping / AO
  mat<4, 4> M = (Viewport * Perspective * ModelView).invert();
  std::vector<double> original_zbuffer = zbuffer;
  std::vector<double> isLit(width * height, 0.0);
  constexpr int n_pass = 30;

  auto smoothstep = [](double edge0, double edge1,
                       double x) { // smoothstep returns 0 if the input is less
                                   // than the left edge,
    double t = std::clamp((x - edge0) / (edge1 - edge0), 0.,
                          1.); // 1 if the input is greater than the right edge,
    return t * t *
           (3 - 2 * t); // Hermite interpolation inbetween. The derivative of
                        // the smoothstep function is zero at both edges.
  };

  for (int i = 0; i < n_pass; i++) {
    double y = ((double)rand() / (RAND_MAX));
    double theta = 2.0 * M_PI * ((double)rand() / (RAND_MAX));
    double r = std::sqrt(1.0 - y * y);
    vec3 position = vec3{r * std::cos(theta), y, r * std::sin(theta)} * 1.5;

    mat<4, 4> N = make_zbuffer_matrix(argc, argv, shadoww, shadowh, position);

#pragma omp parallel for
    for (int x = 0; x < width; x++) {
      for (int y = 0; y < height; y++) {
        vec<4> frag = M * vec<4>{x, y, original_zbuffer[x + y * width], 1};
        vec<4> a = N * frag;
        vec<3> w = {a.x / a.w, a.y / a.w, a.z / a.w};

        double lit =
            (frag.z < -100 || // it's the background or
             (w.x >= 0 && w.x < shadoww && w.y >= 0 &&
              w.y < shadowh && // it is not out of bounds of the shadow buffer
              (w.z >
               zbuffer[int(w.x) + int(w.y) * shadoww] - .03))); // it is visible

        isLit[x + y * width] += (lit - isLit[x + y * width]) / (i + 1.);
      }
    }
  }

  // Test mask
#ifdef test
#include "test.hpp"
  test_shadow_mask(width, height, trash, isLit);
#endif

  // apply the shade
#pragma omp parallel for
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      double m = smoothstep(-1, 1, isLit[x + y * width]);
      TGAColor c = framebuffer.get(x, y);
      framebuffer.set(x, y, {c[0] * m, c[1] * m, c[2] * m, c[3]});
    }
  }

  framebuffer.write_tga_file("framebuffer.tga");
  return 0;
}
