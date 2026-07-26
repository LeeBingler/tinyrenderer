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

std::tuple<int, int> project(vec3 v) {
  return {(v.x + 1.) * width / 2, (v.y + 1.) * height / 2};
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
