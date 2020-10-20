/*
SDL2 raycasting demo.
Author: Andrew Lim Chong Liang
https://github.com/andrew-lim

Linker settings
-lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer

Recommended compiler settings
-Wall -pedantic -O2
*/
#include <SDL.h>
#include <SDL_mixer.h>
#include "sdl2utils.h"
#include <cstdio>
#include <map>
#include <cmath>
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include <ctime>
#include <queue>
#include <windows.h>
#include "raycasting.h"
#include "defaults.h"

using namespace al::sdl2utils;
using namespace al::raycasting;
using namespace std;

// The longer each raycast strip, the less rays will be used.
// Increase this to boost FPS but reduce rendering quality.
// Currently supports values from 1 to 4.
const int STRIP_WIDTH = 2;

// Length of a wall or cell in game units.
// In the original Wolfenstein 3D it was 8 feet (1 foot = 16 units)
const int TILE_SIZE = 128;

const int TEXTURE_SIZE = 128; // length of wall textures in pixels
const int DISPLAY_WIDTH = 800;
const int DISPLAY_HEIGHT = 600;
const int MINIMAP_SCALE = 6;
const int MINIMAP_Y = 0; // position of minimap from top of screen
const int DESIRED_FPS = 120;
const int UPDATE_INTERVAL = 1000/DESIRED_FPS;
const int FOV_DEGREES = 90;
const float FOV_RADIANS = (float)FOV_DEGREES * M_PI / 180; // FOV in radians
const int RAYCOUNT = DISPLAY_WIDTH / STRIP_WIDTH;
const float VIEW_DIST = Raycaster::screenDistance(DISPLAY_WIDTH,FOV_RADIANS);
const float TWO_PI = M_PI*2;

const int SKYBOX_WIDTH = 512;
const int SKYBOX_HEIGHT = 128;

void array2DToVector(int arr[MAP_HEIGHT][MAP_WIDTH], std::vector<int>& out) {
  out.clear();
  for (int y=0; y<MAP_HEIGHT; y++) {
    for (int x=0; x<MAP_WIDTH; x++) {
      out.push_back(arr[y][x]);
    }
  }
}

typedef enum SpriteType {
  SpriteTypeTree1 = 1,
  SpriteTypeTree2 = 2,
  SpriteTypeZombie = 3,
  SpriteTypeSkeleton = 4,
  SpriteTypeRobot = 5,
  SpriteTypeFrogman = 6,
  SpriteTypeHeroine = 7,
  SpriteTypeDruid = 8,
  SpriteTypeProjectile = 9,
  SpriteTypeProjectileSplash = 10
} SpriteType;



class Game {
public:
    Game();
    ~Game();
    void start();
    void stop() ;
    void draw();
    void fillRect(SDL_Rect* rc, int r, int g, int b );
    void drawLine(int startX, int startY, int endX, int endY, int r, int g,
                  int b, int alpha=SDL_ALPHA_OPAQUE );
    void fpsChanged( int fps );
    void onQuit();
    void onKeyDown( SDL_Event* event );
    void onKeyUp( SDL_Event* event );
    void reset();
    void run();
    void update(float timeElapsed);
    void drawFloor(vector<RayHit>& rayHits);
    void drawCeiling(vector<RayHit>& rayHits);
    void drawWeapon();
    void drawMiniMap();
    void drawMiniMapSprites();
    void updatePlayer(float elapsedTime);
    void updateProjectiles(float elaspedTime);
    void drawPlayer();
    void drawRay(float rayX, float rayY);
    void drawRays(vector<RayHit>& rayHits);
    void drawWallStrip(SurfaceTexture& img, float textureX, float textureY,
                       int stripIdx, int wallScreenHeight, int floors=1,
                       int level=0, bool aboveWall=false);
    void drawWorld(vector<RayHit>& rayHits);
    void raycastWorld(vector<RayHit>& rayHits);
    bool isWallCell(int wallX, int wallY);
    SDL_Rect findSpriteScreenPosition( Sprite& sprite );
    void addSpriteAt( int spriteid, int cellX, int cellY );
    void addProjectile(int textureid, int x, int y, int size, float rotation);
    float sine(float f);
    float cosine(float f);
private:
    std::vector< std::vector<int> > grids;
    Raycaster raycaster3D;
    std::vector<int> ceilingGrid;
    std::vector<int> groundWalls;
    std::map<int,int> keys; // store keydown presses
    int frameSkip ;
    int running ;
    SDL_Window* window;
    SDL_Renderer* renderer;
    Sprite player;
    SurfaceTexture wallsImage, wallsImageDark;
    SurfaceTexture gunImage;
    vector<Sprite> sprites;
    std::queue<Sprite> projectilesQueue;
    bool drawMiniMapOn, drawTexturedFloorOn, drawCeilingOn, drawWallsOn;
    Bitmap ceilingBitmap;
    SDL_Surface* skyboxSurface;
    Uint32 ceilingColor;
    Mix_Chunk* projectileFireSound;
    Mix_Chunk* projectileExplodeSound;
    std::map<int,SurfaceTexture> spriteTextures;
    std::vector<Bitmap> floorCeilingBitmaps;

    SDL_Texture* screenTexture;
    SDL_Surface* screenSurface;
    int highestCeilingLevel;
};

float Game::sine(float f) {
  return sin(f);
}

float Game::cosine(float f) {
  return cos(f);
}

void Game::addSpriteAt( int spriteid, int cellX, int cellY ) {
  Sprite s;
  s.x = cellX * TILE_SIZE  + (TILE_SIZE/2);
  s.y = cellY * TILE_SIZE  + (TILE_SIZE/2);
  s.w = TILE_SIZE;
  s.h = TILE_SIZE;
  s.textureID = spriteid;
  sprites.push_back(s);
}

