#include "tgaimage.h"
#include <algorithm>

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
  return .5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) +
               (ay - cy) * (ax + cx));
}

void fillTriangleZ(int ax, int ay, int az, int bx, int by, int bz, int cx,
                   int cy, int cz, TGAImage &framebuffer) {
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

      unsigned char b = static_cast<unsigned char>(alpha * 255);
      unsigned char g = static_cast<unsigned char>(beta * 255);
      unsigned char r = static_cast<unsigned char>(gamma * 255);
      unsigned char a = static_cast<unsigned char>(255);

      if (alpha < .1 || beta < .1 || gamma < 0.1)
        framebuffer.set(x, y, {b, g, r, a});
    }
  }
}
