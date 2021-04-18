#include "shape.h"

using namespace al;

// http://paulbourke.net/geometry/pointlineplane/javascript.txt
bool Shape::linesIntersect(float x1, float y1, float x2, float y2,
                           float x3, float y3, float x4, float y4,
                           float* ix, float* iy)
{
  // Check if none of the lines are of length 0
  if ((x1 == x2 && y1 == y2) || (x3 == x4 && y3 == y4)) {
    return false;
  }

  float denominator = ((y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1));

  // Lines are parallel
  if (denominator == 0) {
    return false;
  }

  float ua = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / denominator;
  float ub = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / denominator;

  // is the intersection along the segments
  if (ua < 0 || ua > 1 || ub < 0 || ub > 1) {
    return false;
  }

  // Return a object with the x and y coordinates of the intersection
  float x = x1 + ua * (x2 - x1);
  float y = y1 + ua * (y2 - y1);

  if (ix) {
    *ix = x;
  }
  if (iy) {
    *iy = y;
  }
  return true;
}

bool Shape::pointInRect(float ptx, float pty,
                        float x, float y, float w, float h)
{
  return x <= ptx && ptx <= (x + w) &&
         y <= pty && pty <= (y + h);
}

float Shape::sign(const Point& p1, const Point& p2, const Point& p3)
{
  return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

// https://stackoverflow.com/a/2049593/1645045
bool Shape::pointInTriangle(const Point& pt, const Point& v1,
                            const Point& v2, const Point& v3)
{
  float d1, d2, d3;
  bool has_neg, has_pos;

  d1 = Shape::sign(pt, v1, v2);
  d2 = Shape::sign(pt, v2, v3);
  d3 = Shape::sign(pt, v3, v1);

  has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
  has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

  return !(has_neg && has_pos);
}

bool Shape::pointInQuad(const Point& pt, const Point& v1,
                        const Point& v2, const Point& v3,
                        const Point& v4)
{
  return Shape::pointInTriangle(pt, v1, v2, v3) ||
         Shape::pointInTriangle(pt, v3, v4, v1);
}