void Game::addProjectile(int textureid, int x, int y, int size, float rotation){
  Sprite s;
  s.x = x;
  s.y = y;
  s.rot = rotation;
  s.w = size;
  s.h = size;
  s.textureID = textureid;
  projectilesQueue.push( s );
}

Game::Game()
:frameSkip(0), running(0), window(NULL), renderer(NULL) {
  srand (time(NULL));

  drawMiniMapOn = true;
  drawTexturedFloorOn = true;
  drawCeilingOn = true;
  drawWallsOn = true;

  reset();
}

Game::~Game() {
  this->stop();
}

void Game::reset()
{
  highestCeilingLevel = 3;

  raycaster3D.createGrids(MAP_WIDTH, MAP_HEIGHT,highestCeilingLevel, TILE_SIZE);
  array2DToVector(g_map, raycaster3D.grids[0]);
  array2DToVector(g_map2, raycaster3D.grids[1]);
  array2DToVector(g_map3, raycaster3D.grids[2]);

  grids = raycaster3D.grids;

  array2DToVector(g_ceilingmap, ceilingGrid);

  player.x = 28 * TILE_SIZE;
  player.y = 12 * TILE_SIZE;
  player.rot = 0;
  player.moveSpeed = TILE_SIZE / (DESIRED_FPS/60.0f*16);
  player.rotSpeed = 1.5 * M_PI/180;

  sprites.clear();
  for (int y=0; y<MAP_HEIGHT; y++) {
    for (int x=0; x<MAP_WIDTH; x++) {
      int spriteid = g_spritemap[ y ][ x ];
      if (spriteid) {
        addSpriteAt( spriteid, x, y );
      }
    }
  }
}

void Game::start() {
  printf("Resolution   = %d x %d\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
  printf("Map size     = %d x %d\n", MAP_WIDTH, MAP_HEIGHT);
  printf("Wall size    = %d game units\n", TILE_SIZE);
  printf("Texture Size = %d pixels\n", TEXTURE_SIZE);
  printf("FOV          = %d degrees\n", FOV_DEGREES);
  printf("STRIP_WIDTH  = %d\n", STRIP_WIDTH);
  printf("RAYCOUNT     = %d\n", RAYCOUNT);
  printf("Distance to Projection Plane = %f\n", VIEW_DIST);
  int flags = SDL_WINDOW_SHOWN ;
  if (SDL_Init(SDL_INIT_EVERYTHING)) {
      return ;
  }


  SDL_version compiled;
  SDL_version linked;

  SDL_VERSION(&compiled);
  SDL_GetVersion(&linked);
  printf("Compiled SDL version = %d.%d.%d\n", compiled.major, compiled.minor,
         compiled.patch);
  printf("Linked SDL version   = %d.%d.%d\n",linked.major, linked.minor,
         linked.patch);

  window = SDL_CreateWindow("SDL2 Raycast Engine",
                             SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED,
                            DISPLAY_WIDTH,
                             DISPLAY_HEIGHT,
                             flags);

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED );
//  SDL_SetWindowResizable(window, SDL_TRUE );
//  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_TARGET, DISPLAY_WIDTH,
                                    DISPLAY_HEIGHT);

  screenSurface = SDL_CreateRGBSurface(0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 32,
                                       0x00FF0000,
                                       0x0000FF00,
                                       0x000000FF,
                                       0xFF000000);

  SDL_RendererInfo rendererInfo;
  SDL_GetRendererInfo(renderer, &rendererInfo);

  printf("Renderer: %s ", rendererInfo.name);
  if (rendererInfo.flags & SDL_RENDERER_SOFTWARE) {
    printf("- SDL_RENDERER_SOFTWARE\n");
  }
  if (rendererInfo.flags & SDL_RENDERER_ACCELERATED) {
    printf("- SDL_RENDERER_ACCELERATED\n");
  }

  Uint32 pf = SDL_GetWindowPixelFormat(window);
  printf("windowPixelFormat = %s\n", SDL_GetPixelFormatName(pf));

  SDL_PixelFormat* tmppf = SDL_AllocFormat(pf);
  ceilingColor = SDL_MapRGB(tmppf, 139, 185, 249);
  SDL_FreeFormat(tmppf);

  //--------------
  // Load Textures
  //--------------

  if (!wallsImage.loadBitmap("..\\res\\walls4.bmp")) {
    printf("Error loading walls4.bmp\n");
    return;
  }
  if (!wallsImageDark.loadBitmap("..\\res\\walls4dark.bmp")) {
    printf("Error loading walls4dark.bmp\n");
    return;
  }
  wallsImage.createTexture(renderer);

  if (!gunImage.loadBitmap("..\\res\\gun1a.bmp")) {
    printf("Error loading gun image\n");
    return;
  }
  Uint32 colorKey = SDL_MapRGB(gunImage.getSurface()->format,152,0,136);
  SDL_SetColorKey( gunImage.getSurface(), true, colorKey );
  gunImage.createTexture(renderer);

  // Load Sprite Images
  std::map<int,std::string> spriteFilenames;
  spriteFilenames[ SpriteTypeTree1 ] = "tree.bmp";
  spriteFilenames[ SpriteTypeTree2 ] = "tree2.bmp";
  spriteFilenames[ SpriteTypeZombie ] = "zombie.bmp";
  spriteFilenames[ SpriteTypeSkeleton ] = "skeleton.bmp";
  spriteFilenames[ SpriteTypeRobot ] = "robot1.bmp";
  spriteFilenames[ SpriteTypeFrogman ] = "frogman.bmp";
  spriteFilenames[ SpriteTypeHeroine ] = "heroine.bmp";
  spriteFilenames[ SpriteTypeDruid ] = "druid.bmp";
  spriteFilenames[ SpriteTypeProjectile ] = "plasmball.bmp";
  spriteFilenames[ SpriteTypeProjectileSplash ] = "fireball0.bmp";
  for (std::map<int,std::string>::iterator i=spriteFilenames.begin();
       i!=spriteFilenames.end(); ++i)
  {
    int textureid = i->first;
    std::string filename = "..\\res\\" + i->second;
    printf("Loading texture image %s\n", filename.c_str());
    SurfaceTexture& surfaceTexture = spriteTextures[textureid];
    if (!surfaceTexture.loadBitmap(filename.c_str())) {
      printf("Error loading %s\n", filename.c_str());
      return;
    }
    SDL_SetColorKey( surfaceTexture.getSurface(), true, colorKey );
    surfaceTexture.createTexture(renderer);
  }

  // Load Floors and Ceiling Images
  std::map<int,std::string> floorCeilingFilenames;
  floorCeilingFilenames[ 0 ] = "grass.bmp";
  floorCeilingFilenames[ 1 ] = "texture1.bmp";
  floorCeilingFilenames[ 2 ] = "texture2.bmp";
  floorCeilingFilenames[ 3 ] = "texture3.bmp";
  floorCeilingFilenames[ 4 ] = "texture4.bmp";
  floorCeilingFilenames[ 5 ] = "default_brick.bmp";
  floorCeilingFilenames[ 6 ] = "default_aspen_wood.bmp";
  floorCeilingFilenames[ 7 ] = "water.bmp";
  floorCeilingFilenames[ 8 ] = "mossycobble.bmp";
  floorCeilingBitmaps.resize(floorCeilingFilenames.size());
  for (std::map<int,std::string>::iterator i=floorCeilingFilenames.begin();
       i!=floorCeilingFilenames.end(); ++i)
  {
    int textureid = i->first;
    std::string filename = "..\\res\\" + i->second;
    printf("Loading bitmap image %s\n", filename.c_str());
    Bitmap& bitmap = floorCeilingBitmaps[textureid];
    if (!bitmap.load(filename.c_str(), renderer, pf)) {
      printf("Error loading %s\n", filename.c_str());
      return;
    }
  }

  ceilingBitmap.load("..\\res\\texture1.bmp", renderer, pf);
  skyboxSurface = SDL_LoadBMP("..\\res\\skybox2.bmp");

  if (!skyboxSurface) {
    printf("Error loading skybox2.bmp\n");
    return;
  }
   skyboxSurface = SDL_ConvertSurface(skyboxSurface, screenSurface->format, 0);

