/*
Andrew Lim's C++ Raycasting Engine
https://github.com/andrew-lim/
*/
#ifndef ANDREW_LIM_RAYCASTING_H
#define ANDREW_LIM_RAYCASTING_H
#include <vector>

namespace al {
namespace raycasting {
  
class Sprite {
public:
    int x, y ;
    int w, h ;
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
    Sprite() :x(0), y(0), w(0), h(0), dir(0), rot(0), speed(0) {
      moveSpeed = 0;
      rotSpeed = 0;
      distance = 0;
      textureID = 0;
      cleanup = false;
      frameRate = 0;
      frame = 0;
      hidden = false;
    }
} ;

// Holds information about a wall hit from a single ray
struct RayHit {
  float x, y;      // wall position in game units
  int wallX, wallY; // wall position in column, row tile units
  int wallType;     // type of wall hit
  int strip;        // strip on screen for this wall
  float tileX;     // x-coordinate within tile, used for calculating texture x
  float distance;  // distance to wall
  float correctDistance; // fisheye correction distance
  bool horizontal;  // true if wall was hit on the bottom or top
  float rayAngle;  // angle used for calculation
  Sprite* sprite; // a sprite was hit
  int level; // ground level (0) or some other level.
  bool furthest;
  RayHit(int worldX=0, int worldY=0, float angle=0)
  : x(worldX), y(worldY), rayAngle(angle) {
    wallType = strip = wallX = wallY = tileX = distance = 0;
    correctDistance = 0;
    horizontal = false;
    level = 0;
    sprite = 0;
    furthest = false;
  }
  // Objects further away are drawn first using this in std::sort
  bool operator<(const RayHit& b) const {
      return distance > b.distance;
  }
};

/*
Contains static utility functions for raycasting.

A Raycaster instance  holds information for a 2D grid.
The grid is stored as a single vector in row-major order.
The offset is calculated using x + y * width.
*/
class Raycaster {
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

  /*
  The raycast methods look for collisions with walls and sprites.
  The collisions are stored in rayHits.
  These functions do not perform any direct rendering.
  */
  void raycast(std::vector<RayHit>& rayHits,
               int playerX, int playerY,
               float playerRot, float stripAngle, int stripIdx,
               std::vector<Sprite>* spritesToLookFor=0);
                      
  static void raycast(std::vector<RayHit>& hits,
                      std::vector< std::vector<int> >& grids,
                      int gridWidth, int gridHeight, int tileSize,
                      int playerX, int playerY, float playerRot,
                      float stripAngle, int stripIdx,
                      std::vector<Sprite>* spritesToLookFor=0 );

  std::vector< std::vector<int> > grids;
  int gridWidth;
  int gridHeight;
  int gridCount;
  int tileSize;
  
  int cellAt( int x, int y ) { return grids[0][x+y*gridWidth]; }
  int cellAt( int x, int y, int z ) { return grids[z][x+y*gridWidth]; }
};

} // raycasting
} // al

#endif
