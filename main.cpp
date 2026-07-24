#include "model.h"
#include "tgaimage.h"
#include <cmath>
#include <utility>

constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blue = {255, 128, 64, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};

constexpr int width = 800;
constexpr int height = 800;

void line(int ax, int ay, int bx, int by, TGAImage &framebuf, TGAColor color) {
  bool steep = std::abs(ax - bx) < std::abs(ay - by);
  if (steep) { // if the line is steep, we transpose the image
    std::swap(ax, ay);
    std::swap(bx, by);
  }
  if (ax > bx) {
    std::swap(ax, bx);
    std::swap(ay, by);
  }

  int y = ay;
  int ierror = 0;

  for (int x = ax; x <= bx; x++) {
    if (steep) // if transposed, de−transpose
      framebuf.set(y, x, color);
    else
      framebuf.set(x, y, color);

    ierror += 2 * std::abs(by - ay);
    y += (by > ay ? 1 : -1) * (ierror > bx - ax);
    ierror -= 2 * (bx - ax) * (ierror > bx - ax);
  }
}

void triangle(int ax, int ay, int bx, int by, int cx, int cy,
              TGAImage &framebuffer, TGAColor color) {
  line(ax, ay, bx, by, framebuffer, color);
  line(bx, by, cx, cy, framebuffer, color);
  line(cx, cy, ax, ay, framebuffer, color);
}

std::tuple<int, int> project(std::array<double, 3> vector) {
  return {(vector[0] + 1.) * width / 2, (vector[1] + 1.) * height / 2};
}

int main(int argc, char **argv) {
  TGAImage framebuffer(width, height, TGAImage::RGB);

  Model m(argv[1]);
  m.load();

  for (auto face = m.face.begin(); face != m.face.end(); face++) {
    auto [ax, ay] = project(m.vertices[face->at(0)]);
    auto [bx, by] = project(m.vertices[face->at(1)]);
    auto [cx, cy] = project(m.vertices[face->at(2)]);

    triangle(ax, ay, bx, by, cx, cy, framebuffer, red);
  }

  framebuffer.write_tga_file("framebuffer.tga");
  return 0;
}
