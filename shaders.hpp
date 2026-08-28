#pragma once

#include "model.h"
#include "our_gl.hpp"

// "OpenGL" state matrices and
extern mat<4, 4> ModelView, Perspective, Viewport;
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
    return {false,
            {std::rand() % 255, std::rand() % 255, std::rand() % 255,
             1}}; // do not discard the pixel
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
    return {false, {255, 255, 255, 255}}; // do not discard the pixel
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
