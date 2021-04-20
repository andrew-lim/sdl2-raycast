/*
Andrew Lim's C++ Raycasting Engine
https://github.com/andrew-lim/sdl2-raycast
*/
#ifndef ANDREW_LIM_RAYCASTING_H
#define ANDREW_LIM_RAYCASTING_H
#include <vector>
#include "shape.h"

#define THICK_WALL_TYPE_NONE 0
#define THICK_WALL_TYPE_RECT 1
#define THICK_WALL_TYPE_TRIANGLE 2
#define THICK_WALL_TYPE_QUAD 3

#define SLOPE_TYPE_WEST_EAST 1
#define SLOPE_TYPE_NORTH_SOUTH 2

namespace al {
namespace raycasting {

// A ThinWall is made up of 2 points. Similar to a linedef in Doom.
struct ThinWall {
public:
  float x1, y1, x2, y2; // start and end of line
  int wallType;
  bool horizontal;
  float height;
  float z;
  float slope;
  bool hidden;
  struct ThickWall* thickWall;
public:
  ThinWall();
  ThinWall(float x1, float y1, float x2, float y2, int wallType,
           ThickWall* thickWall, float wallHeight);
  float distanceToOrigin(float ix, float iy);
};

// A ThickWall is an enclosed area of ThinWalls.
// It can have a ceiling texture and a floor texture.
// It may also be sloped.
struct ThickWall {
public:
  int type;
  int slopeType;
  std::vector<ThinWall> thinWalls;
  float x, y, w, h; // if type is THICK_WALL_TYPE_RECT
  std::vector<Point> points; // for THICK_WALL_TYPE_TRIANGLE and QUAD
  float slope;
  int ceilingTextureID;
  int floorTextureID;
  float startHeight, endHeight;
  float tallerHeight;
  bool invertedSlope;
public:
  ThickWall();
  void createRectThickWall(float x, float y, float w, float h, float z,
                           float wallHeight);
  void createTriangleThickWall(const Point& v1, const Point& v2,
                               const Point &v3, float z, float wallHeight);
  void createQuadThickWall(const Point& v1, const Point& v2,
                           const Point &v3, const Point& v4,
                           float z, float wallHeight);
  void createRectSlope(int slopeType,
                       float x, float y, float w, float h, float z,
                       float startHeight, float endHeight);
  void createRectInvertedSlope(int slopeType,
                               float x, float y, float w, float h, float z,
                               float startHeight, float endHeight);
  void setZ(float z);
  float getZ() { return z; }
  void setHeight(float height);
  float getHeight() { return height; }
  void setThinWallsType(int wallType);
  bool containsPoint(float x, float y);
  void setTallerHeight(float height);
private:
  float height;
  float z;
};

class Sprite {
public:
    float x, y, z;
    int w, h ;
    int level;
    int dir;         // -1 for left or 1 for right.
    float rot;       // rotation (counterclockwise is positive)
    int speed;       // forward (speed = 1) or backwards (speed = -1).
    int moveSpeed;   // how far (in map units) to move each step/update
    float rotSpeed;  // rotation speed (in radians)
    float distance;  // used for z-buffer calculation
    int textureID;
    bool cleanup;
    int frameRate;
    int frame;
    bool hidden;
    bool jumping;
    float heightJumped;
    bool rayhit; // if ray has hit this sprite
    Sprite() :x(0), y(0), z(0), w(0), h(0), level(0), dir(0), rot(0), speed(0) {
      moveSpeed = 0;
      rotSpeed = 0;
      distance = 0;
      textureID = 0;
      cleanup = false;
      frameRate = 0;
      frame = 0;
      hidden = false;
      jumping = false;
      heightJumped = 0;
      rayhit = false;
    }
} ;

// Holds information about a wall hit from a single ray
struct RayHit {
  float x, y;      // wall position in game units
  int wallX, wallY; // wall position in column, row tile units
  int wallType;     // type of wall hit
  int strip;        // strip on screen for this wall
  float tileX;     // x-coordinate within tile, used for calculating texture x
  float squaredDistance; // squared distance
  float distance;  // distance to wall
  float correctDistance; // fisheye correction distance
  bool horizontal;  // true if wall was hit on the bottom or top
  float rayAngle;  // angle used for calculation
  Sprite* sprite; // a sprite was hit
  int level; // ground level (0) or some other level.
  bool right; // if ray angle is in right unit circle quadrant
  bool up; // if ray angle is in upper unit circle quadrant
  ThinWall* thinWall;
  float wallHeight;
  float invertedZ;

  // Slope sibling
  float siblingWallHeight;
  float siblingDistance;
  float siblingCorrectDistance;
  float siblingThinWallZ;
  float siblingInvertedZ;

  // sortdistance is used to sort which objects are drawn first.
  // Further objects are drawn first. Value is usually same as distance, but
  // can be different for edge cases like doors.
  float sortdistance;

  RayHit(int worldX=0, int worldY=0, float angle=0)
  : x(worldX), y(worldY), rayAngle(angle) {
    wallType = strip = wallX = wallY = tileX = squaredDistance = distance = 0;
    correctDistance = 0;
    horizontal = false;
    level = 0;
    sprite = 0;
    sortdistance = 0;
    thinWall = 0;
    wallHeight = 0;
    invertedZ = 0;
    siblingWallHeight = siblingDistance = siblingCorrectDistance = 0;
    siblingThinWallZ = siblingInvertedZ = 0;
  }

