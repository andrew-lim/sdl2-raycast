#include "raycasting.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cassert>
#include "shape.h"
using namespace std;
using namespace al::raycasting;

ThinWall::ThinWall()
{
  this->x1 = 0;
  this->y1 = 0;
  this->x2 = 0;
  this->y2 = 0;
  wallType = 0;
  horizontal = false;
  height = 0;
  z = 0;
  hidden = false;
  thickWall = 0;
  slope = 0;
}

ThinWall::ThinWall(float x1, float y1, float x2, float y2, int wallType,
                   ThickWall* thickWall, float wallHeight)
{
  this->x1 = x1;
  this->y1 = y1;
  this->x2 = x2;
  this->y2 = y2;
  this->wallType = wallType;
  horizontal = false;
  height = wallHeight;
  z = 0;
  hidden = false;
  this->thickWall = thickWall;
  slope = 0;
}

float ThinWall::distanceToOrigin(float ix, float iy)
{
  float dx = x1 - ix;
  float dy = y1 - iy;
  return sqrt(dx*dx + dy*dy);
}

ThickWall::ThickWall()
{
  type = 0;
  slopeType = 0;
  height = 0;
  x = y = w = h = z = 0;
  slope = 0;
  ceilingTextureID = 0;
  floorTextureID = 0;
  startHeight = endHeight = tallerHeight = 0;
  invertedSlope = false;
}

void ThickWall::createRectThickWall(float x, float y, float w, float h,
                                    float z, float wallHeight)
{
  this->type = THICK_WALL_TYPE_RECT;
  this->x = x;
  this->y = y;
  this->w = w;
  this->h = h;
  Point topLeft(x,y);
  Point topRight(x+w, y);
  Point bottomLeft(x, y+h);
  Point bottomRight(x+w, y+h);
  int wallType = 0;
  thinWalls.clear();
  // West
  thinWalls.push_back(ThinWall(topLeft.x,topLeft.y,bottomLeft.x,bottomLeft.y,
                               wallType, this, wallHeight));

  // East
  thinWalls.push_back(ThinWall(topRight.x, topRight.y, bottomRight.x,
                               bottomRight.y, wallType, this, wallHeight));

  // North
  thinWalls.push_back(ThinWall(topLeft.x, topLeft.y, topRight.x, topRight.y,
                               wallType, this, wallHeight));


  // South
  thinWalls.push_back(ThinWall(bottomLeft.x, bottomLeft.y, bottomRight.x,
                               bottomRight.y, wallType, this, wallHeight));

  thinWalls[2].horizontal = true;
  thinWalls[3].horizontal = true;

  setHeight(wallHeight);
  setZ(z);
}

void ThickWall::createTriangleThickWall(const Point& v1, const Point& v2,
                                        const Point &v3, float z,
                                        float h)
{
  this->type = THICK_WALL_TYPE_TRIANGLE;
  points.clear();
  points.push_back(v1);
  points.push_back(v2);
  points.push_back(v3);
  int wallType = 0;
  thinWalls.clear();
  thinWalls.push_back(ThinWall(v1.x, v1.y, v2.x, v2.y, wallType, this, h));
  thinWalls.push_back(ThinWall(v2.x, v2.y, v3.x, v3.y, wallType, this, h));
  thinWalls.push_back(ThinWall(v3.x, v3.y, v1.x, v1.y, wallType, this, h));
  thinWalls[1].horizontal = true;
  setHeight(h);
  setZ(z);
}

void ThickWall::createQuadThickWall(const Point& v1, const Point& v2,
                                    const Point &v3, const Point& v4,
                                    float z, float h)
{
  this->type = THICK_WALL_TYPE_QUAD;
  points.clear();
  points.push_back(v1);
  points.push_back(v2);
  points.push_back(v3);
  points.push_back(v4);
  int wallType = 0;
  thinWalls.clear();
  thinWalls.push_back(ThinWall(v1.x, v1.y, v2.x, v2.y, wallType, this, h));
  thinWalls.push_back(ThinWall(v2.x, v2.y, v3.x, v3.y, wallType, this, h));
  thinWalls.push_back(ThinWall(v3.x, v3.y, v4.x, v4.y, wallType, this, h));
  thinWalls.push_back(ThinWall(v4.x, v4.y, v1.x, v1.y, wallType, this, h));
  setHeight(h);
  setZ(z);
  thinWalls[2].horizontal = true;
  thinWalls[3].horizontal = true;
}

void ThickWall::createRectSlope(int slopeType,
                                float x, float y, float w, float h, float z,
                                float startHeight, float endHeight)
{
  this->slopeType = slopeType;
  this->startHeight = startHeight;
  this->endHeight = endHeight;
  tallerHeight = startHeight > endHeight ? startHeight : endHeight;
  if (SLOPE_TYPE_WEST_EAST == slopeType) {
    createRectThickWall(x,y,w,h,z,endHeight);
    this->slope = (endHeight - startHeight) / w;
    thinWalls[ 0 ].height = startHeight; // west
    thinWalls[ 1 ].height = endHeight; // east
    thinWalls[ 2 ].slope  = slope; // north
    thinWalls[ 3 ].slope  = slope; //south
  }
  else if (SLOPE_TYPE_NORTH_SOUTH == slopeType) {
    createRectThickWall(x,y,w,h,z,endHeight);
    this->slope = (endHeight - startHeight) / h;
    thinWalls[ 0 ].slope = slope; // west
    thinWalls[ 1 ].slope = slope; // east
    thinWalls[ 2 ].height  = startHeight; // north
    thinWalls[ 3 ].height  = endHeight; //south
  }
  setZ(z);
}

