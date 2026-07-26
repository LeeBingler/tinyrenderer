#include "model.h"
#include "tgaimage.h"
#include <algorithm>
#include <cmath>
#include <utility>

constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blue = {255, 128, 64, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};

constexpr int width = 64;
constexpr int height = 64;

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

std::tuple<int, int> project(vec3 vector) {
  return {(vector[0] + 1.) * width / 2, (vector[1] + 1.) * height / 2};
}

void writeWireframe(char *file, TGAImage *framebuffer) {
  Model m(file);
  m.load();

  for (int i = 0; i < m.nfaces(); i++) {
    auto [ax, ay] = project(m.vert(i, 0));
    auto [bx, by] = project(m.vert(i, 1));
    auto [cx, cy] = project(m.vert(i, 2));

    triangle(ax, ay, bx, by, cx, cy, *framebuffer, red);
  }
}

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
  return .5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) +
               (ay - cy) * (ax + cx));
}

void fillTriangle(int ax, int ay, int bx, int by, int cx, int cy,
                  TGAImage &framebuffer, TGAColor color) {
  int minX = std::min({ax, bx, cx});
  int minY = std::min({ay, by, cy});

  int maxX = std::max({ax, bx, cx});
  int maxY = std::max({ay, by, cy});
  double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
  if (total_area < 1)
    return; // backface culling + discarding triangles that cover less than a
            // pixel

#pragma omp parallel for
  for (int x = minX; x <= maxX; x++) {
    for (int y = minY; y <= maxY; y++) {
      double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
      double beta = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
      double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;

      if (alpha < 0 || beta < 0 || gamma < 0)
        continue; // negative barycentric coordinate => the pixel is outside the
                  // triangle
      framebuffer.set(x, y, color);
    }
  }
}

void writeFaces(char *file, TGAImage *framebuffer) {
  Model m(file);
  m.load();

  for (int i = 0; i < m.nfaces(); i++) {
    auto [ax, ay] = project(m.vert(i, 0));
    auto [bx, by] = project(m.vert(i, 1));
    auto [cx, cy] = project(m.vert(i, 2));

    TGAColor rnd;
    for (int c = 0; c < 3; c++)
      rnd[c] = std::rand() % 255;

    fillTriangle(ax, ay, bx, by, cx, cy, *framebuffer, rnd);
  }
}

int main(int argc, char **argv) {
  TGAImage framebuffer(width, height, TGAImage::RGB);

  int ax = 17, ay = 4, az = 13;
  int bx = 55, by = 39, bz = 128;
  int cx = 23, cy = 59, cz = 255;

  writeFaces(argv[1], &framebuffer);

  framebuffer.write_tga_file("framebuffer.tga");
  return 0;
}
