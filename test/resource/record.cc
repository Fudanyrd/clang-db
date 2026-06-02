
namespace gaming {
struct Point2D {
  double x, y;

  enum Color { RED, GREEN, BLUE };

  Point2D() : x(0), y(0) {}

  Point2D &operator+=(const Point2D &other) {
    x += other.x;
    y += other.y;
    return *this;
  }

  Point2D &operator-=(const Point2D &other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  Point2D &rotate(void) {
    double orig_x = x;
    x = -y;
    y = orig_x;
    return *this;
  }
};

Point2D operator+(const Point2D &lhs, const Point2D &rhs) {
  Point2D res = lhs;
  res += rhs;
  return res;
}

Point2D operator-(const Point2D &lhs, const Point2D &rhs) {
  Point2D res = lhs;
  res -= rhs;
  return res;
}
} // namespace gaming