void ThickWall::createRectInvertedSlope(int slopeType,
                                        float x, float y, float w, float h,
                                        float z,
                                        float startHeight, float endHeight)
{
  this->invertedSlope = true;
  this->slopeType = slopeType;
  this->startHeight = startHeight;
  this->endHeight = endHeight;

  tallerHeight = startHeight > endHeight ? startHeight : endHeight;

  float newStartHeight = (tallerHeight - startHeight);
  float newEndHeight   = (tallerHeight - endHeight);
  if (newStartHeight == 0) {
    newStartHeight = 1;
  }
  if (newEndHeight == 0) {
    newEndHeight = 1;
  }

  if (SLOPE_TYPE_WEST_EAST == slopeType) {
    createRectThickWall(x,y,w,h,z,endHeight);
    this->slope = (endHeight - startHeight) / w;
    thinWalls[ 0 ].height = newStartHeight; // west
    thinWalls[ 1 ].height = newEndHeight; // east
    thinWalls[ 2 ].slope  = slope; // north
    thinWalls[ 3 ].slope  = slope; //south
    setZ(z);
    thinWalls[ 0 ].z = z + startHeight;
    thinWalls[ 1 ].z = z + endHeight;
  }
  else if (SLOPE_TYPE_NORTH_SOUTH == slopeType) {
    createRectThickWall(x,y,w,h,z,endHeight);
    this->slope = (endHeight - startHeight) / h;
    thinWalls[ 0 ].slope = slope; // west
    thinWalls[ 1 ].slope = slope; // east
    thinWalls[ 2 ].height  = newStartHeight; // north
    thinWalls[ 3 ].height  = newEndHeight; //south
    setZ(z);
    thinWalls[ 2 ].z = z + startHeight;
    thinWalls[ 3 ].z = z + endHeight;
  }
}

void ThickWall::setTallerHeight(float newTallerHeight)
{
  tallerHeight = newTallerHeight;

  if (invertedSlope) {
    if (startHeight > endHeight) {
      startHeight = tallerHeight;
    }
    else {
      endHeight = tallerHeight;
    }

    float newStartHeight = (tallerHeight - startHeight);
    float newEndHeight   = (tallerHeight - endHeight);
    if (newStartHeight == 0) {
      newStartHeight = 1;
    }
    if (newEndHeight == 0) {
      newEndHeight = 1;
    }

    if (SLOPE_TYPE_WEST_EAST == slopeType) {
      this->slope = (endHeight - startHeight) / w;
      thinWalls[ 0 ].height = newStartHeight; // west
      thinWalls[ 1 ].height = newEndHeight; // east
      thinWalls[ 2 ].slope  = slope; // north
      thinWalls[ 3 ].slope  = slope; //south
      setZ(z);
      thinWalls[ 0 ].z = z + startHeight;
      thinWalls[ 1 ].z = z + endHeight;
    }
    else if (SLOPE_TYPE_NORTH_SOUTH == slopeType) {
      this->slope = (endHeight - startHeight) / h;
      thinWalls[ 0 ].slope = slope; // west
      thinWalls[ 1 ].slope = slope; // east
      thinWalls[ 2 ].height  = newStartHeight; // north
      thinWalls[ 3 ].height  = newEndHeight; //south
      setZ(z);
      thinWalls[ 2 ].z = z + startHeight;
      thinWalls[ 3 ].z = z + endHeight;
    }
  }
  // Normal slope
  else {
    if (SLOPE_TYPE_WEST_EAST == slopeType) {
      if (thinWalls[ 0 ].height > thinWalls[ 1 ].height) {
        thinWalls[ 0 ].height = newTallerHeight;
        startHeight = tallerHeight;
        slope = (endHeight - startHeight) / w;
      }
      else if (thinWalls[ 1 ].height > thinWalls[ 0 ].height) {
        thinWalls[ 1 ].height = newTallerHeight;
        endHeight = tallerHeight;
        slope = (endHeight - startHeight) / w;
      }
      thinWalls[ 2 ].slope  = slope; // north
      thinWalls[ 3 ].slope  = slope; //south
    }
    else if (SLOPE_TYPE_NORTH_SOUTH == slopeType) {
      if (thinWalls[ 2 ].height > thinWalls[ 3 ].height) {
        thinWalls[ 2 ].height = newTallerHeight;
        tallerHeight = newTallerHeight;
      }
      else if  (thinWalls[ 3 ].height > thinWalls[ 3 ].height) {
        thinWalls[ 3 ].height = newTallerHeight;
        tallerHeight = newTallerHeight;
      }
    }
  }
}

void ThickWall::setZ(float z)
{
  this->z = z;
  for (size_t i=0; i<thinWalls.size(); ++i) {
    thinWalls[i].z = z;
  }
}

void ThickWall::setHeight(float wallHeight)
{
  this->height = wallHeight;
  for (size_t i=0; i<thinWalls.size(); ++i) {
    thinWalls[i].height = wallHeight;
  }
}

void ThickWall::setThinWallsType(int wallType)
{
  for (size_t i=0; i<thinWalls.size(); ++i) {
    thinWalls[i].wallType = wallType;
  }
}

bool ThickWall::containsPoint(float px, float py)
{
  if (type == THICK_WALL_TYPE_RECT) {
    return Shape::pointInRect(px, py, this->x, this->y, this->w, this->h);
  }
  if (type == THICK_WALL_TYPE_TRIANGLE) {
    return Shape::pointInTriangle(Point(px,py),points[2],points[1],points[0]);
  }
  if (type == THICK_WALL_TYPE_QUAD) {
    return Shape::pointInQuad(
      Point(px,py),points[0],points[1],points[2],points[3]
    );
  }
  return false;
}

bool RayHit::sameRayHit(const RayHit& rayHit2)
{
  const RayHit& rayHit = *this;
  return rayHit.x        == rayHit2.x &&
         rayHit.y        == rayHit2.y &&
         rayHit.wallType == rayHit2.wallType &&
         rayHit.strip    == rayHit2.strip &&
         rayHit.thinWall == rayHit2.thinWall &&
         rayHit.rayAngle == rayHit2.rayAngle;
}

