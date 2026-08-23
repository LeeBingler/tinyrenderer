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

extern mat<4, 4> ModelView, Perspective; // "OpenGL" state matrices and
extern std::vector<double> zbuffer;      // the depth buffer

struct RandomShader : IShader {
  const Model &model;
  TGAColor color = {};
  vec3 tri[3]; // triangle in eye coordinates

  RandomShader(const Model &m) : model(m) {}

  virtual vec4 vertex(const int face, const int vert) {
    vec4 v = model.vert(face, vert); // current vertex in object coordinates
    vec4 gl_Position = ModelView * vec4{v.x, v.y, v.z, 1.};
    tri[vert] = gl_Position.xyz();    // in eye coordinates
    return Perspective * gl_Position; // in clip coordinates
  }

  virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
    return {false, color}; // do not discard the pixel
  }
};

struct PhongShader : IShader {
  const Model &model;
  vec4 l;
  vec3 varying_nrm[3];
  vec2 varying_uv[3];

  PhongShader(const vec3 light, const Model &m) : model(m) {
    l = normalized((ModelView * vec4{light.x, light.y, light.z, 0.}));
  }

  virtual vec4 vertex(const int face, const int vert) {
    /*
    vec3 v = model.vert(face, vert); // current vertex in object coordinates
    vec3 n = model.normal(face, vert);
    tri[vert] = gl_Position.xyz();    // in eye coordinates
    */

    varying_uv[vert] = model.uv(face, vert);
    vec4 gl_Position = ModelView * model.vert(face, vert);
    return Perspective * gl_Position; // in clip coordinates
  }

  virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
    vec2 uv = varying_uv[0] * bar[0] + varying_uv[1] * bar[1] +
              varying_uv[2] * bar[2];
    vec4 n = normalized(ModelView.invert_transpose() * model.normal(uv));
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

  constexpr vec3 light{1, 1, 1};  // light source
  constexpr vec3 eye{-1, 0, 2};   // camera position
  constexpr vec3 center{0, 0, 0}; // camera direction
  constexpr vec3 up{0, 1, 0};     // camera up vector

  lookat(eye, center, up);              // build the ModelView   matrix
  init_perspective(norm(eye - center)); // build the Perspective matrix
  init_viewport(width / 16, height / 16, width * 7 / 8,
                height * 7 / 8); // build the Viewport    matrix
  init_zbuffer(width, height);
  TGAImage framebuffer(width, height, TGAImage::RGB);

  Model m(argv[1]);
  m.load();

  for (int f = 0; f < m.nfaces(); f++) {
    PhongShader shader(light, m);
    // assemble the primitive
    Triangle clip = {shader.vertex(f, 0), shader.vertex(f, 1),
                     shader.vertex(f, 2)};
    rasterize(clip, shader, framebuffer); // rasterize the primitive
  }

  framebuffer.write_tga_file("framebuffer.tga");
  return 0;
}
