#ifndef ANDREW_SHAPE_H
#define ANDREW_SHAPE_H

namespace al {

struct Point {
  float x,y;
  Point(float x2=0, float y2=0) :x(x2), y(y2) {}
};

struct Shape
{
  static bool linesIntersect(float x1, float y1, float x2, float y2,
                             float x3, float y3, float x4, float y4,
                             float* ix, float* iy);

  static bool pointInRect(float ptx, float pty,
                          float x, float y, float w, float h);

  static float sign(const Point& p1, const Point& p2, const Point& p3);

  static bool pointInTriangle(const Point& pt,
                              const Point& v1, const Point& v2, const Point&v3);

  static bool pointInQuad(const Point& pt, const Point& v1,
                          const Point& v2, const Point& v3,
                          const Point& v4);

};

}

#endif