#define USE_FMOD 1

static float fmod2( float a, int b ) {
  #if USE_FMOD == 1
  return fmod(a,b);
  #else
  return (int)a % b; // faster but causes texture seams
  #endif
}

bool RayHitSorter::operator()(const RayHit& a, const RayHit& b) const
{
  // If either wall is a ThinWall, just use the direct distance
  if (a.thinWall || b.thinWall) {
    return a.distance > b.distance;
  }

  // Sort by distance between player's eye to wall bottom.
  // Further walls drawn first.
  float wallBottomA = a.level*_raycaster->tileSize;
  float wallBottomB = b.level*_raycaster->tileSize;
  float vDistanceToEyeA = _eye-wallBottomA;
  float vDistanceToEyeB = _eye-wallBottomB;

  float distanceA = a.sortdistance ? a.sortdistance : a.distance;
  float distanceB = b.sortdistance ? b.sortdistance : b.distance;

  float distanceToWallBaseA = vDistanceToEyeA*vDistanceToEyeA +
                              distanceA*distanceA;
  float distanceToWallBaseB = vDistanceToEyeB*vDistanceToEyeB +
                              distanceB*distanceB;
  return distanceToWallBaseA > distanceToWallBaseB;
}

void Raycaster::createGrids( int gridWidth, int gridHeight, int gridCount,
                             int tileSize)
{
  this->gridWidth = gridWidth;
  this->gridHeight = gridHeight;
  this->gridCount = gridCount;
  this->tileSize = tileSize;
  grids.clear();
  for (int z=0; z<gridCount; ++z) {
    std::vector<int> grid;
    grid.resize( gridWidth * gridHeight );
    grids.push_back(grid);
  }
}

/*
 screenWidth
+-----+------+  -
\     |     /   ^
 \    |    /    |
  \   |   /     | screenDistance aka projectionPlaneDistance
   \  |  /      |
    \fov/       |
     \|/        v
      v         -

tan(angle) = opposite / adjacent
tan(fov/2) = (screenwidth/2) / screenDistance
screenDistance = (screenwidth/2) / tan(fov/2)
*/
float Raycaster::screenDistance(float screenWidth, float fovRadians){
  return (screenWidth/2) / tan((fovRadians/2));
}

/*
      screenX
            <------
            +-----+------+  +-
            \     |     /   |
             \    |    /    |
rayViewDist   \   |   /     | screenDistance aka projectionPlaneDistance
               \  |  /      |
                \a| /       |
                 \|/        |
                  v         |_

// Pythagoras
rayViewDist = squareroot(screenX*screenX + screenDistance*screenDistance)

// asin is the reverse of sin
asin(opposite / hypotenuse) = angle
angle = asin(opposite / hypotenuse)
a = asin( screenX / rayViewDist )
OR
a = atan( screenX / screenDistance )
*/
float Raycaster::stripAngle(float screenX, float screenDistance) {
  return atan(screenX/screenDistance);

  // Old way using asin()
  // float rayViewDist = sqrt(screenX*screenX +screenDistance*screenDistance);
  // return asin(screenX / rayViewDist);
}

// Calculate the screen height of a wall strip.
// Similar triangle principle
float Raycaster::stripScreenHeight(float screenDistance, float correctDistance,
                                   float tileSize) {
  // Use floor(+0.5) instead of round() because not all compilers have round()
  return floor(screenDistance / correctDistance*tileSize + 0.5 );
}


bool Raycaster::isWallInRayHits(vector<RayHit>& rayHits, int cellX, int cellY) {
  for (vector<RayHit>::iterator it=rayHits.begin(); it!=rayHits.end();
       ++it) {
    RayHit& rayHit = *it;
    if (rayHit.wallType && rayHit.wallX==cellX && rayHit.wallY==cellY) {
      return true;
    }
  }
  return false;
}

vector<Sprite*> Raycaster::findSpritesInCell(vector<Sprite>& sprites,
                                             int cellX, int cellY,
                                             int tileSize)
{
  vector<Sprite*> spritesFound;
  for (vector<Sprite>::iterator it=sprites.begin(); it!=sprites.end(); ++it) {
    Sprite* sprite = &*it;
    if (cellX == ((int)sprite->x/tileSize) &&
        cellY == ((int)sprite->y/tileSize) ) {
      spritesFound.push_back(sprite);
    }
  }
  return spritesFound;
}

// Checks if there are any empty blocks below specified block
bool Raycaster::anySpaceBelow( std::vector< std::vector<int> >& grids,
                               int gridWidth, int x, int y, int z )
{
  if (z==0) {
    std::vector<int>& grid = grids[ 0 ];
    if (x>=0 && y>=0) {
      if (isDoor(grid[x+y*gridWidth])) {
        return true;
      }
    }
  }
  for (int level=z-1; level>=0; level--) {
    std::vector<int>& grid = grids[ level ];
    int gridOffset = x + y*gridWidth;
    if (0 == grid[gridOffset] || isDoor(grid[gridOffset])) {
      return true;
    }
  }
  return false;
}

// Checks if there are any empty blocks above specified block
bool Raycaster::anySpaceAbove( std::vector< std::vector<int> >& grids,
                               int gridWidth, int x, int y, int z )
{
  if (z==0) {
    std::vector<int>& grid = grids[ 0 ];
    if (x>=0 && y>=0) {
      if (isDoor(grid[x+y*gridWidth])) {
        return true;
      }
    }
  }
  for (int level=z+1; level<(int)grids.size(); level++) {
    std::vector<int>& grid = grids[ level ];
    int gridOffset = x + y*gridWidth;
    if (0 == grid[gridOffset] || isDoor(grid[gridOffset])) {
      return true;
    }
  }
  return false;
}

