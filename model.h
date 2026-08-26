#pragma once

#include "geometry.h"
#include "tgaimage.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

class Model {
public:
  std::string path;

  std::vector<vec4> vertices = {};
  std::vector<int> facet_vertices = {};

  std::vector<vec4> normals = {};
  std::vector<int> facet_normals = {};

  std::vector<vec2> uvs = {};
  std::vector<int> facet_uvs = {};

  TGAImage normalmap = {};
  TGAImage colormap = {};
  TGAImage specularmap = {};

  int nverts() const { return vertices.size(); }
  int nfaces() const { return facet_vertices.size() / 3; }

  ~Model() {};
  Model(std::string path) : path(path) {};

  int load() {
    std::string line;
    std::vector<std::string> words;
    std::ifstream file(this->path);

    if (file.fail())
      return -1;

    while (std::getline(file, line)) {
      if (line.empty())
        continue;
      words = this->split(line, ' ');

      if (words[0] == "v") {
        vec4 vertice = {std::stod(words[1]), std::stod(words[2]),
                        std::stod(words[3]), 1};

        vertices.push_back(vertice);

      } else if (words[0] == "vn") {
        vec4 norms = {std::stod(words[1]), std::stod(words[2]),
                      std::stod(words[3]), 1};

        normals.push_back(normalized(norms));

      } else if (words[0] == "vt") {
        vec2 uv = {std::stod(words[1]), 1 - std::stod(words[2])};

        uvs.push_back(uv);
      } else if (words[0] == "f") {
        for (unsigned int i = 1; i < words.size(); i++) {
          std::vector<std::string> values = this->split(words[i], '/');
          facet_vertices.push_back(std::stoi(values[0]) - 1);
          facet_uvs.push_back(std::stoi(values[1]) - 1);
          facet_normals.push_back(std::stoi(values[2]) - 1);
        }
      }
    };

    load_texture("_nm_tangent.tga", normalmap);
    load_texture("_diffuse.tga", colormap);
    load_texture("_spec.tga", specularmap);
    return 0;
  };

  void load_texture(const std::string suffix, TGAImage &img) {
    size_t dot = path.find_last_of(".");
    if (dot == std::string::npos)
      return;
    std::string texfile = path.substr(0, dot) + suffix;
    std::cerr << "texture file " << texfile << " loading "
              << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed")
              << std::endl;
  };

  vec4 vert(const int i) const { return vertices[i]; }

  vec4 vert(const int iface, const int nthvert) const {
    return vertices[facet_vertices[iface * 3 + nthvert]];
  }

  vec4 normal(const int iface, const int nthvert) const {
    return normals[facet_normals[iface * 3 + nthvert]];
  }

  vec4 normal(const vec2 &uv) const {
    TGAColor c =
        normalmap.get(uv[0] * normalmap.width(), uv[1] * normalmap.height());
    return normalized(vec4{(double)c[2], (double)c[1], (double)c[0], 0} * 2. /
                          255. -
                      vec4{1, 1, 1, 0});
  }

  vec4 color(const vec2 &uv) const {
    TGAColor c =
        colormap.get(uv[0] * colormap.width(), uv[1] * colormap.height());
    return vec4{(double)c[2], (double)c[1], (double)c[0], 0} * 2. / 255. -
           vec4{1, 1, 1, 0};
  }

  vec2 uv(const int iface, const int nthvert) const {
    return uvs[facet_uvs[iface * 3 + nthvert]];
  }

  const TGAImage &normal() const { return normalmap; }
  const TGAImage &diffuse() const { return colormap; }
  const TGAImage &specular() const { return specularmap; }

private:
  void split(const std::string &s, char delim,
             std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
      if (!item.empty()) {
        elems.push_back(item);
      }
    }
  }

  std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
  }
};
