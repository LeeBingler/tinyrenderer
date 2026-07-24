#pragma once

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class Model {
public:
  std::string path;
  std::vector<std::array<double, 3>> vertices;
  std::vector<std::array<int, 3>> face;

  Model(std::string path) : path(path) {};
  ~Model() {};

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
        std::array<double, 3> vertice = {
            std::stod(words[1]), std::stod(words[2]), std::stod(words[3])};

        vertices.push_back(vertice);

      } else if (words[0] == "f") {
        std::array<int, 3> face_index;

        for (unsigned int i = 1; i < words.size(); i++) {
          std::vector<std::string> values = this->split(words[i], '/');
          face_index[i - 1] = std::stoi(values[0]) - 1;
        }

        face.push_back(face_index);
      }
    };

    return 0;
  };

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