bool Raycaster::needsNextWall(std::vector< std::vector<int> >& grids,
                              float playerZ, int tileSize,
                              int gridWidth, int x, int y, int z )
{
  if (z==0) {
    std::vector<int>& grid = grids[ 0 ];
    if (x>=0 && y>=0) {
      if (isDoor(grid[x+y*gridWidth])) {
        return true;
      }
    }
  }

  float eyeHeight  = tileSize/2 + playerZ;
  float wallBottom = z * tileSize;
  float wallTop    = wallBottom + tileSize;
  bool eyeAboveWall = eyeHeight > wallTop;
  bool eyeBelowWall = eyeHeight < wallBottom;

  if (eyeAboveWall) {
    return anySpaceAbove(grids, gridWidth, x, y, z);
  }
  if (eyeBelowWall) {
    return anySpaceBelow(grids, gridWidth, x, y, z);
  }
  return false;
}

void Raycaster::raycast(std::vector<RayHit>& rayHits,
                        int playerX, int playerY, float playerZ,
                        float playerRot, float stripAngle, int stripIdx,
                        std::vector<Sprite>* spritesToLookFor)
{
  Raycaster::raycast(rayHits, this->grids,
                     this->gridWidth, this->gridHeight, this->tileSize,
                     playerX, playerY, playerZ, playerRot,
                     stripAngle, stripIdx,
                     spritesToLookFor);
}

