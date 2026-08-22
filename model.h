#pragma once

#include "geometry.h"
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

  std::vector<vec3> vertices = {};
  std::vector<int> facet_vertices = {};

  std::vector<vec3> normals = {};
  std::vector<int> facet_normals = {};

  std::vector<vec2> uvs = {};
  std::vector<int> facet_uvs = {};

  Model(std::string path) : path(path) {};
  ~Model() {};

  int nverts() const { return vertices.size(); }
  int nfaces() const { return facet_vertices.size() / 3; }

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
        vec3 vertice = {std::stod(words[1]), std::stod(words[2]),
                        std::stod(words[3])};

        vertices.push_back(vertice);

      } else if (words[0] == "vn") {
        // TODO : Why is the vn line have a empty line at place 1 instead of the
        // value???

        vec3 norms = {std::stod(words[2]), std::stod(words[3]),
                      std::stod(words[4])};

        normals.push_back(normalized(norms));

      } else if (words[0] == "vt") {
        // TODO : Why is the vt line have a empty line at place 1 instead of the
        // value???

        vec2 uv = {std::stod(words[2]), std::stod(words[3])};

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

    return 0;
  };

  vec3 vert(const int i) const { return vertices[i]; }

  vec3 vert(const int iface, const int nthvert) const {
    return vertices[facet_vertices[iface * 3 + nthvert]];
  }

  vec3 normal(const int iface, const int nthvert) const {
    return normals[facet_normals[iface * 3 + nthvert]];
  }

  vec2 uv(const int iface, const int nthvert) const {
    return uvs[facet_uvs[iface * 3 + nthvert]];
  }

private:
  void split(const std::string &s, char delim,
             std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
      elems.push_back(item);
    }
  }

  std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
  }
};
