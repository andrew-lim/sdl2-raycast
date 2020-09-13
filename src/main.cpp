/*
SDL2 raycasting example in the style of Wolfenstein 3D.
Author: Andrew Lim Chong Liang
https://github.com/andrew-lim

Linker settings
-lmingw32 -lSDL2main -lSDL2
*/
#include <SDL2/SDL.h>
#include <cstdio>
#include <map>
#include <cmath>
#include <vector>

// Length of a wall or cell in game units.
// In the original Wolfenstein 3D it was 8 feet (1 foot = 16 units)
const int TILE_SIZE = 128;

const int TEXTURE_SIZE = 128; // length of wall textures in pixels
const int DISPLAY_WIDTH = 800;
const int DISPLAY_HEIGHT = 600;
const int MINIMAP_SCALE = 6;
const int MINIMAP_Y = 0; // position of minimap from top of screen
const int UPDATE_INTERVAL = 1000/60;
const int STRIP_WIDTH = 1;
const int FOV_DEGREES = 60;
const double FOV_RADIANS = (double)FOV_DEGREES * M_PI / 180; // FOV in radians
const int RAYCOUNT = DISPLAY_WIDTH / STRIP_WIDTH; //  Math.ceil(screenWidth / STRIP_WIDTH);
const double viewDist = (DISPLAY_WIDTH/2) / tan((FOV_RADIANS / 2));
const double TWO_PI = M_PI*2;

// Holds information about a wall hit from a single ray
struct WallHit {
  double x, y;      // wall position in game units
  int wallX, wallY; // wall position in column, row tile units
  int wallType;     // type of wall hit
  int strip;        // strip on screen for this wall
  double tileX;     // x-coordinate within tile, used for calculating texture x
  double distance;  // distance to wall
  bool horizontal;  // true if wall was hit on the bottom or top
  double rayAngle;  // angle used for calculation
  WallHit() {
    x = y = wallType = strip = wallX = wallY = tileX = distance = 0;
    horizontal = false;
    rayAngle = 0;
  }
};

class Sprite {
public:
    int x, y ;
    int dir;          // the direction that the player is turning, either -1 for left or 1 for right.
    double rot;       // the current angle of rotation. Counterclockwise is positive.
    int speed;        // is the playing moving forward (speed = 1) or backwards (speed = -1).
    int moveSpeed;    // how far (in map units) does the player move each step/update
    double  rotSpeed; // how much does the player rotate each step/update (in radians)
    Sprite() :x(0), y(0), dir(0), rot(0), speed(0) {
      moveSpeed = TILE_SIZE / 8;
      rotSpeed = 4 * M_PI/180;
    }
} ;

const int MAP_WIDTH = 32;
const int MAP_HEIGHT = 24;