void Raycaster::raycast(vector<RayHit>& hits,
                        vector< vector<int> >& grids,
                        int gridWidth, int gridHeight, int tileSize,
                        int playerX, int playerY, float playerZ,
                        float playerRot,
                        float stripAngle, int stripIdx,
                        vector<Sprite>* spritesToLookFor)
{
  if (grids.empty()) {
    return;
  }

  // This should always be false. However if some wall strips that should be
  // detected are not, setting this to true can help debug the problem.
  bool findAllWalls = false;

  float rayAngle = stripAngle + playerRot;
  const float TWO_PI = M_PI*2;
  while (rayAngle < 0) rayAngle += TWO_PI;
  while (rayAngle >= TWO_PI) rayAngle -= TWO_PI;

  bool right = (rayAngle<TWO_PI*0.25 && rayAngle>=0) || // Quadrant 1
              (rayAngle>TWO_PI*0.75); // Quadrant 4
  bool up    = rayAngle<TWO_PI*0.5  && rayAngle>=0; // Quadrant 1 and 2

  std::vector<int>& groundGrid = grids[0];

  int currentTileX = playerX / tileSize;
  int currentTileY = playerY / tileSize;

  for (int level=0; level<(int)grids.size(); ++level) {
    vector<int>& grid = grids[level];

    //--------------------------------------------------------------------------
    // Check if the player is standing below or above a wall, and add that wall
    // Without this you might see some wall bottom or top surfaces you're above
    // or below disappear abruptly.
    // Figured this out by trial and error!
    // Not really sure why it works. :|
    //--------------------------------------------------------------------------
    int playerOffset = currentTileX + currentTileY*gridWidth;
    float trialAndErrorDistance = 10.0f;
    if (level+1<(int)grids.size() && grids[level+1][playerOffset] > 0) {
      const float distX = trialAndErrorDistance;
      const float distY = trialAndErrorDistance;
      const float blockDist = distX*distX + distY*distY;
      if (blockDist) {
        float texX = fmod2(playerY, tileSize);
        texX = right ? texX : tileSize - texX; // Facing left, flip image
        RayHit rayHit(playerX, playerY, rayAngle);
        rayHit.strip = stripIdx;
        rayHit.wallType = grids[level+1][playerOffset];
        rayHit.wallX = currentTileX;
        rayHit.wallY = currentTileY;
        rayHit.level = level+1;
        rayHit.distance = sqrt(blockDist);
        rayHit.correctDistance = rayHit.distance * cos(stripAngle);
        rayHit.horizontal = false;
        rayHit.tileX = texX;
        hits.push_back( rayHit );
      }
    }
    if (level-1>=0 && grids[level-1][playerOffset] > 0 &&
        !isDoor(grids[level-1][playerOffset]) ) {
      const float distX = trialAndErrorDistance;
      const float distY = trialAndErrorDistance;
      const float blockDist = distX*distX + distY*distY;
      if (blockDist) {
        float texX = fmod2(playerY, tileSize);
        texX = right ? texX : tileSize - texX; // Facing left, flip image
        RayHit rayHit(playerX, playerY, rayAngle);
        rayHit.strip = stripIdx;
        rayHit.wallType = grids[level-1][playerOffset];
        rayHit.wallX = currentTileX;
        rayHit.wallY = currentTileY;
        rayHit.level = level-1;
        rayHit.distance = sqrt(blockDist);
        rayHit.correctDistance = rayHit.distance * cos(stripAngle);
        rayHit.horizontal = false;
        rayHit.tileX = texX;
        hits.push_back( rayHit );
      }
    }

    //----------------------------------------
    // Check player's current tile for sprites
    //----------------------------------------
    vector<Sprite*> spritesHit;

    if (spritesToLookFor) {
      vector<Sprite*> spritesFound = findSpritesInCell( *spritesToLookFor,
                                                        currentTileX,
                                                        currentTileY,
                                                        tileSize );
      for (size_t i=0; i<spritesFound.size(); ++i) {
        Sprite* sprite = spritesFound[ i ];
        if (!sprite->rayhit) {
          const float distX = playerX - sprite->x;
          const float distY = playerY - sprite->y;
          const float blockDist = distX*distX + distY*distY;
          if (blockDist) {
            sprite->rayhit = true;
            sprite->distance = sqrt(blockDist);
            spritesHit.push_back(sprite);
          }
        }
      }
    }

    //--------------------------
    // Vertical Lines Checking
    //--------------------------
    float verticalLineDistance = 0;
    RayHit verticalWallHit;

    // Find x coordinate of vertical lines on the right and left
    float vx = 0;
    if (right) {
      vx = floor(playerX/tileSize) * tileSize + tileSize;
    }
    else {
      vx = floor(playerX/tileSize) * tileSize - 1;
    }

    // Calculate y coordinate of those lines
    // lineY = playerY + (playerX-lineX)*tan(ALPHA);
    float vy = playerY + (playerX-vx)*tan(rayAngle);

    // Calculate stepping vector for each line
    float stepx = right ? tileSize : -tileSize;
    float stepy = tileSize * tan(rayAngle);

    // tan() returns positive values in Quadrant 1 and Quadrant 4 but window
    // coordinates need negative coordinates for Y-axis so we reverse
    if ( right ) {
      stepy = -stepy;
    }

    // TODO: We might not need this prevGaps variable at all.
    // Remove it and do more testing to see if nothing is affected.
    bool prevGaps = false;
    while (vx>=0 && vx<gridWidth*tileSize && vy>=0 && vy<gridHeight*tileSize) {
      int wallY = floor(vy / tileSize);
      int wallX = floor(vx / tileSize);
      int wallOffset = wallX + wallY * gridWidth;

      // Look for sprites in current cell
      if (spritesToLookFor) {
        vector<Sprite*> spritesFound = findSpritesInCell( *spritesToLookFor,
                                                          wallX, wallY,
                                                          tileSize );
        for (size_t i=0; i<spritesFound.size(); ++i) {
          Sprite* sprite = spritesFound[ i ];
          if (!sprite->rayhit) {
            const float distX = playerX - sprite->x;
            const float distY = playerY - sprite->y;
            const float blockDist = distX*distX + distY*distY;
            sprite->distance = sqrt(blockDist);
            spritesHit.push_back(sprite);

            RayHit spriteRayHit(vx, vy, rayAngle);
            spriteRayHit.strip = stripIdx;
            if (sprite->distance) {
              spriteRayHit.distance = sprite->distance;
              spriteRayHit.correctDistance = spriteRayHit.distance *
                                              cos(stripAngle);
            }
            spriteRayHit.wallType = 0;
            spriteRayHit.sprite = sprite;
            spriteRayHit.level = sprite->level;
            spriteRayHit.distance = sprite->distance;
            sprite->rayhit = true;
            hits.push_back( spriteRayHit );
          }
        }
      }

      // Check if current cell is a wall
      if (grid[wallOffset]>0 && !isHorizontalDoor(grid[wallOffset])) {

        float distX = playerX - vx;
        float distY = playerY - vy;
        float blockDist = distX*distX + distY*distY;
        if (blockDist) {
          float texX = fmod2(vy, tileSize);
          texX = right ? texX : tileSize - texX; // Facing left, flip image
          RayHit rayHit(vx, vy, rayAngle);
          rayHit.strip = stripIdx;
          rayHit.wallType = grid[wallOffset];
          rayHit.wallX = wallX;
          rayHit.wallY = wallY;
          rayHit.level = level;
          rayHit.up = up;
          rayHit.right = right;
          rayHit.distance = sqrt(blockDist);
          rayHit.sortdistance = rayHit.distance;
          bool canAdd = true;
          // If a door was hit, move ray halfway inside
          if (isVerticalDoor(grid[wallOffset])) {
            int newWallY = floor((vy+stepy/2) / tileSize);
            int newWallX = floor((vx+stepx/2) / tileSize);
            if (newWallY==wallY && newWallX==wallX) {
              float halfDistance = stepx/2*stepx/2 + stepy/2*stepy/2;
              float halfDistanceSquared = sqrt( halfDistance );
              rayHit.distance += halfDistanceSquared;
              texX = fmod2(vy+stepy/2, tileSize);

              // Give doors slightly lower drawing priority to prevent the
              // wall above drawing its bottom surface later than the door
              rayHit.sortdistance -= 1;
            }
            else {
              canAdd = false;
            }
          }
          rayHit.correctDistance = rayHit.distance * cos(stripAngle);
          rayHit.horizontal = false;
          rayHit.tileX = texX;
          bool gaps=needsNextWall(grids,playerZ,tileSize,gridWidth,wallX,wallY,
                                  level);
          // There is an empty space below this wall, or the wall before
          if (gaps) {
            prevGaps = gaps; // for next wall check
          }
          else if (!findAllWalls) {
            verticalWallHit = rayHit;
            verticalLineDistance = blockDist;
            break;
          }
          if (canAdd) {
            hits.push_back( rayHit );
          }
        }
      }
      vx += stepx;
      vy += stepy;
    }

    //--------------------------
    // Horizontal Lines Checking
    //--------------------------
    float horizontalLineDistance = 0;

    // Find y coordinate of horizontal lines on the right and left
    float hy = 0;
    if (up) {
      hy = floor(playerY/tileSize) * tileSize - 1;
    }
    else {
      hy = floor(playerY/tileSize) * tileSize + tileSize;
    }

    // Calculation x coordinate of horizontal line
    // lineX = playerX + (playerY-lineY)/tan(ALPHA);
    float hx = playerX + (playerY-hy) / tan(rayAngle);
    stepy = up ? -tileSize : tileSize;
    stepx = tileSize / tan(rayAngle);

    // tan() returns stepx as positive in quadrant 3 and negative in quadrant 4
    // This is the opposite of window coordinates so we need to reverse when
    // angle is facing down
    if ( !up ) {
      stepx = -stepx;
    }

    prevGaps = false;
    while (hx>=0 && hx<gridWidth*tileSize && hy>=0 && hy<gridHeight*tileSize) {
      int wallY = floor(hy / tileSize);
      int wallX = floor(hx / tileSize);
      int wallOffset = wallX + wallY * gridWidth;

      // Look for sprites in current cell
      if (spritesToLookFor) {
        vector<Sprite*> spritesFound = findSpritesInCell( *spritesToLookFor,
                                                          wallX, wallY,
                                                          tileSize );
        for (size_t i=0; i<spritesFound.size(); ++i) {
          Sprite* sprite = spritesFound[ i ];
          if (!sprite->rayhit) {
            const float distX = playerX - sprite->x;
            const float distY = playerY - sprite->y;
            const float blockDist = distX*distX + distY*distY;
            sprite->distance = sqrt(blockDist);
            spritesHit.push_back(sprite);
            RayHit spriteRayHit(hx, hy, rayAngle);
            spriteRayHit.strip = stripIdx;
            if (sprite->distance) {
              spriteRayHit.distance = sprite->distance;
              spriteRayHit.correctDistance = spriteRayHit.distance *
                                              cos(stripAngle);
            }
            spriteRayHit.wallType = 0;
            spriteRayHit.sprite = sprite;
            spriteRayHit.level = sprite->level;
            spriteRayHit.distance = sprite->distance;
            sprite->rayhit = true;
            hits.push_back( spriteRayHit );
          }
        }
      }

      // Check if current cell is a wall
      if (grid[wallOffset]>0 && !isVerticalDoor(grid[wallOffset])) {
        const float distX = playerX - hx;
        const float distY = playerY - hy;
        float blockDist = distX*distX + distY*distY;

        // If vertical distance is less than horizontal line distance, stop
        if (verticalLineDistance>0 && verticalLineDistance<blockDist) {
          // Unless there was some space below previous wall
          if (!prevGaps) {
            break;
          }
        }

        if (blockDist) {
          float texX =  fmod2(hx, tileSize);
          texX = up ? texX : tileSize - texX; // Facing down, flip image
          RayHit rayHit(hx, hy, rayAngle);
          rayHit.strip = stripIdx;
          rayHit.wallType = grid[wallOffset];
          rayHit.wallX = wallX;
          rayHit.wallY = wallY;
          rayHit.level = level;
          rayHit.up = up;
          rayHit.right = right;
          rayHit.distance = sqrt(blockDist);
          rayHit.sortdistance = rayHit.distance;
          bool canAdd = true;
          // If a door was hit, move ray halfway inside
          if (isHorizontalDoor(grid[wallOffset])) {
            int newWallY = floor((hy+stepy/2) / tileSize);
            int newWallX = floor((hx+stepx/2) / tileSize);
            if (newWallY==wallY && newWallX==wallX) {
              float halfDistance = stepx/2*stepx/2 + stepy/2*stepy/2;
              float halfDistanceSquared = sqrt( halfDistance );
              rayHit.distance += halfDistanceSquared;
              texX = fmod2(hx+stepx/2, tileSize);

              // Give doors slightly lower drawing priority to prevent the
              // wall above drawing its bottom surface later than the door
              rayHit.sortdistance -= 1;
            }
            else {
              canAdd = false;
            }
          }
          rayHit.correctDistance = rayHit.distance * cos(stripAngle);
          rayHit.horizontal = true;
          rayHit.tileX = texX;
          horizontalLineDistance = blockDist;
          if (canAdd) {
            hits.push_back( rayHit );
          }

          bool gaps=needsNextWall(grids,playerZ,tileSize,gridWidth,wallX,wallY,
                                  level);
          // There is an empty space below this wall, or the wall before
          if (gaps) {
            // Add the previous vertical line if any
            if (verticalLineDistance) {
              hits.push_back(verticalWallHit);
              verticalLineDistance = 0; // Make sure not to add it again
            }
            prevGaps = gaps; // for next wall check
          }
          else if (!findAllWalls) {
            break;
          }
        }
      }
      hx += stepx;
      hy += stepy;
    }

    // If no horizontal line was found, but a vertical line was.
    if (!horizontalLineDistance && verticalLineDistance) {
      hits.push_back(verticalWallHit);
    }
  }
}

