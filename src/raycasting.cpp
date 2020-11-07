#include "raycasting.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cassert>
using namespace std;
using namespace al::raycasting;

#define USE_FMOD 1

static float fmod2( float a, int b ) {
  #if USE_FMOD == 1
  return fmod(a,b);
  #else
  return (int)a % b; // faster but causes texture seams
  #endif
}

bool RayHit::operator<(const RayHit& b) const {
  // If same level, draw furthest strip first
//  if (level == b.level) {
    return distance > b.distance;
//  }
//  return level > b.level; // draw high levels first

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
*/
float Raycaster::stripAngle(float screenX, float screenDistance) {
    float rayViewDist = sqrt(screenX*screenX +screenDistance*screenDistance);
    return asin(screenX / rayViewDist);
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

    if (spritesToLookFor && level==0) {
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
      if (spritesToLookFor && level==0) {
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
        if (blockDist) {
          rayHit.distance = sqrt(blockDist);
          // If a door was hit, move ray halfway inside
          if (isVerticalDoor(grid[wallOffset])) {
            float halfDistance = stepx/2*stepx/2 + stepy/2*stepy/2;
            float halfDistanceSquared = sqrt( halfDistance );
            rayHit.distance += halfDistanceSquared;
            texX = fmod2(vy+stepy/2, tileSize);
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
          hits.push_back( rayHit );
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
      if (spritesToLookFor && level==0) {
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
        const float blockDist = distX*distX + distY*distY;

        // If vertical distance is less than horizontal line distance, stop
        if (verticalLineDistance>0 && verticalLineDistance<blockDist) {
          // Unless there was some space below previous wall
          if (!prevGaps) {
            break;
          }
        }

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
        if (blockDist) {
          rayHit.distance = sqrt(blockDist);
          // If a door was hit, move ray halfway inside
          if (isHorizontalDoor(grid[wallOffset])) {
            float halfDistance = stepx/2*stepx/2 + stepy/2*stepy/2;
            float halfDistanceSquared = sqrt( halfDistance );
            rayHit.distance += halfDistanceSquared;
            texX = fmod2(hx+stepx/2, tileSize);
          }
          rayHit.correctDistance = rayHit.distance * cos(stripAngle);
          rayHit.horizontal = true;
          rayHit.tileX = texX;
          horizontalLineDistance = blockDist;
          hits.push_back( rayHit );

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