//  SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

  //Initialize SDL_mixer
  if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 )
  {
    printf( "Mix_OpenAudio failed. SDL_mixer Error: %s\n", Mix_GetError() );
    return;
  }

  projectileFireSound = Mix_LoadWAV( "..\\res\\iceball.wav" );
  projectileExplodeSound = Mix_LoadWAV( "..\\res\\explode.wav" );
  Mix_VolumeChunk(projectileFireSound, MIX_MAX_VOLUME/2);
  Mix_VolumeChunk(projectileExplodeSound, MIX_MAX_VOLUME/3);

  this->running = 1 ;
  run();
}

void Game::draw() {
    // Clear screen
//    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 139, 185, 249, SDL_ALPHA_OPAQUE );
    SDL_RenderClear(renderer);

//    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
//    SDL_RenderFillRect(renderer, NULL);

    static vector<RayHit> allRayHits;
    allRayHits.clear();
    raycastWorld(allRayHits);
    drawWorld(allRayHits);
    drawWeapon();
    SDL_UpdateTexture(screenTexture, NULL, screenSurface->pixels,
                      screenSurface->pitch);
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    if (drawMiniMapOn) {
      drawMiniMap();
      drawRays(allRayHits);
      drawPlayer();
      drawMiniMapSprites();
    }


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

void Game::drawLine(int startX, int startY, int endX, int endY,
                    int r, int g, int b, int alpha ) {

  SDL_SetRenderDrawColor(renderer, r, g, b, alpha);
  SDL_RenderDrawLine(renderer, startX, startY, endX, endY);
}

void Game::drawWeapon() {
  float gunScale = DISPLAY_WIDTH / 320;
  SDL_Rect dstRect;
  SDL_Surface* gunSurface = gunImage.getSurface();
  dstRect.w = gunSurface->w * gunScale;
  dstRect.h = gunSurface->h * gunScale;
  dstRect.x = (DISPLAY_WIDTH - dstRect.w) / 2;
  dstRect.y = DISPLAY_HEIGHT - dstRect.h;
  SDL_BlitScaled(gunSurface, NULL, screenSurface, &dstRect);
}

void Game::fpsChanged( int fps ) {
    char szFps[ 128 ] ;
    sprintf( szFps, "SDL2 Raycast Demo - %d FPS", fps );
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
            update(timeElapsed);
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
//        SDL_Delay( 1 );
    }
}

