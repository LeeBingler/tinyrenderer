#include "model.h"
#include "our_gl.hpp"

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

int main(int argc, char **argv) {
  constexpr int width = 800;
  constexpr int height = 800;

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
    RandomShader shader(m);
    shader.color = {std::rand() % 255, std::rand() % 255, std::rand() % 255,
                    255};
    // assemble the primitive
    Triangle clip = {shader.vertex(f, 0), shader.vertex(f, 1),
                     shader.vertex(f, 2)};
    rasterize(clip, shader, framebuffer); // rasterize the primitive
  }

  framebuffer.write_tga_file("framebuffer.tga");
  return 0;
}
