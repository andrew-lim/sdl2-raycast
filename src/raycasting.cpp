#include "raycasting.h"
#include <cmath>
#include <algorithm>
using namespace std;
using namespace al::raycasting;

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
                        bool lookForMultipleWalls,
                        std::vector<Sprite>* spritesToLookFor,
                        std::vector<RayHit>* wallsToIgnore )
{
  Raycaster::raycast(rayHits, this->grid,
                     this->gridWidth, this->gridHeight, this->tileSize,
                     playerX, playerY, playerRot,
                     stripAngle, stripIdx,
                     lookForMultipleWalls,
                     spritesToLookFor, wallsToIgnore);
}

void Raycaster::raycast(vector<RayHit>& hits, vector<int>& grid,
                        int gridWidth, int gridHeight, int tileSize,
                        int playerX, int playerY, float playerRot,
                        float stripAngle, int stripIdx,
                        bool lookForMultipleWalls,
                        vector<Sprite>* spritesToLookFor,
                        vector<RayHit>* wallsToIgnore )
{
  float rayAngle = stripAngle + playerRot;
  const float TWO_PI = M_PI*2;
  while (rayAngle < 0) rayAngle += TWO_PI;
  while (rayAngle >= TWO_PI) rayAngle -= TWO_PI;

  bool right = (rayAngle<TWO_PI*0.25 && rayAngle>=0) || // Quadrant 1
              (rayAngle>TWO_PI*0.75); // Quadrant 4
  bool up    = rayAngle<TWO_PI*0.5  && rayAngle>=0; // Quadrant 1 and 2

  //----------------------------------------
  // Check player's current tile for sprites
  //----------------------------------------
  vector<Sprite*> spritesHit;
  int currentTileX = playerX / tileSize;
  int currentTileY = playerY / tileSize;
  if (spritesToLookFor) {
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
            spriteRayHit.straightDistance = spriteRayHit.distance *
                                            cos(stripAngle);
          }
          spriteRayHit.wallType = 0;
          spriteRayHit.sprite = sprite;
          spriteRayHit.distance = sprite->distance;
          hits.push_back( spriteRayHit );
        }
      }
    }

    // Skip walls requested to be ignored
    if (wallsToIgnore && isWallInRayHits(*wallsToIgnore, wallX, wallY)) {
    }

    // Check if current cell is a wall
    else if (grid[wallOffset] > 0) {
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
      if (blockDist) {
        rayHit.distance = sqrt(blockDist);
        rayHit.straightDistance = rayHit.distance * cos(stripAngle);
      }
      rayHit.horizontal = false;
      rayHit.tileX = texX;

      if (false == lookForMultipleWalls) {
        verticalWallHit = rayHit;
        verticalLineDistance = blockDist;
        break;
      }
      hits.push_back( rayHit );
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
            spriteRayHit.straightDistance = spriteRayHit.distance *
                                            cos(stripAngle);
          }
          spriteRayHit.wallType = 0;
          spriteRayHit.sprite = sprite;
          spriteRayHit.distance = sprite->distance;
          hits.push_back( spriteRayHit );
        }
      }
    }

    if (wallsToIgnore && isWallInRayHits(*wallsToIgnore, wallX, wallY)) {
    }

    // Check if current cell is a wall
    else if (grid[wallOffset] > 0) {
      const float distX = playerX - hx;
      const float distY = playerY - hy;
      const float blockDist = distX*distX + distY*distY;

      // If horizontal distance is less than vertical line distance, stop
      if (false==lookForMultipleWalls) {
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
      if (blockDist) {
        rayHit.distance = sqrt(blockDist);
        rayHit.straightDistance = rayHit.distance * cos(stripAngle);
      }
      rayHit.horizontal = true;
      rayHit.tileX = texX;
      horizontalLineDistance = blockDist;
      hits.push_back( rayHit );

      if (false == lookForMultipleWalls) {
        break;
      }
    }
    hx += stepx;
    hy += stepy;
  }

  // Choose the shortest distance if looking for nearest wall
  if (false==lookForMultipleWalls) {
    if (!horizontalLineDistance && verticalLineDistance) {
      hits.push_back(verticalWallHit);
    }
  }
}
