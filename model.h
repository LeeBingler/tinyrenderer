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
  std::vector<vec3> vertices;
  std::vector<int> face;

  Model(std::string path) : path(path) {};
  ~Model() {};

  int nverts() const { return vertices.size(); }
  int nfaces() const { return face.size() / 3; }

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

      } else if (words[0] == "f") {

        for (unsigned int i = 1; i < words.size(); i++) {
          std::vector<std::string> values = this->split(words[i], '/');
          face.push_back(std::stoi(values[0]) - 1);
        }
      }
    };

    std::vector<int> idx(
        nfaces()); // permutation, a map from new to old facet indices
    for (int i = 0; i < nfaces(); i++) // we start with the identity
      idx[i] = i;

    std::sort(idx.begin(), idx.end(),
              [&](const int &a, const int &b) { // given two triangles, compare
                                                // their min z coordinate
                float aminz = std::min(vert(a, 0).z,
                                       std::min(vert(a, 1).z, vert(a, 2).z));
                float bminz = std::min(vert(b, 0).z,
                                       std::min(vert(b, 1).z, vert(b, 2).z));
                return aminz < bminz;
              });

    std::vector<int> facet_vrt2(
        nfaces() * 3); // allocate an array to store permutated facets
    for (int i = 0; i < nfaces(); i++) // for each (new) facet
      for (int j = 0; j < 3; j++) // copy its three vertices from the old array
        facet_vrt2[i * 3 + j] = face[idx[i] * 3 + j];

    face = facet_vrt2;

    return 0;
  };

  vec3 vert(const int i) const { return vertices[i]; }
  vec3 vert(const int iface, const int nthvert) const {
    return vertices[face[iface * 3 + nthvert]];
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