void Raycaster::findIntersectingThinWalls(std::vector<RayHit>& rayHits,
                                          std::vector<ThinWall*>& thinWalls,
                                          float playerX,
                                          float playerY,
                                          float rayEndX,
                                          float rayEndY)
{
  for (int i=0; i<(int)thinWalls.size(); ++i) {
    ThinWall& thinWall = *thinWalls[i];
    float ix=0, iy=0;
    bool hitFound = Shape::linesIntersect(thinWall.x1, thinWall.y1,
                                          thinWall.x2, thinWall.y2,
                                          playerX, playerY,
                                          rayEndX, rayEndY,
                                          &ix, &iy);
    if (hitFound) {
      RayHit rayHit;
      float distX = playerX - ix;
      float distY = playerY - iy;
      float squaredDistance = distX*distX + distY*distY;
      rayHit.squaredDistance = squaredDistance;
      rayHit.distance = sqrt(squaredDistance);
      rayHit.thinWall = &thinWall;
      rayHit.x = ix;
      rayHit.y = iy;
      rayHits.push_back(rayHit);
    }
  }
}

void Raycaster::raycastThinWalls(std::vector<RayHit>& rayHits,
                                 std::vector<ThinWall*>& thinWalls,
                                 float playerX, float playerY, float playerZ,
                                 float playerRot,
                                 float stripAngle, int stripIdx)
{
  const float TWO_PI = M_PI*2;
  float rayAngle = stripAngle + playerRot;
  while (rayAngle < 0) rayAngle += TWO_PI;
  while (rayAngle >= TWO_PI) rayAngle -= TWO_PI;

  bool right = (rayAngle<TWO_PI*0.25 && rayAngle>=0) || // Quadrant 1
              (rayAngle>TWO_PI*0.75); // Quadrant 4

  float vx = 0;
  if (right) {
    vx = gridWidth * tileSize;
  }
  else {
    vx = 0;
  }

  float vy = playerY + (playerX-vx)*tan(rayAngle);

  std::vector<RayHit> newRayHits;
  std::vector<RayHit*> addedRayHits;
  findIntersectingThinWalls(newRayHits, thinWalls, playerX, playerY, vx, vy);
  for (int i=0; i<(int)newRayHits.size(); ++i) {
    RayHit& rayHit = newRayHits[i];
    ThinWall* thinWall = rayHit.thinWall;
    ThickWall* thickWall = thinWall->thickWall;

    rayHit.wallHeight = thinWall->height;
    rayHit.strip      = stripIdx;
    float dto         = round(thinWall->distanceToOrigin(rayHit.x,rayHit.y));
    rayHit.tileX      = (int)(dto) % this->tileSize;
    rayHit.horizontal = thinWall->horizontal;
    rayHit.wallType   = thinWall->wallType;
    rayHit.rayAngle   = rayAngle;
    if (rayHit.distance) {
      rayHit.correctDistance = rayHit.distance * cos( playerRot - rayAngle );
    }

    // Slope
    if (thinWall->slope) {
      rayHit.wallHeight = thickWall->startHeight + thinWall->slope *
                          thinWall->distanceToOrigin(rayHit.x,rayHit.y);
      if (thickWall->invertedSlope) {
        rayHit.invertedZ = rayHit.wallHeight;
        rayHit.wallHeight = thickWall->tallerHeight - rayHit.wallHeight;
      }
    }

    if (thickWall && thickWall->slope) {
      for (int j=0; j<(int)addedRayHits.size(); ++j) {
        RayHit* addedRayHit = addedRayHits[ j ];
        if (!addedRayHit->sameRayHit(rayHit)) {
          if (addedRayHit->thinWall->thickWall == thickWall) {
            addedRayHit->copySibling(rayHit);
            rayHit.copySibling(*addedRayHit);
          }
        }
      }
    }

    addedRayHits.push_back(&rayHit);
  }

  for (size_t i=0; i<addedRayHits.size(); ++i) {
    rayHits.push_back(*addedRayHits[i]);
  }
}

