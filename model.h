#pragma once

#include <fstream>
#include <iostream>
#include <string>

class Model {
public:
  std::string path;
  Model(std::string path) : path(path) {};
  ~Model() {};

  int load() {
    std::string line;
    std::ifstream file(this->path);

    while (std::getline(file, line)) {
      std::cout << this->path << std::endl;
      std::cout << line << std::endl;
    };

    return 0;
  };

  int unload() { return 0; };
};
