#include "raycasting.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cassert>
using namespace std;
using namespace al::raycasting;

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
    if (cellX == (sprite->x/tileSize) && cellY == (sprite->y/tileSize) ) {
      spritesFound.push_back(sprite);
    }
  }
  return spritesFound;
}

void Raycaster::raycast(std::vector<RayHit>& rayHits,
                        int playerX, int playerY,
                        float playerRot, float stripAngle, int stripIdx,
                        std::vector<Sprite>* spritesToLookFor)
{
  Raycaster::raycast(rayHits, this->grids,
                     this->gridWidth, this->gridHeight, this->tileSize,
                     playerX, playerY, playerRot,
                     stripAngle, stripIdx,
                     spritesToLookFor);
}

void Raycaster::raycast(vector<RayHit>& hits,
                        vector< vector<int> >& grids,
                        int gridWidth, int gridHeight, int tileSize,
                        int playerX, int playerY, float playerRot,
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
  
  for (int level=0; level<(int)grids.size(); ++level) {
    vector<int>& grid = grids[level];

    //----------------------------------------
    // Check player's current tile for sprites
    //----------------------------------------
    vector<Sprite*> spritesHit;
    int currentTileX = playerX / tileSize;
    int currentTileY = playerY / tileSize;
    if (spritesToLookFor && level==0) {
      vector<Sprite*> spritesFound = findSpritesInCell( *spritesToLookFor,
                                                        currentTileX,
                                                        currentTileY,
                                                        tileSize );
      for (size_t i=0; i<spritesFound.size(); ++i) {
        Sprite* sprite = spritesFound[ i ];
        bool addedAlready = std::find(spritesHit.begin(), spritesHit.end(),
                                      sprite) != spritesHit.end();
        if (!addedAlready) {
          const float distX = playerX - sprite->x;
          const float distY = playerY - sprite->y;
          const float blockDist = distX*distX + distY*distY;
          sprite->distance = sqrt(blockDist);
          spritesHit.push_back(sprite);
        }
      }
    }
    
    // Check if the player is standing under a wall, and add that wall
    // Figure this out by trial and error
    // NO IDEA WHY THIS WORKS BUT IT DOES
    int playerOffset = currentTileX + currentTileY*gridWidth;
    if (level>0 && grid[playerOffset] > 0) {
      const float distX = 1;
      const float distY = 1;
      const float blockDist = distX*distX + distY*distY;
      float texX = fmod(playerY, tileSize);
      texX = right ? texX : tileSize - texX; // Facing left, flip image

      RayHit rayHit(playerX, playerY, rayAngle);
      rayHit.strip = stripIdx;
      rayHit.wallType = grid[playerOffset];
      rayHit.wallX = currentTileX;
      rayHit.wallY = currentTileY;
      rayHit.level = level;
      if (blockDist) {
        rayHit.distance = sqrt(blockDist);
        rayHit.correctDistance = rayHit.distance * cos(stripAngle);
      }
      rayHit.horizontal = false;
      rayHit.tileX = texX;
      hits.push_back( rayHit );
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

    bool aboveEmptyBlock = level > 0 && groundGrid[playerOffset]==0;
    int lastWallFoundIndex = -1;
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
          bool addedAlready = std::find(spritesHit.begin(), spritesHit.end(),
                                        sprite) != spritesHit.end();
          if (!addedAlready) {
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
            spriteRayHit.distance = sprite->distance;
            hits.push_back( spriteRayHit );
          }
        }
      }

      // Check if current cell is a wall
      if (grid[wallOffset] > 0) {
        const float distX = playerX - vx;
        const float distY = playerY - vy;
        const float blockDist = distX*distX + distY*distY;
        float texX = fmod(vy, tileSize);
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
          rayHit.correctDistance = rayHit.distance * cos(stripAngle);
        }
        rayHit.horizontal = false;
        rayHit.tileX = texX;

        // There is an empty space below this wall
        aboveEmptyBlock = level>0&&(aboveEmptyBlock||groundGrid[wallOffset]==0);
        if (aboveEmptyBlock) {
          aboveEmptyBlock = groundGrid[wallOffset]==0;
        }
        else {
          verticalWallHit = rayHit;
          verticalLineDistance = blockDist;
          break;
        }
        hits.push_back( rayHit );
        lastWallFoundIndex = hits.size() - 1;
      }

      vx += stepx;
      vy += stepy;
    }
    if (lastWallFoundIndex != -1) {
      hits[lastWallFoundIndex].furthest = true;
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

    aboveEmptyBlock = level > 0 && groundGrid[playerOffset]==0;
    lastWallFoundIndex = -1;
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
          bool addedAlready = std::find(spritesHit.begin(), spritesHit.end(),
                                        sprite) != spritesHit.end();
          if (!addedAlready) {
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
            spriteRayHit.distance = sprite->distance;
            hits.push_back( spriteRayHit );
          }
        }
      }

      // Check if current cell is a wall
      if (grid[wallOffset] > 0) {
        const float distX = playerX - hx;
        const float distY = playerY - hy;
        const float blockDist = distX*distX + distY*distY;
        // There is an empty space below this wall
        aboveEmptyBlock = level>0&&(aboveEmptyBlock||groundGrid[wallOffset]==0);
        if (aboveEmptyBlock) {
          aboveEmptyBlock = groundGrid[wallOffset]==0;
        }
        // If vertical distance is less than horizontal line distance, stop
        else {
          if (verticalLineDistance>0 && verticalLineDistance<blockDist) {
            break;
          }
        }

        float texX =  fmod(hx, tileSize);
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
          rayHit.correctDistance = rayHit.distance * cos(stripAngle);
        }
        rayHit.horizontal = true;
        rayHit.tileX = texX;
        horizontalLineDistance = blockDist;
        hits.push_back( rayHit );
        lastWallFoundIndex = hits.size() - 1;

        // There is an empty space below this wall
        aboveEmptyBlock = level>0&&(aboveEmptyBlock||groundGrid[wallOffset]==0);
        if (aboveEmptyBlock) {
          aboveEmptyBlock = groundGrid[wallOffset]==0;
        }
        else {
          break;
        }
      }
      hx += stepx;
      hy += stepy;
    }
    if (lastWallFoundIndex != -1) {
      hits[lastWallFoundIndex].furthest = true;
    }

    // Choose the shortest distance if looking for nearest wall
    if (!horizontalLineDistance && verticalLineDistance) {
      verticalWallHit.furthest = true;
      hits.push_back(verticalWallHit);
    }
  }
}