bool Raycaster::findSiblingAtAngle(RayHit& sibling,
                                   ThinWall& originThinWall,
                                   float originAngle, float playerRot,
                                   int stripIdx, float playerX, float playerY,
                                   float rayStartX, float rayStartY)
{
  if (!originThinWall.thickWall) {
    return false;
  }

  ThickWall* thickWall = originThinWall.thickWall;

  const float TWO_PI = M_PI*2;
  float rayAngle = originAngle; // Note: we don't add player angle
  while (rayAngle < 0) rayAngle += TWO_PI;
  while (rayAngle >= TWO_PI) rayAngle -= TWO_PI;

  bool right = (rayAngle<TWO_PI*0.25 && rayAngle>=0) || // Quadrant 1
              (rayAngle>TWO_PI*0.75); // Quadrant 4

  float vx = 0;
  if (right) {
    vx = gridWidth * tileSize;
  }
  else {
    vx = 0;
  }

  float vy = playerY + (playerX-vx)*tan(rayAngle);

  for (size_t i=0; i<thickWall->thinWalls.size(); ++i) {
    ThinWall* thinWall = &thickWall->thinWalls[i];
    if (thinWall == &originThinWall) {
      continue;
    }
    RayHit rayHit;
    if (Shape::linesIntersect(thinWall->x1, thinWall->y1,
                              thinWall->x2, thinWall->y2,
                              rayStartX, rayStartY,
                              vx, vy, &rayHit.x, &rayHit.y))
    {
      rayHit.thinWall = thinWall;

      float distX = rayStartX - rayHit.x;
      float distY = rayStartY - rayHit.y;
      float squaredDistance = distX*distX + distY*distY;
      rayHit.squaredDistance = squaredDistance;
      rayHit.distance = sqrt(rayHit.squaredDistance);

      rayHit.wallHeight = thinWall->height;
      rayHit.strip      = stripIdx;
      float dto         = round(thinWall->distanceToOrigin(rayHit.x,rayHit.y));
      rayHit.tileX      = (int)(dto) % this->tileSize;
      rayHit.horizontal = thinWall->horizontal;
      rayHit.wallType   = thinWall->wallType;
      rayHit.rayAngle   = rayAngle;
      if (rayHit.distance) {
        rayHit.correctDistance = rayHit.distance * cos( playerRot - rayAngle );
      }

      // Slope
      if (thinWall->slope) {
        rayHit.wallHeight = thickWall->startHeight + thinWall->slope *
                            thinWall->distanceToOrigin(rayHit.x,rayHit.y);
        if (thickWall->invertedSlope) {
          rayHit.invertedZ = rayHit.wallHeight;
          rayHit.wallHeight = thickWall->tallerHeight - rayHit.wallHeight;
        }
      }

      sibling = rayHit;

      return true;
    }
  }
  return false;
}

