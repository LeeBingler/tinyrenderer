#include "geometry.h"
#include "model.h"
#include "our_gl.hpp"
#include "tgaimage.h"
#include <algorithm>
#include <cstdlib>

constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blue = {255, 128, 64, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};

extern mat<4, 4> ModelView, Perspective,
    Viewport;                       // "OpenGL" state matrices and
extern std::vector<double> zbuffer; // the depth buffer

struct RandomShader : IShader {
  const Model &model;
  vec3 tri[3]; // triangle in eye coordinates

  RandomShader(const vec3 light, const Model &m) : model(m) {}

  virtual vec4 vertex(const int face, const int vert) {
    vec4 v = model.vert(face, vert); // current vertex in object coordinates
    vec4 gl_Position = ModelView * vec4{v.x, v.y, v.z, 1.};
    tri[vert] = gl_Position.xyz();    // in eye coordinates
    return Perspective * gl_Position; // in clip coordinates
  }

  virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
    return {false, white}; // do not discard the pixel
  }
};

struct BlankShader : IShader {
  const Model &model;

  BlankShader(const Model &m) : model(m) {}

  virtual vec4 vertex(const int face, const int vert) {
    vec4 gl_Position = ModelView * model.vert(face, vert);
    return Perspective * gl_Position; // in clip coordinates
  }

  virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
    return {false, white}; // do not discard the pixel
  }
};

struct PhongShader : IShader {
  const Model &model;
  vec4 l;
  vec2 varying_uv[3];
  vec4 varying_nrm[3];
  vec4 tri[3]; // triangle in view coordinates

  PhongShader(const vec3 light, const Model &m) : model(m) {
    l = normalized((ModelView * vec4{light.x, light.y, light.z, 0.}));
  }

  virtual vec4 vertex(const int face, const int vert) {
    varying_uv[vert] = model.uv(face, vert);
    varying_nrm[vert] = ModelView.invert_transpose() * model.normal(face, vert);
    vec4 gl_Position = ModelView * model.vert(face, vert);
    tri[vert] = gl_Position;
    return Perspective * gl_Position; // in clip coordinates
  }

  virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
    vec2 uv = varying_uv[0] * bar[0] + varying_uv[1] * bar[1] +
              varying_uv[2] * bar[2];

    mat<2, 4> E = {tri[1] - tri[0], tri[2] - tri[0]};
    mat<2, 2> U = {varying_uv[1] - varying_uv[0],
                   varying_uv[2] - varying_uv[0]};
    mat<2, 4> T = U.invert() * E;
    mat<4, 4> D = {normalized(T[0]),
                   normalized(T[1]),
                   normalized(varying_nrm[0] * bar[0] +
                              varying_nrm[1] * bar[1] +
                              varying_nrm[2] * bar[2]),
                   {0, 0, 0, 1}};
    vec4 n = normalized(D.transpose() * model.normal(uv));
    vec4 r = normalized(n * (n * l) * 2 - l);
    TGAColor color = sample2D(model.diffuse(), uv);

    double ambient = 0.4;
    double diffuse = std::max(0., n * l);
    double specular = (3. * sample2D(model.specular(), uv)[0] / 255.) *
                      std::pow(std::max(r.z, 0.), 35);

    for (int channel : {0, 1, 2})
      color[channel] =
          std::min<int>(255, color[channel] * (ambient + diffuse + specular));
    return {false, color}; // do not discard the pixel
  }
};

int main(int argc, char **argv) {
  constexpr int width = 800;
  constexpr int height = 800;
  constexpr int shadoww = 1600;
  constexpr int shadowh = 1600;

  constexpr vec3 light{1, 1, 1};  // light source
  constexpr vec3 eye{-1, 0, 2};   // camera position
  constexpr vec3 center{0, 0, 0}; // camera direction
  constexpr vec3 up{0, 1, 0};     // camera up vector

  lookat(eye, center, up);              // build the ModelView matrix
  init_perspective(norm(eye - center)); // build the Perspective matrix
  init_viewport(width / 16, height / 16, width * 7 / 8,
                height * 7 / 8); // build the Viewport    matrix
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
      rasterize(clip, shader, framebuffer); // rasterize the primitive
    }
  }

  // Shadow mapping
  mat<4, 4> M = (Viewport * Perspective * ModelView).invert();
  std::vector<double> zbuffer_copy = zbuffer;
  lookat(light, center, up);            // build the ModelView matrix
  init_perspective(norm(eye - center)); // build the Perspective matrix
  init_viewport(shadoww / 16, shadowh / 16, shadoww * 7 / 8,
                shadowh * 7 / 8); // build the Viewport    matrix
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

  trash.write_tga_file("light_pov.tga");
  mat<4, 4> N = Viewport * Perspective * ModelView;
  std::vector<bool> isLit(width * height, false);

  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      vec<4> frag = M * vec<4>{x, y, zbuffer_copy[x + y * width], 1};
      vec<4> a = N * frag;
      vec<3> w = {a.x / a.w, a.y / a.w, a.z / a.w};
      bool lit = frag.z < -100 || w.x < 0 || w.x > shadoww || w.y < 0 ||
                 w.y > shadowh ||
                 w.z > zbuffer[int(w.x) + int(w.y) * shadoww] - .04;

      isLit[x + y * width] = lit;
    }
  }

  // Test mask
  TGAImage maskimg(width, height, TGAImage::GRAYSCALE);
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      if (isLit[x + y * width])
        continue;
      maskimg.set(x, y, {255, 255, 255, 255});
    }
  }
  maskimg.write_tga_file("mask.tga");

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