  void copySibling(RayHit& rayHit2) {
    siblingWallHeight = rayHit2.wallHeight;
    siblingDistance = rayHit2.distance;
    siblingCorrectDistance = rayHit2.correctDistance;
    if (rayHit2.thinWall) {
      siblingThinWallZ = rayHit2.thinWall->z;
    }
    siblingInvertedZ = rayHit2.invertedZ;
  }

  bool sameRayHit(const RayHit& rayHit2);
};

/**
Contains static utility functions for raycasting.

A Raycaster instance holds information for one or more 2D grids with the same
dimensions. Each grid is stored as a single vector in row-major order.
The element offset for each grid is calculated using x + y * width.

If there are two or more grids, it is effectively a 3D grid.
**/
class Raycaster {
public:
  std::vector< std::vector<int> > grids;
  int gridWidth;
  int gridHeight;
  int gridCount;
  int tileSize;
public:
  Raycaster()
  : gridWidth(0), gridHeight(0), gridCount(0), tileSize(0) {
  }

  Raycaster(int gridWidth, int gridHeight, int tileSize)
  : gridWidth(gridWidth), gridHeight(gridHeight), tileSize(tileSize) {
    createGrids(gridWidth, gridHeight, gridCount, tileSize);
  }

  void createGrids( int gridWidth, int gridHeight, int gridCount, int tileSize);

  // Distance between player to screen / projection plane
  static float screenDistance(float screenWidth, float fovRadians);

  // Relative angle between player and a ray column strip
  static float stripAngle(float screenX, float screenDistance);

  // Calculate the screen height of a wall strip.
  static float stripScreenHeight(float screenDistance, float correctDistance,
                                 float tileSize);

  static bool isWallInRayHits(std::vector<RayHit>& rayHits,int cellX,int cellY);

  static std::vector<Sprite*> findSpritesInCell(std::vector<Sprite>& sprites,
                                                int cellX, int cellY,
                                                int tileSize);

  // Checks if there are any empty blocks below specified block.
  // This checks multiple levels of blocks, not just the one directly below.
  static bool anySpaceBelow( std::vector< std::vector<int> >& grids,
                             int gridWidth, int x, int y, int z );

   // Checks if there are any empty blocks above specified block.
  // This checks multiple levels of blocks, not just the one directly above.
  static bool anySpaceAbove( std::vector< std::vector<int> >& grids,
                             int gridWidth, int x, int y, int z );

  static
  bool needsNextWall(std::vector< std::vector<int> >& grids,
                     float playerZ, int tileSize,
                     int gridWidth, int x, int y, int z );

  /*
  The raycast methods look for collisions with walls and sprites.
  The collisions are stored in rayHits.
  These functions do not perform any direct rendering.
  */
  void raycast(std::vector<RayHit>& rayHits,
               int playerX, int playerY, float playerZ,
               float playerRot, float stripAngle, int stripIdx,
               std::vector<Sprite>* spritesToLookFor=0);

  static void raycast(std::vector<RayHit>& hits,
                      std::vector< std::vector<int> >& grids,
                      int gridWidth, int gridHeight, int tileSize,
                      int playerX, int playerY, float playerZ,
                      float playerRot,
                      float stripAngle, int stripIdx,
                      std::vector<Sprite>* spritesToLookFor=0 );

  int cellAt( int x, int y ) { return grids[0][x+y*gridWidth]; }
  int cellAt( int x, int y, int z ) { return grids[z][x+y*gridWidth]; }
  int safeCellAt( int x, int y, int z, int fallback=0 ) {
    const int offset = x+y*gridWidth;
    if (z<0 || z>=gridCount || offset<0 || offset>=gridWidth*gridHeight) {
      return fallback;
    }
    return grids[z][offset];
  }

  // Reserve wall types above 1000 for door detection
  static bool isHorizontalDoor(int wallType) { return wallType > 1500; }
  static bool isVerticalDoor(int wallType) {
    return wallType > 1000 && wallType <= 1500;
  }
  static bool isDoor(int wallType) {
    return isVerticalDoor(wallType) || isHorizontalDoor(wallType);
  }

  static void findIntersectingThinWalls(std::vector<RayHit>& rayHits,
                                        std::vector<ThinWall*>& thinWalls,
                                        float playerX,
                                        float playerY,
                                        float rayEndX,
                                        float rayEndY);

  void raycastThinWalls(std::vector<RayHit>& rayHits,
                        std::vector<ThinWall*>& thinWalls,
                        float playerX, float playerY, float playerZ,
                        float playerRot,
                        float stripAngle, int stripIdx);

  bool findSiblingAtAngle(RayHit& sibling,
                          ThinWall& originThinWall,
                          std::vector<ThinWall*>& thinWalls,
                          float originAngle, float playerRot,
                          int stripIdx, float playerX, float playerY,
                          float rayStartX, float rayStartY);

  static void raycastSprites(std::vector<RayHit>& hits,
                             std::vector< std::vector<int> >& grids,
                             int gridWidth, int gridHeight, int tileSize,
                             int playerX, int playerY, float playerZ,
                             float playerRot,
                             float stripAngle, int stripIdx,
                             std::vector<Sprite>* spritesToLookFor);
};

/**
Used to sort rayhits from furthest to nearest.
raycaster - the Raycaster instance
eye       - distance between player's eye and ground
**/
struct RayHitSorter {
  Raycaster* _raycaster;
  float _eye;
  RayHitSorter(Raycaster* raycaster, float eye)
  : _raycaster(raycaster), _eye(eye)
  {}
  bool operator()(const RayHit& a, const RayHit& b) const;
};

} // raycasting
} // al

#endif
