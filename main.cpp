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
    vec3 v = model.vert(face, vert); // current vertex in object coordinates
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
  vec3 tri[3]; // triangle in eye coordinates
  vec3 l;

  PhongShader(const vec3 light, const Model &m) : model(m) {
    l = normalized((ModelView * vec4{light.x, light.y, light.z, 0.}).xyz());
  }

  virtual vec4 vertex(const int face, const int vert) {
    vec3 v = model.vert(face, vert); // current vertex in object coordinates
    vec4 gl_Position = ModelView * vec4{v.x, v.y, v.z, 1.};
    tri[vert] = gl_Position.xyz();    // in eye coordinates
    return Perspective * gl_Position; // in clip coordinates
  }

  virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
    TGAColor color = {255, 255, 255, 255};
    vec3 n = normalized(cross(tri[1] - tri[0], tri[2] - tri[0]));
    vec3 r = normalized(n * (n * l) * 2 - l);

    double ambient = 0.1;
    double diffuse = std::max(0., n * l);
    double specular = std::pow(std::max(0., r.z), 35);

    for (int channel : {0, 1, 2})
      color[channel] *= std::min(1., ambient + .4 * diffuse + .9 * specular);
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
