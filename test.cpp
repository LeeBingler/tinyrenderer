#include "test.hpp"

void test_shadow_mask(const int width, const int height, TGAImage trash,
                      const std::vector<bool> isLit) {
  trash.write_tga_file("light_pov.tga");
  TGAImage maskimg(width, height, TGAImage::GRAYSCALE);
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      if (isLit[x + y * width])
        continue;
      maskimg.set(x, y, {255, 255, 255, 255});
    }
  }
  maskimg.write_tga_file("mask.tga");
}