void Game::update(float timeElapsed) {
    if (keys[SDLK_UP] || keys[SDLK_w]) { // up, move player forward
      player.speed = 1;
    }
    else if (keys[SDLK_DOWN] || keys[SDLK_s]) { // down, move player backward
      player.speed = -1;
    }
    else {
      player.speed = 0; // stop moving
    }

    if (keys[SDLK_LEFT] || keys[SDLK_a]) { // left, rotate player left
      player.dir = -1;
    }
    else if (keys[SDLK_RIGHT] || keys[SDLK_d]) { // right, rotate player right
      player.dir = 1;
    }
    else {
      player.dir = 0; // stop rotating
    }
    updatePlayer(timeElapsed);
    updateProjectiles(timeElapsed);
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

void Game::updatePlayer(float elapsedTime) {
  float timeBasedFactor = elapsedTime / UPDATE_INTERVAL;

  float moveStep = player.speed * player.moveSpeed * timeBasedFactor;
  player.rot += -player.dir * player.rotSpeed  * timeBasedFactor;
  int newX = player.x +  cosine(player.rot) * moveStep;
  int newY = player.y + -sine(player.rot) * moveStep;
  int wallX = newX / TILE_SIZE;
  int wallY = newY / TILE_SIZE;
  if (isWallCell(wallX, wallY)) {
    return;
  }
  if (g_floormap[wallY][wallX]==7) {
    // water tile
    return;
  }
  player.x = newX;
  player.y = newY;
}

bool needsCleanUp (const Sprite& sprite) {
  return sprite.cleanup;
}

bool isKillableSprite(int textureid) {
  return textureid >= 3 && textureid <= 8;
}

void Game::updateProjectiles(float timeElapsed) {

  sprites.erase( remove_if(sprites.begin(), sprites.end(), needsCleanUp),
                 sprites.end() );


  float timeBasedFactor = timeElapsed / UPDATE_INTERVAL;
  float projectileSpeed = player.moveSpeed * 3 * timeBasedFactor;
  float moveStep = 1 * projectileSpeed;
  while (!projectilesQueue.empty()) {
    Sprite newProjectile = projectilesQueue.front();
    if (newProjectile.textureID != SpriteTypeProjectileSplash) {
      int newX = newProjectile.x +  cosine(newProjectile.rot) *player.moveSpeed;
      int newY = newProjectile.y + -sine(newProjectile.rot) *player.moveSpeed;
      newProjectile.x = newX;
      newProjectile.y = newY;
    }
    else {
      newProjectile.frame = 0;
      newProjectile.frameRate = 200;
    }
    sprites.push_back(newProjectile);
    projectilesQueue.pop();
  }
  for (vector<Sprite>::iterator it=sprites.begin();
       it!=sprites.end(); ++it) {
    Sprite& projectile = *it;
    if (projectile.textureID == SpriteTypeProjectileSplash) {
      projectile.frameRate -= timeElapsed;
      if (projectile.frameRate <= 0) {
        projectile.cleanup = true;
      }
      continue;
    }
    if (isKillableSprite(projectile.textureID)) {
      if (projectile.frameRate) {
        projectile.frameRate -= timeElapsed;
        if (projectile.frameRate <= 0) {
          projectile.hidden = true;
        }
      }
      continue;
    }
    if (projectile.textureID != SpriteTypeProjectile)
    {
      continue;
    }
    int newX = projectile.x +  cosine(projectile.rot) * moveStep;
    int newY = projectile.y + -sine(projectile.rot) * moveStep;
    int wallX = newX / TILE_SIZE;
    int wallY = newY / TILE_SIZE;

    bool wallHit = false;
    bool outOfBounds = false;
    bool hitOtherSprite = false;
    if (isWallCell(wallX, wallY)) {
      newX = projectile.x +  cosine(projectile.rot) * moveStep/4;
      newY = projectile.y + -sine(projectile.rot) * moveStep/4;
      wallX = newX / TILE_SIZE;
      wallY = newY / TILE_SIZE;
      if (isWallCell(wallX, wallY)) {
        if (!projectile.cleanup) {
          addProjectile(SpriteTypeProjectileSplash, projectile.x, projectile.y,
                        TILE_SIZE, projectile.rot);
          Mix_PlayChannel( -1, projectileExplodeSound, 0 );
        }
        projectile.cleanup = true;
        wallHit = true;
      }
    }

    if (wallHit) {
    }
    else if (newX<0 || newX>MAP_WIDTH*TILE_SIZE ||
             newY<0 || newY>MAP_HEIGHT*TILE_SIZE) {
      outOfBounds = true;
      projectile.cleanup = true;
    }

    if (!wallHit && !outOfBounds)
    {
      vector<Sprite*> spritesHit =
        Raycaster::findSpritesInCell(sprites, wallX, wallY, TILE_SIZE);

      for (vector<Sprite*>::iterator it=spritesHit.begin();
           it!=spritesHit.end(); ++it)
      {
        Sprite* spriteHit = *it;
        if (isKillableSprite(spriteHit->textureID) && !spriteHit->hidden) {
          if (spriteHit->frameRate==0) {
            spriteHit->frameRate = 200;
            hitOtherSprite = true;
            if (!projectile.cleanup) {
              projectile.cleanup = true;
              addProjectile(SpriteTypeProjectileSplash, projectile.x,
                            projectile.y, TILE_SIZE, projectile.rot);
              Mix_PlayChannel( -1, projectileExplodeSound, 0 );
            }
          }
        }
      }
    }

    if (!wallHit && !outOfBounds && !hitOtherSprite)
    {
      projectile.x = newX;
      projectile.y = newY;
    }
  }
}

void Game::drawPlayer() {
  SDL_Rect playerRect;

  float playerX =  (float)player.x / (MAP_WIDTH*TILE_SIZE) * 100;
  playerX = playerX/100 * MINIMAP_SCALE * MAP_WIDTH;

  float playerY =  (float)player.y / (MAP_HEIGHT*TILE_SIZE) * 100;
  playerY = playerY/100 * MINIMAP_SCALE * MAP_HEIGHT;

  playerRect.x = playerX - 2 ;
  playerRect.y = playerY - 2 + MINIMAP_Y;
  playerRect.w = 5;
  playerRect.h = 5;
  fillRect(&playerRect, 255, 0, 0);

  float lineEndX = playerX +  cosine(player.rot) * 4 * MINIMAP_SCALE;
  float lineEndY = playerY + -sine(player.rot) * 4 * MINIMAP_SCALE;

  drawLine(playerX, playerY+MINIMAP_Y, lineEndX, lineEndY+MINIMAP_Y, 255, 0, 0);
}

void Game::drawMiniMapSprites() {
  for (vector<Sprite>::iterator it=sprites.begin(); it!=sprites.end(); ++it)
  {
    Sprite* sprite = &*it;
    float spriteX =  (float)sprite->x / (MAP_WIDTH*TILE_SIZE) * 100;
    spriteX = spriteX/100 * MINIMAP_SCALE * MAP_WIDTH;

    float spriteY =  (float)sprite->y / (MAP_HEIGHT*TILE_SIZE) * 100;
    spriteY = spriteY/100 * MINIMAP_SCALE * MAP_HEIGHT;

    SDL_Rect rc;
    rc.x = spriteX - 2 ;
    rc.y = spriteY - 2 + MINIMAP_Y;
    rc.w = 5;
    rc.h = 5;
    fillRect(&rc, 0, 255, 255);
  }

}

void Game::drawRay(float rayX, float rayY) {
  float playerX =  (float)player.x / (MAP_WIDTH*TILE_SIZE) * 100;
  playerX = playerX/100 * MINIMAP_SCALE * MAP_WIDTH;

  float playerY =  (float)player.y / (MAP_HEIGHT*TILE_SIZE) * 100;
  playerY = playerY/100 * MINIMAP_SCALE * MAP_HEIGHT;

  rayX = rayX / (MAP_WIDTH*TILE_SIZE) * 100.0;
  rayX = rayX/100.0 * MINIMAP_SCALE * MAP_WIDTH;
  rayY = rayY / (MAP_HEIGHT*TILE_SIZE) * 100.0;
  rayY = rayY/100.0 * MINIMAP_SCALE * MAP_HEIGHT;

  drawLine(playerX, playerY+MINIMAP_Y, rayX, rayY+MINIMAP_Y,0,100,0,0.3*255);
}

void Game::drawRays(vector<RayHit>& rayHits) {
   for (int i=0; i<(int)rayHits.size(); ++i) {
    RayHit rayHit = rayHits[i];
    if (rayHit.wallType && rayHit.level==0) {
      drawRay(rayHit.x, rayHit.y);
    }
  }
}

void Game::drawFloor(vector<RayHit>& rayHits)
{
  // If floor texture mapping off, just draw a solid color
  if (!drawTexturedFloorOn) {
    SDL_Rect rc;
    rc.x = 0;
    rc.y = DISPLAY_HEIGHT/2;
    rc.w = DISPLAY_WIDTH;
    rc.h = DISPLAY_HEIGHT/2;
    SDL_FillRect(screenSurface, &rc,SDL_MapRGB(screenSurface->format,52,158,0));
    return;
  }

  Uint32* streamingPixels = (Uint32*) screenSurface->pixels;
  for (int i=0; i<(int)rayHits.size(); ++i) {
    RayHit rayHit = rayHits[i];

    // Must be a wall, not a sprite
    if (!rayHit.wallType) {
      continue;
    }

    // Only draw below lowest wall
    if (rayHit.level>0) {
      continue;
    }

    int wallScreenHeight = Raycaster::stripScreenHeight(VIEW_DIST,
                                                        rayHit.correctDistance,
                                                        TILE_SIZE);
    int screenX = rayHit.strip * STRIP_WIDTH;
    int screenY = (DISPLAY_HEIGHT - wallScreenHeight)/2 + wallScreenHeight-1;
    float eyeHeight = TILE_SIZE / 2;
    float centerPlane = DISPLAY_HEIGHT / 2;
    for (; screenY<DISPLAY_HEIGHT; screenY++)
    {
      float ratio= eyeHeight/(screenY-centerPlane);
      float diagonalDistance = (float)VIEW_DIST * (float)ratio;

      // To correct for fisheye effect
      float correctDistance = diagonalDistance *
                               (1/cos(player.rot-rayHit.rayAngle));

      float xEnd = (correctDistance *  cosine(rayHit.rayAngle));
      float yEnd = (correctDistance * -sine(rayHit.rayAngle));
      yEnd += player.y;
      xEnd += player.x;
      int x = (int)(yEnd*2) % TILE_SIZE;
      int y = (int)(xEnd*2) % TILE_SIZE;
      int tileX = xEnd / TILE_SIZE;
      int tileY = yEnd / TILE_SIZE;
      int floorTileType = g_floormap[ tileY ][ tileX ];
      Bitmap& bitmap = floorCeilingBitmaps[ floorTileType ];
      Uint32* pix = (Uint32*)bitmap.getPixels();
      if (!pix) {
        continue;
      }
      int dstPixel = screenX + screenY * DISPLAY_WIDTH;
      int srcPixel = y * bitmap.getWidth() + x;
      streamingPixels[dstPixel] = pix[srcPixel];

      // Clamp the strip width so we don't write out of bounds of  the
      // streamingPixels. Not sure if this is necessary.
      int stripWidth = STRIP_WIDTH;
      while (stripWidth * rayHit.strip >= DISPLAY_WIDTH) {
        stripWidth--;
      }

      switch (stripWidth) {
        case 4:
          streamingPixels[dstPixel+3] = pix[srcPixel];
        case 3:
          streamingPixels[dstPixel+2] = pix[srcPixel];
        case 2:
          streamingPixels[dstPixel+1] = pix[srcPixel];
        default:
          break;
      }
    }
  }
}

void Game::drawCeiling(vector<RayHit>& rayHits)
{
  if (!drawCeilingOn) {
    SDL_Rect rc;
    rc.x = 0;
    rc.y = 0;
    rc.w = DISPLAY_WIDTH;
    rc.h = DISPLAY_HEIGHT/2;
    SDL_FillRect(screenSurface, &rc,
                 SDL_MapRGB(screenSurface->format,139, 185, 249));
    return;
  }
  Uint32* streamingPixels = (Uint32*) screenSurface->pixels;

  for (int i=0; i<(int)rayHits.size(); i++) {
    RayHit rayHit = rayHits[i];
      // Only draw above furthest wall
    if (!rayHit.wallType) {
      continue;
    }

    // Only draw above highest wall
    if (rayHit.level!=highestCeilingLevel-1) {
      int gridAbove = rayHit.level + 1;
      if (gridAbove<raycaster3D.gridCount &&
          raycaster3D.cellAt(rayHit.wallX, rayHit.wallY, gridAbove)) {
        continue;
      }
    }

    int wallScreenHeight = Raycaster::stripScreenHeight(VIEW_DIST,
                                                        rayHit.correctDistance,
                                                        TILE_SIZE);
    int screenX = rayHit.strip * STRIP_WIDTH;
    int screenY = (DISPLAY_HEIGHT - wallScreenHeight)/2 - 1;
    float eyeHeight = TILE_SIZE / 2;
    float centerPlane = DISPLAY_HEIGHT / 2;
    for (;screenY>=0;screenY--)
    {
      float ceilingHeight = TILE_SIZE * highestCeilingLevel;
      float ratio = (ceilingHeight - eyeHeight) / (centerPlane - screenY);
      float diagonalDistance = (float)VIEW_DIST * (float)ratio;

      // To correct for fisheye effect
      float correctDistance = diagonalDistance *
                               (1/cos(player.rot-rayHit.rayAngle));

      float xEnd = (correctDistance *  cosine(rayHit.rayAngle));
      float yEnd = (correctDistance * -sine(rayHit.rayAngle));
      yEnd += player.y;
      xEnd += player.x;

      bool outOfBounds = xEnd<0 || xEnd>=MAP_WIDTH*TILE_SIZE ||
                         yEnd<0 || yEnd>=MAP_HEIGHT*TILE_SIZE;

      int x = (int)(yEnd) % TILE_SIZE;
      int y = (int)(xEnd) % TILE_SIZE;
      int tileX = xEnd / TILE_SIZE;
      int tileY = yEnd / TILE_SIZE;

      int tileType = outOfBounds ? 0 : g_ceilingmap[ tileY ][ tileX ];
      int dstPixel = screenX + screenY * DISPLAY_WIDTH;
      int srcPixel = y * TEXTURE_SIZE + x;

      // Clamp the strip width so we don't write out of bounds of  the
      // streamingPixels. Not sure if this is necessary.
      int stripWidth = STRIP_WIDTH;
      while (stripWidth * rayHit.strip >= DISPLAY_WIDTH) {
        stripWidth--;
      }

      if (tileType) {
        Bitmap& bitmap = floorCeilingBitmaps[tileType];
        Uint32* pix = (Uint32*)bitmap.getPixels();
        if (!pix) {
          continue;
        }
        switch (stripWidth) {
          case 4:
            streamingPixels[dstPixel+3] = pix[srcPixel];
          case 3:
            streamingPixels[dstPixel+2] = pix[srcPixel];
          case 2:
            streamingPixels[dstPixel+1] = pix[srcPixel];
          default:
            streamingPixels[dstPixel] = pix[srcPixel];
            break;
        }
      }
      else {
        const int PIXEL_LENGTH = SKYBOX_WIDTH * SKYBOX_HEIGHT;
        int skyboxY = (screenY / (DISPLAY_HEIGHT/2.0f) * SKYBOX_HEIGHT);
        Uint32* pix2 = (Uint32*) skyboxSurface->pixels;
        int skyboxX = (float)screenX / DISPLAY_WIDTH * SKYBOX_WIDTH;
        float rotation = player.rot;
  //        while (rotation < 0) rotation += TWO_PI;
  //        while (rotation >= TWO_PI) rotation -= TWO_PI;
        int skyboxOffsetX = -((rotation/TWO_PI)*SKYBOX_WIDTH)*4;
        skyboxX += skyboxOffsetX;
        int offset = (skyboxX%SKYBOX_WIDTH +skyboxY*SKYBOX_WIDTH);
        if (offset < 0) {
          offset = 0;
        }
        if (offset >= PIXEL_LENGTH) {
          offset = PIXEL_LENGTH - 1;
        }
        Uint32 pixel = pix2[ offset ];
        switch (stripWidth) {
          case 4:
            streamingPixels[dstPixel+3] = pixel;
          case 3:
            streamingPixels[dstPixel+2] = pixel;
          case 2:
            streamingPixels[dstPixel+1] = pixel;
          default:
            streamingPixels[dstPixel] = pixel;
            break;
        }
      }
    }
  }
}

void Game::drawWorld(vector<RayHit>& rayHits)
{
  // Depth sorth by furthest first. Draw using painter's algorithm.
  std::sort(rayHits.begin(), rayHits.end());

  drawCeiling(rayHits);
  drawFloor(rayHits);

  //-----------------------
  // Draw Walls and Sprites
  //-----------------------
  if (!drawWallsOn) {
    return;
  }

  for (int i=0; i<(int)rayHits.size(); ++i) {
    RayHit rayHit = rayHits[i];

    // Wall
    if (rayHit.wallType) {
      int wallScreenHeight = Raycaster::stripScreenHeight(VIEW_DIST,
                                                        rayHit.correctDistance,
                                                        TILE_SIZE);
      float sx = rayHit.tileX/TILE_SIZE*TEXTURE_SIZE;
      float sy = TEXTURE_SIZE * (rayHit.wallType-1);
      bool aboveWall = rayHit.level &&
                 raycaster3D.cellAt(rayHit.wallX, rayHit.wallY, rayHit.level-1);
      int sxi = (int) sx;

      SurfaceTexture* img = rayHit.horizontal ? &wallsImageDark : &wallsImage;

      //------------------------------------------------------------------------
      // Corner Checking Start
      //
      // I use different textures for horizontal and vertical lines.
      // However my raycasting algorithm has problems with some corners
      // where 2 identical blocks touching each other with the same texture
      // have a "tear" caused by a perpendicular line.
      //
      // For example, for a block type 1, we use texture 1a for its horizontal
      // faces, and 1b for its vertical faces.
      //
      // Now imagine we have 2 blocks of type 1 beside each other on the X-axis.
      // Their horizontal faces should have the same texture.
      // But sometimes at the corner where they touch the raycasting algorithm
      // finds a vertical line (also of the same block type).
      // So texture 1b is drawn and can cause a visible vertical line tear if
      // the 1a and 1b texture edges are very different.
      //
      // This block of code checks each possible corner where 2 blocks meet
      // and makes sure the perpendicular line drawn is the right texture.
      //
      // If you use the same texture for all sides of a block, you don't need
      // this check at all and can set cornerCheck to false.
      //------------------------------------------------------------------------
      bool cornerCheck = true;
      bool isLeftEdge = sxi==0;
      bool isRightEdge = sxi == TEXTURE_SIZE-1;
      if (cornerCheck && (isLeftEdge||isRightEdge))
      {
        const int wallX = rayHit.wallX;
        const int wallY = rayHit.wallY;
        const int level = rayHit.level;
        int rightWall = 0;
        int leftWall = 0;
        int bottomWall = 0;
        int topWall = 0;
        if(rayHit.wallX+1<MAP_WIDTH) {
          rightWall = raycaster3D.cellAt(wallX+1, wallY, level);
        }
        if(rayHit.wallX-1>=0) {
          leftWall = raycaster3D.cellAt(wallX-1, wallY, level);
        }
        if (rayHit.wallY+1<MAP_HEIGHT) {
          bottomWall = raycaster3D.cellAt(wallX, wallY+1, level);
        }
        if (rayHit.wallY-1>=0) {
          topWall = raycaster3D.cellAt(wallX, wallY-1, level);
        }
        if (isRightEdge) {
          if (rayHit.horizontal && rayHit.up && !rayHit.right && bottomWall) {
            img = &wallsImage;
          }
          else if (rayHit.up && rayHit.right && leftWall) {
            img = &wallsImageDark;
          }
        }
        else if (isLeftEdge) {
          if (rayHit.horizontal && !rayHit.up && !rayHit.right && topWall) {
            img = &wallsImage;
          }
          else if (rayHit.up && !rayHit.right && rightWall) {
            img = &wallsImageDark;
          }
        }
      }
      //---------------------
      // Corner Checking End
      //---------------------

      drawWallStrip(*img, sx, sy, rayHit.strip, wallScreenHeight, 1,
                    rayHit.level, aboveWall);

      // Ceilings below floating blocks
      if (drawCeilingOn) {
        int screenX = rayHit.strip * STRIP_WIDTH;
        float eyeHeight = TILE_SIZE / 2;
        float centerPlane = DISPLAY_HEIGHT / 2;

        // Draw bottom ceiling layer if wall is above an empty space
        int levelBelow = rayHit.level - 1;
        if (levelBelow>=0 &&
            raycaster3D.cellAt(rayHit.wallX,rayHit.wallY, levelBelow)==0) {
          int screenY = DISPLAY_HEIGHT/2;
          int groundWallX = rayHit.wallX;
          int groundWallY = rayHit.wallY;
          bool wasInCeilingTile = false;
          for (;screenY>=0;screenY--)
          {
            float ceilingHeight = TILE_SIZE * (rayHit.level);
            float ratio = (ceilingHeight - eyeHeight) / (centerPlane - screenY);
            float diagonalDistance = (float)VIEW_DIST * (float)ratio;

            // To correct for fisheye effect
            float correctDistance = diagonalDistance *
                                     (1/cos(player.rot-rayHit.rayAngle));

            float xEnd = (correctDistance *  cosine(rayHit.rayAngle));
            float yEnd = (correctDistance * -sine(rayHit.rayAngle));
            yEnd += player.y;
            xEnd += player.x;

            int x = (int)(yEnd) % TILE_SIZE;
            int y = (int)(xEnd) % TILE_SIZE;
            int wallX = xEnd / TILE_SIZE;
            int wallY = yEnd / TILE_SIZE;

            bool outOfBounds =xEnd<0 || xEnd>=MAP_WIDTH*TILE_SIZE ||
                              yEnd<0 || yEnd>=MAP_HEIGHT*TILE_SIZE;
            bool sameTile = wallX==groundWallX&&wallY==groundWallY;
            if (outOfBounds || !sameTile) {
              if (wasInCeilingTile) {
                break;
              }
            }

            int tileType = outOfBounds?0
                                      :(sameTile);
            if (tileType) {
              wasInCeilingTile = true;
              Bitmap& bitmap = floorCeilingBitmaps[ rayHit.wallType ];
              Uint32* pix = (Uint32*)bitmap.getPixels();
              if (!pix) {
                continue;
              }
              Uint32* screenPixels = (Uint32*) screenSurface->pixels;
              int dstPixel = screenX + screenY * DISPLAY_WIDTH;
              int srcPixel = y * bitmap.getWidth() + x;
              switch (STRIP_WIDTH) {
                case 4:
                  screenPixels[dstPixel+3] = pix[srcPixel];
                case 3:
                  screenPixels[dstPixel+2] = pix[srcPixel];
                case 2:
                  screenPixels[dstPixel+1] = pix[srcPixel];
                default:
                  screenPixels[dstPixel] = pix[srcPixel];
                  break;
              } // switch
            } // if (tileType)
          } // for
        } // if (raycaster)
      } // if (drawCeilingOn)
    } // if rayHit.wallType

    // Sprite
    else if (rayHit.sprite && !rayHit.sprite->hidden) {
      SDL_Rect dstRect;
      SurfaceTexture* spriteSurfaceTexture = NULL;
      std::map<int,SurfaceTexture>::iterator i =
        spriteTextures.find(rayHit.sprite->textureID);
      if (i == spriteTextures.end()) {
        continue;
      }
      spriteSurfaceTexture = &spriteTextures[rayHit.sprite->textureID];
      dstRect = findSpriteScreenPosition( *rayHit.sprite  );
      SDL_BlitScaled(spriteSurfaceTexture->getSurface(), NULL, screenSurface,
                     &dstRect);
    }
  }
}

void Game::drawWallStrip(SurfaceTexture& img, float textureX, float textureY,
                         int stripIdx, int wallScreenHeight, int floors,
                         int level, bool aboveWall)
{
  float sx = textureX;
  float sy = textureY;
  float swidth = 1;
  float sheight = TEXTURE_SIZE;
  float imgx = stripIdx * STRIP_WIDTH;
  float imgy = (DISPLAY_HEIGHT - wallScreenHeight)/2;
  float imgw = STRIP_WIDTH;
  float imgh = wallScreenHeight;

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

  dstrect.y--;
  // Hack: Make floating walls slightly longer to hide seams and tears
  // caused by ceiling drawing
  if (level && !aboveWall) {
    dstrect.h += 2 * level;
  }

  dstrect.y -= level * wallScreenHeight;
  SDL_BlitScaled(img.getSurface(), &srcrect, screenSurface, &dstrect);
  while (floors-- > 1) {
    dstrect.y -= wallScreenHeight;
    SDL_BlitScaled(img.getSurface(), &srcrect, screenSurface, &dstrect);
  }
}

// Algorithm here taken from this link but I use unit circle rotation instead.
// https://dev.opera.com/articles/3d-games-with-canvas-and-raycasting-part-2/
SDL_Rect Game::findSpriteScreenPosition( Sprite& sprite )
{
  // Translate position to viewer space
  float dx = sprite.x - player.x;
  float dy = sprite.y - player.y;

  // Distance to sprite
  float dist = sqrt(dx*dx + dy*dy);

  float spriteAngle = atan2(dy, dx) + player.rot;
  float spriteDistance = cos(spriteAngle)*dist;
  float spriteScreenWidth = TILE_SIZE * VIEW_DIST / spriteDistance;

  // X-position on screen
  float x = tan(spriteAngle) * VIEW_DIST;

  SDL_Rect rc;
  rc.x = (DISPLAY_WIDTH/2) + x - (spriteScreenWidth/2);
  rc.y = (DISPLAY_HEIGHT - spriteScreenWidth)/2.0f;
  rc.w = spriteScreenWidth;
  rc.h = spriteScreenWidth;
  return rc;
}

void Game::raycastWorld(vector<RayHit>& rayHits)
{
  vector<Sprite*> spritesFound;
  for (int strip=0; strip<RAYCOUNT; strip++) {
    float screenX = (RAYCOUNT/2 - strip) * STRIP_WIDTH;
    const float stripAngle = Raycaster::stripAngle(screenX, VIEW_DIST);

    vector<RayHit> rayHitsFound;
    raycaster3D.raycast(rayHitsFound, player.x, player.y, player.rot,
                        stripAngle, strip, &sprites);

    for (size_t j=0; j<rayHitsFound.size(); ++j) {
      RayHit rayHit = rayHitsFound[ j ];
      if ( rayHit.distance ) {
        // Wall found
        if (rayHit.wallType) {
          rayHits.push_back(rayHit);
        }
        // Sprite found
        else if (rayHit.sprite) {
          bool addedAlready = std::find(spritesFound.begin(),
                                        spritesFound.end(),  rayHit.sprite) !=
                                        spritesFound.end();
          if (!addedAlready) {
            spritesFound.push_back(rayHit.sprite);
            rayHits.push_back(rayHit);
          }
        }
      }
    }
  }
}

bool Game::isWallCell(int x, int y) {
  // first make sure that we cannot move outside the boundaries of the level
  if (y < 0 || y >= MAP_HEIGHT || x < 0 || x >= MAP_WIDTH)
    return true;

  // return true if the map block is not 0, ie. if there is a blocking wall.
  return (g_map[y][x]!= 0);
}

void Game::onKeyDown( SDL_Event* evt ) {
    keys[ evt->key.keysym.sym ] = 1 ;
}

void Game::onKeyUp( SDL_Event* evt ) {
    keys[ evt->key.keysym.sym ] = 0 ;
    switch(evt->key.keysym.sym) {
      case SDLK_m: {
        drawMiniMapOn = !drawMiniMapOn;
        printf("drawMiniMapOn = %s\n", drawMiniMapOn?"true":"false");
        break;
      }
      case SDLK_r: {
        reset();
        printf("Game reset!\n");
        break;
      }
      case SDLK_f: {
        drawTexturedFloorOn = !drawTexturedFloorOn;
        printf("drawTexturedFloorOn = %s\n",drawTexturedFloorOn?"true":"false");
        break;
      }
      case SDLK_c: {
        drawCeilingOn = !drawCeilingOn;
        printf("drawCeilingOn = %s\n", drawCeilingOn?"true":"false");
        break;
      }
      case SDLK_1: {
        drawWallsOn = !drawWallsOn;
        printf("drawWallsOn = %s\n", drawWallsOn?"true":"false");
        break;
      }
      case SDLK_SPACE: {
        Mix_PlayChannel( -1, projectileFireSound, 0 );
        addProjectile(SpriteTypeProjectile, player.x, player.y, TILE_SIZE,
                      player.rot);
        break;
      }
    }
}

int main(int argc, char** argv){
    Game game;
    game.start();
    return 0;
}