void Raycaster::raycastSprites(vector<RayHit>& hits,
                               vector< vector<int> >& grids,
                               int gridWidth, int gridHeight, int tileSize,
                               int playerX, int playerY, float playerZ,
                               float playerRot,
                               float stripAngle, int stripIdx,
                               vector<Sprite>* spritesToLookFor)
{
  if (grids.empty()) {
    return;
  }

  float rayAngle = stripAngle + playerRot;
  const float TWO_PI = M_PI*2;
  while (rayAngle < 0) rayAngle += TWO_PI;
  while (rayAngle >= TWO_PI) rayAngle -= TWO_PI;

  bool right = (rayAngle<TWO_PI*0.25 && rayAngle>=0) || // Quadrant 1
              (rayAngle>TWO_PI*0.75); // Quadrant 4
  bool up    = rayAngle<TWO_PI*0.5  && rayAngle>=0; // Quadrant 1 and 2

  std::vector<int>& groundGrid = grids[0];

  int currentTileX = playerX / tileSize;
  int currentTileY = playerY / tileSize;

  //----------------------------------------
  // Check player's current tile for sprites
  //----------------------------------------
  vector<Sprite*> spritesHit;

  vector<Sprite*> spritesFound = findSpritesInCell( *spritesToLookFor,
                                                    currentTileX,
                                                    currentTileY,
                                                    tileSize );
  for (size_t i=0; i<spritesFound.size(); ++i) {
    Sprite* sprite = spritesFound[ i ];
    if (!sprite->rayhit) {
      const float distX = playerX - sprite->x;
      const float distY = playerY - sprite->y;
      const float blockDist = distX*distX + distY*distY;
      if (blockDist) {
        sprite->rayhit = true;
        sprite->distance = sqrt(blockDist);
        spritesHit.push_back(sprite);
      }
    }
  }

  //--------------------------
  // Vertical Lines Checking
  //--------------------------
  RayHit verticalWallHit;

  // Find x coordinate of vertical lines on the right and left
  float vx = 0;
  if (right) {
    vx = floor(playerX/tileSize) * tileSize + tileSize;
  }
  else {
    vx = floor(playerX/tileSize) * tileSize - 1;
  }

  // Calculate y coordinate of those lines
  // lineY = playerY + (playerX-lineX)*tan(ALPHA);
  float vy = playerY + (playerX-vx)*tan(rayAngle);

  // Calculate stepping vector for each line
  float stepx = right ? tileSize : -tileSize;
  float stepy = tileSize * tan(rayAngle);

  // tan() returns positive values in Quadrant 1 and Quadrant 4 but window
  // coordinates need negative coordinates for Y-axis so we reverse
  if ( right ) {
    stepy = -stepy;
  }

  while (vx>=0 && vx<gridWidth*tileSize && vy>=0 && vy<gridHeight*tileSize) {
    int wallY = floor(vy / tileSize);
    int wallX = floor(vx / tileSize);

    // Look for sprites in current cell
    vector<Sprite*> spritesFound = findSpritesInCell( *spritesToLookFor,
                                                      wallX, wallY,
                                                      tileSize );
    for (size_t i=0; i<spritesFound.size(); ++i) {
      Sprite* sprite = spritesFound[ i ];
      if (!sprite->rayhit) {
        const float distX = playerX - sprite->x;
        const float distY = playerY - sprite->y;
        const float blockDist = distX*distX + distY*distY;
        sprite->distance = sqrt(blockDist);
        spritesHit.push_back(sprite);

        RayHit spriteRayHit(vx, vy, rayAngle);
        spriteRayHit.strip = stripIdx;
        if (sprite->distance) {
          spriteRayHit.distance = sprite->distance;
          spriteRayHit.correctDistance = spriteRayHit.distance *
                                          cos(stripAngle);
        }
        spriteRayHit.wallType = 0;
        spriteRayHit.sprite = sprite;
        spriteRayHit.level = sprite->level;
        spriteRayHit.distance = sprite->distance;
        sprite->rayhit = true;
        hits.push_back( spriteRayHit );
      }
    }

    vx += stepx;
    vy += stepy;
  }

  //--------------------------
  // Horizontal Lines Checking
  //--------------------------

  // Find y coordinate of horizontal lines on the right and left
  float hy = 0;
  if (up) {
    hy = floor(playerY/tileSize) * tileSize - 1;
  }
  else {
    hy = floor(playerY/tileSize) * tileSize + tileSize;
  }

  // Calculation x coordinate of horizontal line
  // lineX = playerX + (playerY-lineY)/tan(ALPHA);
  float hx = playerX + (playerY-hy) / tan(rayAngle);
  stepy = up ? -tileSize : tileSize;
  stepx = tileSize / tan(rayAngle);

  // tan() returns stepx as positive in quadrant 3 and negative in quadrant 4
  // This is the opposite of window coordinates so we need to reverse when
  // angle is facing down
  if ( !up ) {
    stepx = -stepx;
  }

  while (hx>=0 && hx<gridWidth*tileSize && hy>=0 && hy<gridHeight*tileSize) {
    int wallY = floor(hy / tileSize);
    int wallX = floor(hx / tileSize);

    // Look for sprites in current cell
    vector<Sprite*> spritesFound = findSpritesInCell( *spritesToLookFor,
                                                      wallX, wallY,
                                                      tileSize );
    for (size_t i=0; i<spritesFound.size(); ++i) {
      Sprite* sprite = spritesFound[ i ];
      if (!sprite->rayhit) {
        const float distX = playerX - sprite->x;
        const float distY = playerY - sprite->y;
        const float blockDist = distX*distX + distY*distY;
        sprite->distance = sqrt(blockDist);
        spritesHit.push_back(sprite);
        RayHit spriteRayHit(hx, hy, rayAngle);
        spriteRayHit.strip = stripIdx;
        if (sprite->distance) {
          spriteRayHit.distance = sprite->distance;
          spriteRayHit.correctDistance = spriteRayHit.distance *
                                          cos(stripAngle);
        }
        spriteRayHit.wallType = 0;
        spriteRayHit.sprite = sprite;
        spriteRayHit.level = sprite->level;
        spriteRayHit.distance = sprite->distance;
        sprite->rayhit = true;
        hits.push_back( spriteRayHit );
      }
    }

    hx += stepx;
    hy += stepy;
  }
}
