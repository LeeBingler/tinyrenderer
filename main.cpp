#include "model.h"
#include "tgaimage.h"
#include <algorithm>

constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blue = {255, 128, 64, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};

constexpr int width = 800;
constexpr int height = 800;

vec3 rot(vec3 v) {
  constexpr double a = M_PI / 6;
  constexpr mat<3, 3> Ry = {{{std::cos(a), 0, std::sin(a)},
                             {0, 1, 0},
                             {-std::sin(a), 0, std::cos(a)}}};
  return Ry * v;
}

vec3 persp(vec3 v) {
  constexpr double c = 6.;
  return v / (1 - v.z / c);
}

std::tuple<int, int, int> project(vec3 v) {
  // First of all, (x,y) is an orthogonal projection of the
  // vector (x,y,z).
  // Second, since the input models are scaled to have fit in the
  // [-1,1]^3 world coordinates,
  // we want to shift the vector (x,y) and then
  // scale it to span the entire screen.

  return {(v.x + 1.) * width / 2, (v.y + 1.) * height / 2,
          (v.z + 1.) * 255. / 2};
}

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
  return .5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) +
               (ay - cy) * (ax + cx));
}

void triangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy,
              int cz, TGAImage &framebuffer, TGAImage &zBuffer,
              TGAColor color) {
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
      unsigned char z =
          static_cast<unsigned char>(alpha * az + beta * bz + gamma * cz);
      if (z <= zBuffer.get(x, y)[0])
        continue;
      zBuffer.set(x, y, {z});
      framebuffer.set(x, y, color);
    }
  }
}

int main(int argc, char **argv) {
  TGAImage framebuffer(width, height, TGAImage::RGB);
  TGAImage zBuffer(width, height, TGAImage::RGB);

  Model m(argv[1]);
  m.load();

  for (int i = 0; i < m.nfaces(); i++) {
    auto [ax, ay, az] = project(persp(rot(m.vert(i, 0))));
    auto [bx, by, bz] = project(persp(rot(m.vert(i, 1))));
    auto [cx, cy, cz] = project(persp(rot(m.vert(i, 2))));

    TGAColor rnd;
    for (int c = 0; c < 3; c++)
      rnd[c] = std::rand() % 255;

    triangle(ax, ay, az, bx, by, bz, cx, cy, cz, framebuffer, zBuffer, rnd);
  }

  framebuffer.write_tga_file("framebuffer.tga");
  zBuffer.write_tga_file("zBuffer.tga");
  return 0;
}