static int g_map[MAP_HEIGHT][MAP_WIDTH] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,3,0,3,0,0,1,1,1,2,1,1,1,1,1,2,1,1,1,2,1,0,0,0,0,0,0,0,0,1},
  {1,0,0,3,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,1,1,1,1,1},
  {1,0,0,3,0,3,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
  {1,0,0,3,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,2},
  {1,0,0,3,0,3,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,3,3,3,0,0,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
  {1,0,0,0,0,0,0,0,0,3,3,3,0,0,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,3,3,3,0,0,3,3,3,0,0,0,0,0,0,0,0,0,3,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,3,3,3,0,0,3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,4,0,0,4,2,0,2,2,2,2,2,2,2,2,0,2,4,4,0,0,4,0,0,0,0,0,0,0,1},
  {1,0,0,4,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,4,0,0,0,0,0,0,0,1},
  {1,0,0,4,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,4,0,0,0,0,0,0,0,1},
  {1,0,0,4,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,4,0,0,0,0,0,0,0,1},
  {1,0,0,4,3,3,4,2,2,2,2,2,2,2,2,2,2,2,2,2,4,3,3,4,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

class Game {
public:
    Game();
    ~Game();
    void start();
    void stop() ;
    void draw();
    void fillRect(SDL_Rect* rc, int r, int g, int b );
    void drawLine(int startX, int startY, int endX, int endY, int r, int g, int b, int alpha=SDL_ALPHA_OPAQUE );
    void fpsChanged( int fps );
    void onQuit();
    void onKeyDown( SDL_Event* event );
    void onKeyUp( SDL_Event* event );
    void run();
    void update();
    void drawMiniMap();
    void updatePlayer();
    void drawFloorAndCeiling();
    void drawPlayer();
    void drawRay(double rayX, double rayY);
    void drawRays(std::vector<WallHit>& wallHits);
    void drawWallStrip(double textureX, double textureY, int stripIdx, int wallScreenHeight);
    void drawWalls(std::vector<WallHit>& wallHits);
    WallHit calculateWallHit(int playerX, int playerY, double rayAngle, int stripIdx);
    void raycast(std::vector<WallHit>& wallHits);
    bool wallHit(int wallX, int wallY);
private:
    std::map<int,int> keys; // No SDLK_LAST. SDL2 migration guide suggests std::map
    int frameSkip ;
    int running ;
    SDL_Window* window;
    SDL_Renderer* renderer;
    Sprite player;
    SDL_Surface* wallBitmap;
    SDL_Texture* wallTexture;
    SDL_Surface* floorBitmap;
    SDL_Texture* floorTexture;
    SDL_Surface* gunBitmap;
    SDL_Texture* gunTexture;
    bool drawMiniMapOn;
};

Game::Game()
:frameSkip(0), running(0), window(NULL), renderer(NULL) {
  player.x = 16 * TILE_SIZE;
  player.y = 10 * TILE_SIZE;
  drawMiniMapOn = true;
}

Game::~Game() {
    this->stop();
}

void Game::start() {
  printf("Press M to toggle minimap\n");
  printf("Display=(%d,%d)\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
  printf("Map size=(%d,%d)\n", MAP_WIDTH, MAP_HEIGHT);
  printf("Wall/Tile Size=%d\n", TILE_SIZE);
  printf("Texture Size=%d\n", TEXTURE_SIZE);
  int flags = SDL_WINDOW_SHOWN ;
  if (SDL_Init(SDL_INIT_EVERYTHING)) {
      return ;
  }
  if (SDL_CreateWindowAndRenderer(DISPLAY_WIDTH, DISPLAY_HEIGHT, flags, &window, &renderer)) {
      return;
  }
  SDL_SetWindowTitle(window, "SDL2 Raycasting - https://github.com/andrew-lim");
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  wallBitmap = SDL_LoadBMP("..\\res\\walls4.bmp");
  if (!wallBitmap) {
    printf("Error loading walls4.bmp");
    return;
  }
  wallTexture = SDL_CreateTextureFromSurface(renderer, wallBitmap);
  if (!wallTexture) {
    printf("Error creating texture from walls");
    return;
  }
  floorBitmap =  SDL_LoadBMP("..\\res\\floor.bmp");
  floorTexture = SDL_CreateTextureFromSurface(renderer, floorBitmap);
  gunBitmap = SDL_LoadBMP("..\\res\\SMG.bmp");
  SDL_SetColorKey( gunBitmap, true, SDL_MapRGB(gunBitmap->format, 152, 0, 136) );
  gunTexture = SDL_CreateTextureFromSurface(renderer, gunBitmap);
  this->running = 1 ;
  run();
}

void Game::draw() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 1, 65, 65, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    drawFloorAndCeiling();
    static std::vector<WallHit> wallHits;
    wallHits.clear();
    raycast(wallHits);
    drawWalls(wallHits);
    if (drawMiniMapOn) {
      drawMiniMap();
      drawRays(wallHits);
      drawPlayer();
    }
    
    // Draw Gun
    double gunScale = DISPLAY_WIDTH / 320;
    SDL_Rect dstRect;
    dstRect.w = gunBitmap->w * gunScale;
    dstRect.h = gunBitmap->h * gunScale;
    dstRect.x = (DISPLAY_WIDTH - dstRect.w) / 2;
    dstRect.y = DISPLAY_HEIGHT - dstRect.h;
    SDL_RenderCopy(renderer, gunTexture, NULL, &dstRect);
    
    SDL_RenderPresent(renderer);
}

void Game::stop() {
    if (NULL != renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (NULL != window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    SDL_Quit() ;
}

void Game::fillRect(SDL_Rect* rc, int r, int g, int b ) {
    SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, rc);
}

void Game::drawLine(int startX, int startY, int endX, int endY, int r, int g, int b, int alpha ) {
  
  SDL_SetRenderDrawColor(renderer, r, g, b, alpha);
  SDL_RenderDrawLine(renderer, startX, startY, endX, endY);
}

void Game::fpsChanged( int fps ) {
    char szFps[ 128 ] ;
    sprintf( szFps, "%s - %d FPS", "SDL2 Raycasting - https://github.com/andrew-lim", fps );
    SDL_SetWindowTitle(window, szFps);
}

void Game::onQuit() {
    running = 0 ;
}

void Game::run() {
    int past = SDL_GetTicks();
    int now = past, pastFps = past ;
    int fps = 0, framesSkipped = 0 ;
    SDL_Event event ;
    while ( running ) {
        int timeElapsed = 0 ;
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:    onQuit();            break;
                case SDL_KEYDOWN: onKeyDown( &event ); break ;
                case SDL_KEYUP:   onKeyUp( &event );   break ;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEMOTION:
                    break ;
            }
        }
        // update/draw
        timeElapsed = (now=SDL_GetTicks()) - past ;
        if ( timeElapsed >= UPDATE_INTERVAL  ) {
            past = now ;
            update();
            if ( framesSkipped++ >= frameSkip ) {
                draw();
                ++fps ;
                framesSkipped = 0 ;
            }
        }
        // fps
        if ( now - pastFps >= 1000 ) {
            pastFps = now ;
            fpsChanged( fps );
            fps = 0 ;
        }
        // sleep?
        SDL_Delay( 1 );
    }
}

void Game::update() {
    if (keys[SDLK_UP]) { // up, move player forward, ie. increase speed
      player.speed = 1;
    }
    else if (keys[SDLK_DOWN]) { // down, move player backward, set negative speed
      player.speed = -1;
    }
    else {
      player.speed = 0; // stop moving
    }
    
    if (keys[SDLK_LEFT]) { // left, rotate player left
      player.dir = -1;
    }
    else if (keys[SDLK_RIGHT]) { // right, rotate player right
      player.dir = 1;
    }
    else {
      player.dir = 0; // stop rotating
    }
    
    updatePlayer();
}

void Game::drawMiniMap() {
  SDL_Rect miniMapRect;
  miniMapRect.x = 0; 
  miniMapRect.y = MINIMAP_Y ;
  miniMapRect.w = MAP_WIDTH * MINIMAP_SCALE;
  miniMapRect.h = MAP_HEIGHT * MINIMAP_SCALE;
  fillRect(&miniMapRect, 0, 0, 0);
  
  for (int y=0; y<MAP_HEIGHT; ++y) {
    for (int x=0; x<MAP_WIDTH; ++x) {
      if (g_map[y][x]>0) {
        SDL_Rect rc;
        rc.x = x * MINIMAP_SCALE;
        rc.y = y * MINIMAP_SCALE;
        rc.y += MINIMAP_Y;
        rc.w = MINIMAP_SCALE;
        rc.h = MINIMAP_SCALE;
        fillRect(&rc, 200, 200, 200);
      }
    }
  }
}

void Game::updatePlayer() {
  double moveStep = player.speed * player.moveSpeed;
  player.rot += -player.dir * player.rotSpeed;
  int newX = player.x +  cos(player.rot) * moveStep;
  int newY = player.y + -sin(player.rot) * moveStep;
  int wallX = newX / TILE_SIZE;
  int wallY = newY / TILE_SIZE;
  if (wallHit(wallX, wallY)) {
    return;
  }
  player.x = newX;
  player.y = newY;
}

void Game::drawFloorAndCeiling() {
  SDL_Rect srcrect;
  srcrect.x = 0;
  srcrect.y = 0;
  srcrect.w = 640;
  srcrect.h = 200;
  
  SDL_Rect ceilingRect;
  ceilingRect.x = 0;
  ceilingRect.y = 0;
  ceilingRect.w = DISPLAY_WIDTH;
  ceilingRect.h = DISPLAY_HEIGHT/2;
  fillRect(&ceilingRect, 56, 56, 56 );
  
  SDL_Rect floorRect;
  floorRect.x = 0;
  floorRect.y = DISPLAY_HEIGHT/2;
  floorRect.w = DISPLAY_WIDTH;
  floorRect.h = DISPLAY_HEIGHT/2;
//  fillRect(&floorRect, 112, 112, 112 );
  SDL_RenderCopy(renderer, floorTexture, &srcrect, &floorRect);
}

void Game::drawPlayer() {
  SDL_Rect playerRect;
  
  double playerX =  (double)player.x / (MAP_WIDTH*TILE_SIZE) * 100;
  playerX = playerX/100 * MINIMAP_SCALE * MAP_WIDTH;
  
  double playerY =  (double)player.y / (MAP_HEIGHT*TILE_SIZE) * 100;
  playerY = playerY/100 * MINIMAP_SCALE * MAP_HEIGHT;

  playerRect.x = playerX - 2 ;
  playerRect.y = playerY - 2 + MINIMAP_Y;
  playerRect.w = 5;
  playerRect.h = 5;
  fillRect(&playerRect, 255, 0, 0);
  
  double lineEndX = playerX +  cos(player.rot) * 4 * MINIMAP_SCALE;
  double lineEndY = playerY + -sin(player.rot) * 4 * MINIMAP_SCALE;
  
  drawLine(playerX, playerY + MINIMAP_Y, lineEndX, lineEndY + MINIMAP_Y, 255, 0, 0);
}

void Game::drawRay(double rayX, double rayY) {
  int startY = DISPLAY_HEIGHT + 44 /*padding*/;
  
  double playerX =  (double)player.x / (MAP_WIDTH*TILE_SIZE) * 100;
  playerX = playerX/100 * MINIMAP_SCALE * MAP_WIDTH;
  
  double playerY =  (double)player.y / (MAP_HEIGHT*TILE_SIZE) * 100;
  playerY = playerY/100 * MINIMAP_SCALE * MAP_HEIGHT;

  rayX = rayX / (MAP_WIDTH*TILE_SIZE) * 100.0;
  rayX = rayX/100.0 * MINIMAP_SCALE * MAP_WIDTH;
  rayY = rayY / (MAP_HEIGHT*TILE_SIZE) * 100.0;
  rayY = rayY/100.0 * MINIMAP_SCALE * MAP_HEIGHT;

  drawLine(playerX, playerY + MINIMAP_Y, rayX, rayY + MINIMAP_Y, 0, 100, 0, 0.3*255);
}

void Game::drawRays(std::vector<WallHit>& wallHits) {
   for (int i=0; i<wallHits.size(); ++i) {
    WallHit wallHit = wallHits[i];
    drawRay(wallHit.x, wallHit.y);
  }
}

void Game::drawWalls(std::vector<WallHit>& wallHits) {
  for (int i=0; i<wallHits.size(); ++i) {
    WallHit wallHit = wallHits[i];

    // distorted_dist = correct_dist / cos(relative_angle_of_ray)
    double dist = wallHit.distance * cos( player.rot - wallHit.rayAngle );

    // "real" wall height in the game world is 1 unit, the distance from the player to the screen is viewDist,
    // thus the height on the screen is equal to wall_height_real * viewDist / dist
    int wallScreenHeight = round(viewDist / dist*TILE_SIZE);

    double sx = (wallHit.horizontal?TEXTURE_SIZE:0) + (wallHit.tileX/TILE_SIZE*TEXTURE_SIZE);
    double sy = TEXTURE_SIZE * (wallHit.wallType-1);
    drawWallStrip(sx, sy, wallHit.strip, wallScreenHeight);
  }
}

void Game::drawWallStrip(double textureX, double textureY, int stripIdx, int wallScreenHeight)
{
  double sx = textureX;
  double sy = textureY;
  double swidth = 1;
  double sheight = TEXTURE_SIZE;
  double imgx = stripIdx * STRIP_WIDTH;
  double imgy = (DISPLAY_HEIGHT - wallScreenHeight)/2;
  double imgw = STRIP_WIDTH;
  double imgh = wallScreenHeight;

  SDL_Rect srcrect;
  srcrect.x = sx;
  srcrect.y = sy;
  srcrect.w = swidth;
  srcrect.h = sheight;

  SDL_Rect dstrect;
  dstrect.x = imgx;
  dstrect.y = imgy;
  dstrect.w = imgw;
  dstrect.h = imgh;

  SDL_RenderCopy(renderer, wallTexture, &srcrect, &dstrect);
}

void Game::raycast(std::vector<WallHit>& wallHits) {
  int stripIdx = 0;
  for (int i=0;i<RAYCOUNT;i++, stripIdx++) {
    // where on the screen does ray go through?
    double rayScreenPos = (RAYCOUNT/2 - i) * STRIP_WIDTH;

    // the distance from the viewer to the point on the screen, simply Pythagoras.
    double rayViewDist = sqrt(rayScreenPos*rayScreenPos + viewDist*viewDist);

    // the angle of the ray, relative to the viewing direction.
    // right triangle: a = sin(A) * c
    double rayAngle = asin(rayScreenPos / rayViewDist);

    WallHit wallHit = calculateWallHit(player.x, player.y, player.rot + rayAngle, stripIdx);
    if ( wallHit.distance ) {
      wallHits.push_back(wallHit);
    }
  }
}

WallHit Game::calculateWallHit(int playerX, int playerY,
                               double rayAngle, int stripIdx)
{
  while (rayAngle < 0) rayAngle += TWO_PI;
  while (rayAngle >= TWO_PI) rayAngle -= TWO_PI;

  bool right = (rayAngle<TWO_PI*0.25 && rayAngle>=0) || // Quadrant 1
              (rayAngle>TWO_PI*0.75); // Quadrant 4
  bool up    = rayAngle<TWO_PI*0.5  && rayAngle>=0; // Quadrant 1 and 2

  int wallType = 0;
  double tileX; // the x-coord on the texture of the block, ie. what part of the texture are we going to render

  double dist = 0; // the distance to the block we hit
  double xHit = 0; // the x and y coord of where the ray hit the block
  double yHit = 0;

  int wallHitX = 0;
  int wallHitY = 0;

  bool wallHorizontal = false;

  //--------------------------
  //
  // Vertical Lines Checking
  //
  //--------------------------

  // Find x coordinate of vertical lines on the right and left
  double vx = 0;
  if (right) {
    vx = floor(playerX/TILE_SIZE) * TILE_SIZE + TILE_SIZE;
  }
  else {
    vx = floor(playerX/TILE_SIZE) * TILE_SIZE - 1;
  }

  // Calculate y coordinate of those lines
  // lineY = playerY + (playerX-lineX)*tan(ALPHA);
  double vy = playerY + (playerX-vx)*tan(rayAngle);

  // Calculate stepping vector for each line
  double stepx = right ? TILE_SIZE : -TILE_SIZE;
  double stepy = TILE_SIZE * tan(rayAngle);

  // tan() returns positive values in Quadrant 1 and Quadrant 4
  // But window coordinates need negative coordinates for Y-axis so we reverse them
  if ( right ) {
    stepy = -stepy;
  }

  while (vx >= 0 && vx < MAP_WIDTH*TILE_SIZE && vy >= 0 && vy < MAP_HEIGHT*TILE_SIZE) {
    int wallY = floor(vy / TILE_SIZE);
    int wallX = floor(vx / TILE_SIZE);
    if (g_map[wallY][wallX] > 0) {
      double distX = playerX - vx;
      double distY = playerY - vy;
      double blockDist = distX*distX + distY*distY;
      dist = blockDist;
      xHit = vx;
      yHit = vy;
      wallHitX = wallX;
      wallHitY = wallY;
      wallType = g_map[wallY][wallX];
      tileX = fmod(vy, TILE_SIZE);

      // Facing left, flip image
      if (!right) {
        tileX = TILE_SIZE - tileX;
      }
      break;
    }
    vx += stepx;
    vy += stepy;
  }

  //--------------------------
  //
  // Horizontal Lines Checking
  //
  //--------------------------

  // Find y coordinate of horizontal lines on the right and left
  double hy = 0;
  if (up) {
    hy = floor(playerY/TILE_SIZE) * TILE_SIZE - 1;
  }
  else {
    hy = floor(playerY/TILE_SIZE) * TILE_SIZE + TILE_SIZE;
  }

  // Calculation x coordinate of horizontal line
  // lineX = playerX + (playerY-lineY)/tan(ALPHA);
  double hx = playerX + (playerY-hy) / tan(rayAngle);
  stepy = up ? -TILE_SIZE : TILE_SIZE;
  stepx = TILE_SIZE / tan(rayAngle);

  // tan() returns stepx as positive in quadrant 3 and negative in quadrant 4
  // This is the opposite of window coordinates so we need to reverse when angle is facing down
  if ( !up ) {
    stepx = -stepx;
  }

  while (hx >= 0 && hx < MAP_WIDTH*TILE_SIZE && hy >= 0 && hy < MAP_HEIGHT*TILE_SIZE) {
    int wallY = floor(hy / TILE_SIZE);
    int wallX = floor(hx / TILE_SIZE);

    if (g_map[wallY][wallX] > 0) {
      double distX = playerX - hx;
      double distY = playerY - hy;
      double blockDist = distX*distX + distY*distY;
      if (!dist || blockDist < dist) {
        dist = blockDist;
        xHit = hx;
        yHit = hy;
        wallHitX = wallX;
        wallHitY = wallY;
        wallType = g_map[wallY][wallX];
        tileX =  fmod(hx, TILE_SIZE);
        wallHorizontal = true;

        // Facing down, flip image
        if (!up) {
          tileX = TILE_SIZE - tileX;
        }
      }
      break;
    }
    hx += stepx;
    hy += stepy;
  }

  WallHit wallHit;
  wallHit.strip = stripIdx;
  wallHit.x = xHit;
  wallHit.y = yHit;
  wallHit.wallType = wallType;
  wallHit.wallX = wallHitX;
  wallHit.wallY = wallHitY;
  wallHit.distance = sqrt(dist);
  wallHit.horizontal = wallHorizontal;
  wallHit.tileX = tileX;
  wallHit.rayAngle = rayAngle;
  return wallHit;
}
bool Game::wallHit(int x, int y) {
  // first make sure that we cannot move outside the boundaries of the level
  if (y < 0 || y >= MAP_HEIGHT || x < 0 || x >= MAP_WIDTH)
    return true;

  // return true if the map block is not 0, ie. if there is a blocking wall.
  return (g_map[y][x] != 0);
}

void Game::onKeyDown( SDL_Event* evt ) {
    keys[ evt->key.keysym.sym ] = 1 ;
}

void Game::onKeyUp( SDL_Event* evt ) {
    keys[ evt->key.keysym.sym ] = 0 ;
    switch(evt->key.keysym.sym) {
      case SDLK_m: {
        drawMiniMapOn = !drawMiniMapOn;
        break;
      }
    }
}

int main(int argc, char** argv){
    Game game;
    game.start();
    return 0;
}
