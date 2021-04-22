/*
SDL2 raycasting demo.
Author: Andrew Lim Chong Liang
https://github.com/andrew-lim/sdl2-raycast

Linker settings
-lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer

Recommended compiler settings
-Wall -pedantic -O2
OR
-Wall -pedantic -O3 -fno-tree-vectorize
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
#include "settingsmanager.h"

using namespace al::sdl2utils;
using namespace al::raycasting;
using namespace std;
using namespace al;

// Length of a wall or cell in game units.
const int TILE_SIZE = 128;

const int TEXTURE_SIZE = 128; // length of wall textures in pixels
const int MINIMAP_SCALE = 6;
const int MINIMAP_Y = 0; // position of minimap from top of screen
const int DESIRED_FPS = 120;
const int UPDATE_INTERVAL = 1000/DESIRED_FPS;
const float TWO_PI = M_PI*2;
const int SKYBOX_WIDTH = 512;
const int SKYBOX_HEIGHT = 128;

// Maximum vertical distance travelled up and down in 1 jump
const float MAX_JUMP_DISTANCE = 3*TILE_SIZE;

// Half a jump, once this is reached, player begins falling down
const float HALF_JUMP_DISTANCE = MAX_JUMP_DISTANCE/2;

// Player floats at the jump apex
const float FLOAT_HEIGHT = HALF_JUMP_DISTANCE;

void array2DToVector(int arr[MAP_HEIGHT][MAP_WIDTH], std::vector<int>& out) {
  out.clear();
  for (int y=0; y<MAP_HEIGHT; y++) {
    for (int x=0; x<MAP_WIDTH; x++) {
      out.push_back(arr[y][x]);
    }
  }
}

// Copies the addresses of ThinWalls from in vector b to a
void appendThinWalls( std::vector<ThinWall*>& a,
                      std::vector<ThinWall>& b)
{
  for (size_t i=0; i<b.size(); ++i) {
    a.push_back( &b[i] );
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
  SpriteTypeProjectileSplash = 10,
  SpriteTypeGates = 11
} SpriteType;

// Fog Settings
#define FOG_R 150.0f
#define FOG_G 150.0f
#define FOG_B 150.0f
#define FOG_START_DISTANCE (TILE_SIZE*8)

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
    void printHelp();
    void update(float timeElapsed);
    void drawWallTop(RayHit& rayHit, int wallScreenHeight, float playerScreenZ);
    void drawWallBottom(RayHit&rayHit,int wallScreenHeight,float playerScreenZ);
    void drawThinWallTop(RayHit& rayHit, int wallScreenHeight);
    void drawThinWallBottom(RayHit& rayHit, int wallScreenHeight);
    bool drawSlope(RayHit& rayHit, float playerScreenZ);
    bool drawSlopeInverted(RayHit& rayHit, float playerScreenZ);
    void drawFloor(vector<RayHit>& rayHits);
    void drawSkyboxAndHighestCeiling(vector<RayHit>& rayHits);
    void drawWeapon();
    void drawMiniMap();
    void drawMiniMapSprites();
    void updatePlayer(float elapsedTime);
    void updateProjectiles(float elaspedTime);
    void drawPlayer();
    void drawRay(float rayX, float rayY);
    void drawRays(vector<RayHit>& rayHits);
    float wallScreenY(RayHit& rayHit, float wallHeight);
    SDL_Rect stripScreenRect(RayHit& rayHit, float wallHeight);
    void drawSlopeStrip(RayHit& rayHit, SurfaceTexture& img,
                        float textureX, float textureY);
    void drawThinWallStrip(RayHit& rayHit, SurfaceTexture& img,
                           float textureX, float textureY,
                           bool aboveWall=false, bool beloWall=false);
    void drawWallStrip(RayHit& rayHit, SurfaceTexture& img,
                       float textureX, float textureY,
                       int wallScreenHeight,
                       bool aboveWall=false, bool beloWall=false);
    bool ignoreWallStrip(RayHit& rayHit);
    void drawWorld(vector<RayHit>& rayHits);
    void raycastWorld(vector<RayHit>& rayHits);
    bool isWallCell(int wallX, int wallY, int level=0);
    bool playerInWall(float playerX, float playerY, float playerZ);
    SDL_Rect findSpriteScreenPosition( Sprite& sprite );
    void addSpriteAt( int spriteid, int cellX, int cellY );
    void addProjectile(int textureid, int x, int y, int z, int size,
                       float rotation);
    float sine(float f);
    float cosine(float f);
    void toggleDoorPressed();
    void toggleDoor(int cellX, int cellY);
    Uint32 fogPixel( Uint32 pixel, float distance );
    void fogWallStrip( SDL_Rect* dstrect, float distance  );
    float slopeHeightAt(float worldX, float worldY);
    void createThinWalls();
private:
    int displayWidth, displayHeight, stripWidth, rayCount;
    int fovDegrees;
    float fovRadians, viewDist;
    bool fullscreen;
    float pitch;
    Raycaster raycaster3D;
    std::vector<int> ceilingGrid;
    std::vector<int> groundWalls;
    int frameSkip ;
    int running ;
    SDL_Window* window;
    SDL_Renderer* renderer;
    Sprite player;
    SurfaceTexture wallsImage, wallsImageDark, gatesImage, gatesOpenImage;
    SurfaceTexture gunImage;
    vector<Sprite> sprites;
    std::queue<Sprite> projectilesQueue;
    bool drawMiniMapOn, drawTexturedFloorOn, drawCeilingOn, drawWallsOn;
    bool skipDrawnFloorStrips, skipDrawnSkyboxStrips;
    bool skipDrawnHighestCeilingStrips;
    bool drawWeaponOn;
    bool fogOn;
    Bitmap ceilingBitmap;
    SDL_Surface* skyboxSurface;
    Uint32 ceilingColor;
    Mix_Chunk* projectileFireSound;
    Mix_Chunk* projectileExplodeSound;
    Mix_Chunk* doorOpenSound;
    Mix_Chunk* doorCloseSound;
    std::map<int,SurfaceTexture> spriteTextures;
    std::vector<Bitmap> floorCeilingBitmaps;

    SDL_Texture* screenTexture;
    SDL_Surface* screenSurface;
    int highestCeilingLevel;
    int rayHitsCount;
    int doors[MAP_WIDTH * MAP_HEIGHT];
    std::vector<ThinWall*> thinWalls;
    ThickWall* rectWall;
    ThickWall* triangleWall;
    ThickWall* diamondWall;
    std::vector<ThickWall*> helloWalls;
    std::vector<ThickWall*> slopedWalls;
};


Game::Game()
:frameSkip(0), running(0), window(NULL), renderer(NULL) {
  srand (time(NULL));
  drawMiniMapOn = true;
  skipDrawnFloorStrips = true;
  skipDrawnSkyboxStrips = true;
  skipDrawnHighestCeilingStrips = true;
  drawTexturedFloorOn = true;
  drawCeilingOn = true;
  drawWeaponOn = true;
  drawWallsOn = true;
  rayHitsCount = 0;
  fogOn = false;
  rectWall = triangleWall = diamondWall = 0;
  reset();
}

Game::~Game() {
  this->stop();
}

void Game::reset()
{
  pitch = 0;

  highestCeilingLevel = 3;

  raycaster3D.createGrids(MAP_WIDTH, MAP_HEIGHT,highestCeilingLevel, TILE_SIZE);
  array2DToVector(g_map, raycaster3D.grids[0]);
  array2DToVector(g_map2, raycaster3D.grids[1]);
  array2DToVector(g_map3, raycaster3D.grids[2]);

  array2DToVector(g_ceilingmap, ceilingGrid);

  player.x = 26 * TILE_SIZE;
  player.y = 6 * TILE_SIZE;
  player.z = 0;
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

  for (int y=0; y<MAP_HEIGHT; ++y) {
    for (int x=0; x<MAP_WIDTH; ++x) {
      doors[ x + y * MAP_WIDTH ] = 0;
    }
  }

  createThinWalls();
}

void Game::createThinWalls()
{
  float commonZ = TILE_SIZE*0;
  int commonThinWallType = 4;
  int commonCeilingTextureID = 5;
  int commonFloorTextureID = 8;
  int commonHeight = TILE_SIZE * 0.2;

  if (slopedWalls.empty()) {
    // Floor slope 1
    ThickWall* slopedWall = new ThickWall();
    slopedWall->createRectSlope(
      SLOPE_TYPE_WEST_EAST,
      5*TILE_SIZE, 19*TILE_SIZE, TILE_SIZE*3, TILE_SIZE*1, 0,
      1, TILE_SIZE
    );
    slopedWalls.push_back(slopedWall);
    slopedWall->ceilingTextureID = commonCeilingTextureID;
    slopedWall->floorTextureID = 5;
    slopedWall->setThinWallsType(commonThinWallType);
    appendThinWalls(thinWalls, slopedWall->thinWalls);

    // Floor slope 2
    slopedWall = new ThickWall();
    slopedWall->createRectSlope(
      SLOPE_TYPE_NORTH_SOUTH,
      11*TILE_SIZE, 18*TILE_SIZE, TILE_SIZE*1, TILE_SIZE*2, 0,
      0, TILE_SIZE
    );
    slopedWalls.push_back(slopedWall);
    slopedWall->ceilingTextureID = commonCeilingTextureID;
    slopedWall->floorTextureID = 4;
    slopedWall->setThinWallsType(commonThinWallType);
    appendThinWalls(thinWalls, slopedWall->thinWalls);

    // Floor slope 3
    slopedWall = new ThickWall();
    slopedWall->createRectSlope(
      SLOPE_TYPE_WEST_EAST,
      7*TILE_SIZE, 12*TILE_SIZE, TILE_SIZE*5, TILE_SIZE, 0,
      TILE_SIZE, 0
    );
    slopedWalls.push_back(slopedWall);
    slopedWall->ceilingTextureID = commonCeilingTextureID;
    slopedWall->floorTextureID = 5;
    slopedWall->setThinWallsType(commonThinWallType);
    appendThinWalls(thinWalls, slopedWall->thinWalls);

    // Ceiling slope
    slopedWall = new ThickWall();
    slopedWall->createRectInvertedSlope(
      SLOPE_TYPE_NORTH_SOUTH,
      7*TILE_SIZE, 7*TILE_SIZE, TILE_SIZE, TILE_SIZE*2, TILE_SIZE*1.5,
      TILE_SIZE, 1
    );
    slopedWalls.push_back(slopedWall);
    slopedWall->ceilingTextureID = commonCeilingTextureID;
    slopedWall->floorTextureID = 5;
    slopedWall->setThinWallsType(commonThinWallType);
    appendThinWalls(thinWalls, slopedWall->thinWalls);
  }

  if (!rectWall) {
    rectWall = new ThickWall();
    rectWall->ceilingTextureID = commonCeilingTextureID;
    rectWall->floorTextureID = commonFloorTextureID;
    rectWall->createRectThickWall(
      18*TILE_SIZE, 17*TILE_SIZE, TILE_SIZE, TILE_SIZE, commonZ, commonHeight
    );
    rectWall->setThinWallsType(commonThinWallType);
    appendThinWalls(thinWalls, rectWall->thinWalls);
  }

  if (!triangleWall) {
    triangleWall = new ThickWall();
    triangleWall->ceilingTextureID = commonCeilingTextureID;
    triangleWall->floorTextureID = commonFloorTextureID;
    triangleWall->createTriangleThickWall(
      Point(rectWall->x - 1.5*TILE_SIZE, rectWall->y),
      Point(rectWall->x - 0.5*TILE_SIZE, rectWall->y + TILE_SIZE),
      Point(rectWall->x - 2.5*TILE_SIZE, rectWall->y + TILE_SIZE),
      commonZ, commonHeight
    );
    triangleWall->setThinWallsType(commonThinWallType);
    appendThinWalls(thinWalls, triangleWall->thinWalls);
  }

  if (!diamondWall) {
    diamondWall = new ThickWall();
    diamondWall->ceilingTextureID = commonCeilingTextureID;
    diamondWall->floorTextureID = commonFloorTextureID;
    diamondWall->createQuadThickWall(
      Point(rectWall->x - 3.5*TILE_SIZE, rectWall->y),
      Point(rectWall->x - 3*TILE_SIZE, rectWall->y + 0.5*TILE_SIZE),
      Point(rectWall->x - 3.5*TILE_SIZE, rectWall->y + TILE_SIZE),
      Point(rectWall->x - 4*TILE_SIZE, rectWall->y + 0.5*TILE_SIZE),
      commonZ, commonHeight
    );
    diamondWall->setThinWallsType(4);
    appendThinWalls(thinWalls, diamondWall->thinWalls);
  }

  if (helloWalls.empty()) {
    float helloCoords[15][6] = {
      { 4*TILE_SIZE, 18*TILE_SIZE, 0.2*TILE_SIZE, 0.2*TILE_SIZE, 0, 128 },
      { 4*TILE_SIZE, 17.80*TILE_SIZE, 0.2*TILE_SIZE, 0.2*TILE_SIZE, 48, 32 },
      { 4*TILE_SIZE, 17.61*TILE_SIZE, 0.2*TILE_SIZE, 0.2*TILE_SIZE, 0, 128 },
      { 4*TILE_SIZE, 17.22*TILE_SIZE, 0.2*TILE_SIZE, 0.2*TILE_SIZE, 0, 128 },
      { 4*TILE_SIZE, 16.83*TILE_SIZE, 0.2*TILE_SIZE, 0.39*TILE_SIZE, 0, 25 },
      { 4*TILE_SIZE, 16.83*TILE_SIZE, 0.2*TILE_SIZE, 0.39*TILE_SIZE, 51, 25 },
      { 4*TILE_SIZE, 16.83*TILE_SIZE, 0.2*TILE_SIZE, 0.39*TILE_SIZE, 103, 25 },
      { 4*TILE_SIZE, 16.44*TILE_SIZE, 0.2*TILE_SIZE, 0.2*TILE_SIZE, 0, 128 },
      { 4*TILE_SIZE, 16.05*TILE_SIZE, 0.2*TILE_SIZE, 0.39*TILE_SIZE, 0, 25 },
      { 4*TILE_SIZE, 15.66*TILE_SIZE, 0.2*TILE_SIZE, 0.2*TILE_SIZE, 0, 128 },
      { 4*TILE_SIZE, 15.27*TILE_SIZE, 0.2*TILE_SIZE, 0.39*TILE_SIZE, 0, 25 },
      { 4*TILE_SIZE, 14.88*TILE_SIZE, 0.2*TILE_SIZE, 0.2*TILE_SIZE, 0, 128 },
      { 4*TILE_SIZE, 14.68*TILE_SIZE, 0.2*TILE_SIZE, 0.2*TILE_SIZE, 0, 25 },
      { 4*TILE_SIZE, 14.68*TILE_SIZE, 0.2*TILE_SIZE, 0.2*TILE_SIZE, 103, 25 },
      { 4*TILE_SIZE, 14.48*TILE_SIZE, 0.2*TILE_SIZE, 0.2*TILE_SIZE, 0, 128 },
    };
    for (int i=0; i<15; ++i) {
      ThickWall* tmpWall = new ThickWall();
      tmpWall->ceilingTextureID = commonCeilingTextureID;
      tmpWall->floorTextureID = commonFloorTextureID;
      tmpWall->createRectThickWall(
        helloCoords[i][0],
        helloCoords[i][1],
        helloCoords[i][2],
        helloCoords[i][3],
        helloCoords[i][4],
        helloCoords[i][5]
      );
      helloWalls.push_back( tmpWall );
      tmpWall->setThinWallsType(4);
      appendThinWalls(thinWalls, tmpWall->thinWalls);
    }
  }
}

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

   // If sprite is inside a wall, keep moving it up
  int spriteWallX = (int)s.x / TILE_SIZE;
  int spriteWallY = (int)s.y / TILE_SIZE;
  int level = 0;
  bool inWall = raycaster3D.safeCellAt(spriteWallX, spriteWallY, level, 0);
  while (inWall) {
    level++;
    inWall = raycaster3D.safeCellAt(spriteWallX, spriteWallY, level, 0);
    if (!inWall) {
      break;
    }
  }
  s.level = level;
  s.z = level * TILE_SIZE;

  sprites.push_back(s);
}

void Game::addProjectile(int textureid, int x, int y, int z, int size,
                         float rotation)
{
  Sprite s;
  s.x = x;
  s.y = y;
  s.rot = rotation;
  s.w = size;
  s.h = size;
  s.textureID = textureid;
  s.z = z;
  projectilesQueue.push( s );
}

void Game::start() {
  SettingsManager settingsManager;
  settingsManager.loadConfig("config.ini");

  displayWidth = settingsManager.getInt("displayWidth",DEFAULT_DISPLAY_WIDTH);
  displayHeight =settingsManager.getInt("displayHeight",DEFAULT_DISPLAY_HEIGHT);
  stripWidth = settingsManager.getInt("stripWidth", DEFAULT_STRIP_WIDTH);
  rayCount = displayWidth / stripWidth;
  fovDegrees = settingsManager.getInt("fov", DEFAULT_FOV_DEGREES);
  fovRadians = (float)fovDegrees * M_PI / 180;
  viewDist = Raycaster::screenDistance(displayWidth, fovRadians);
  fullscreen = settingsManager.getInt("fullscreen", 0);

  printf("Resolution   = %d x %d\n", displayWidth, displayHeight);
  printf("Map size     = %d x %d\n", MAP_WIDTH, MAP_HEIGHT);
  printf("FOV          = %d degrees\n", fovDegrees);
  printf("stripWidth   = %d\n", stripWidth);
  printf("rayCount     = %d\n", rayCount);
  printf("Distance to Projection Plane = %f\n", viewDist);
  printf("Wall size    = %d game units\n", TILE_SIZE);
  printf("Texture Size = %d pixels\n", TEXTURE_SIZE);
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
                            displayWidth,
                             displayHeight,
                             flags);

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED );
  SDL_SetWindowResizable(window, SDL_TRUE );

  screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_TARGET, displayWidth,
                                    displayHeight);

  screenSurface = SDL_CreateRGBSurface(0, displayWidth, displayHeight, 32,
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

  if (fullscreen) {
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
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
  if (!gatesImage.loadBitmap("..\\res\\gates.bmp")) {
    printf("Error loading gates.bmp\n");
    return;
  }
  if (!gatesOpenImage.loadBitmap("..\\res\\gatesopen.bmp")) {
    printf("Error loading gatesopen.bmp\n");
    return;
  }

  wallsImage.createTexture(renderer);
  wallsImageDark.createTexture(renderer);
  gatesImage.createTexture(renderer);
  gatesOpenImage.createTexture(renderer);

  if (!gunImage.loadBitmap("..\\res\\gun1a.bmp")) {
    printf("Error loading gun image\n");
    return;
  }
  Uint32 colorKey = SDL_MapRGB(gunImage.getSurface()->format,152,0,136);
  SDL_SetColorKey( gunImage.getSurface(), true, colorKey );
  gunImage.createTexture(renderer);

  SDL_SetColorKey( gatesImage.getSurface(), true, colorKey );
  SDL_SetColorKey( gatesOpenImage.getSurface(), true, colorKey );

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
  spriteFilenames[ SpriteTypeGates ] = "gates.bmp";
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

  //Initialize SDL_mixer
  if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 )
  {
    printf( "Mix_OpenAudio failed. SDL_mixer Error: %s\n", Mix_GetError() );
    return;
  }

  projectileFireSound = Mix_LoadWAV( "..\\res\\iceball.wav" );
  projectileExplodeSound = Mix_LoadWAV( "..\\res\\explode.wav" );
  doorOpenSound = Mix_LoadWAV( "..\\res\\door-09.wav" );
  doorCloseSound = Mix_LoadWAV( "..\\res\\door-08.wav" );
  Mix_VolumeChunk(projectileFireSound, MIX_MAX_VOLUME/2);
  Mix_VolumeChunk(projectileExplodeSound, MIX_MAX_VOLUME/3);
  Mix_VolumeChunk(doorOpenSound, MIX_MAX_VOLUME);
  Mix_VolumeChunk(doorCloseSound, MIX_MAX_VOLUME);

  this->running = 1 ;
  printHelp();
  run();
}

void Game::draw() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE );
    SDL_RenderClear(renderer);

    //memset(screenSurface->pixels,0,sizeof(Uint32)*displayWidth*displayHeight);

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
  if (!drawWeaponOn) {
    return;
  }
  float gunScale = displayHeight / 320.0;
  SDL_Rect dstRect;
  SDL_Surface* gunSurface = gunImage.getSurface();
  dstRect.w = gunSurface->w * gunScale;
  dstRect.h = gunSurface->h * gunScale;
  dstRect.x = (displayWidth - dstRect.w) / 2;
  dstRect.y = displayHeight - dstRect.h;
  SDL_BlitScaled(gunSurface, NULL, screenSurface, &dstRect);
}

void Game::fpsChanged( int fps ) {
    char szFps[ 128 ] ;
    sprintf( szFps, "rayHits=%d FPS=%d Cell=(%d,%d,%d) Rot=(%d deg)",
     rayHitsCount, fps,
     (int)(player.x/TILE_SIZE), (int)(player.y/TILE_SIZE),
     (int)(player.z/TILE_SIZE),
     (int)(player.rot*(180/M_PI))
    );
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

void Game::printHelp() {
  printf("======== SDL2 Raycasting Demo =======\n");
  printf("=== https://github.com/andrew-lim ===\n");
  printf("Controls:\n"
         "Arrow keys or WASD to move\n"
         "R      - reset player/sprite positions\n"
         "Enter  - Open Doors\n"
         "Q      - Strafe Left\n"
         "E      - Strafe Right\n"
         "LCtrl  - Jump\n"
         "Space  - Shoot\n"
         "PageUp - Look Up\n"
         "PageDn - Look Down\n"
         "M      - Toggle minimap\n"
         "T      - Toggle textured floors\n"
         "C      - Toggle skybox and ceilings\n"
         "G      - Toggle weapon visibility\n"
         "F      - Toggle fog\n"
         "H      - Print this message again\n"
         "See config.ini for more settings.\n"
         "=====================================\n");
}

void Game::update(float timeElapsed) {
  const Uint8* keyboard = SDL_GetKeyboardState(NULL);

  if (keyboard[SDL_SCANCODE_UP] || keyboard[SDL_SCANCODE_W]) {
    // up, move player forward
    player.speed = 1;
  }
  else if (keyboard[SDL_SCANCODE_DOWN] || keyboard[SDL_SCANCODE_S]) {
    // down, move player backward
    player.speed = -1;
  }
  else {
    player.speed = 0; // stop moving
  }

  if (keyboard[SDL_SCANCODE_LEFT] || keyboard[SDL_SCANCODE_A]) {
    // left, rotate player left
    player.dir = -1;
  }
  else if (keyboard[SDL_SCANCODE_RIGHT] || keyboard[SDL_SCANCODE_D]) {
    // right, rotate player right
    player.dir = 1;
  }
  else {
    player.dir = 0; // stop rotating
  }

  float timeBasedFactor = timeElapsed / UPDATE_INTERVAL;
  static const float MAX_PITCH = displayHeight/2;
  const int PITCH_SPEED = 10 * timeBasedFactor;

  if (keyboard[SDL_SCANCODE_PAGEDOWN]) {
    pitch -= PITCH_SPEED;
    if (pitch < -MAX_PITCH) {
      pitch = -MAX_PITCH;
    }
  }
  else if (keyboard[SDL_SCANCODE_PAGEUP]) {
    pitch += PITCH_SPEED;
    if (pitch > MAX_PITCH) {
      pitch = MAX_PITCH;
    }
  }
  else if (pitch<0) {
    pitch += PITCH_SPEED;
    if (pitch > 0) {
      pitch = 0;
    }
  }
  else if (pitch>0) {
    pitch -= PITCH_SPEED;
    if (pitch < 0) {
      pitch = 0;
    }
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
  const Uint8 *state = SDL_GetKeyboardState(NULL);

  float timeBasedFactor = elapsedTime / UPDATE_INTERVAL;
  float moveStep = player.speed * player.moveSpeed * timeBasedFactor;
  player.rot += -player.dir * player.rotSpeed  * timeBasedFactor;
  float newX = player.x +  cosine(player.rot) * moveStep;
  float newY = player.y + -sine(player.rot) * moveStep;
  float newZ = player.z;

  float strafeMagnitude = 0;
  float strafeRotation = 0;
  bool strafeLeft = state[SDL_SCANCODE_Q];
  bool strafeRight = state[SDL_SCANCODE_E];

  // Strafing left/right is just adding/subtracting 90 degrees to the
  // player's rotation
  if (strafeLeft) {
    strafeMagnitude = player.moveSpeed * timeBasedFactor;
    strafeRotation  = player.rot + M_PI / 2;
  }
  if (strafeRight) {
    strafeMagnitude = player.moveSpeed * timeBasedFactor;
    strafeRotation  = player.rot - M_PI / 2;
  }

  newX += cos(strafeRotation) * strafeMagnitude;
  newY += -sin(strafeRotation) * strafeMagnitude;

  // Jump Logic
  float jumpSpeed = (TILE_SIZE/40) * timeBasedFactor;
  if (player.jumping) {
    // Going up
    if (player.heightJumped<HALF_JUMP_DISTANCE) {
      player.heightJumped += jumpSpeed;
      newZ += jumpSpeed;
    }
    // Falling down
    else if (player.heightJumped<MAX_JUMP_DISTANCE) {
      player.heightJumped += jumpSpeed;
      newZ -= jumpSpeed;
      if (newZ<0) {
        newZ = 0;
        player.jumping = false;
        player.heightJumped = 0;
      }
    }
    // Not jumping anymore
    else {
      player.jumping = false;
      player.heightJumped = 0;
    }
  }
  else {
    newZ -= jumpSpeed;
    float slopeHeight = slopeHeightAt(newX, newY);
    if (newZ<slopeHeight) {
      newZ = slopeHeight;
      player.jumping = false;
      player.heightJumped = 0;
    }
    // New vertical position hits a wall, try to step over it
    if (playerInWall(newX, newY, newZ)) {
      float maximumStep = TILE_SIZE/4;
      int tileZ = newZ / TILE_SIZE;
      float stepZ = (tileZ+1) * TILE_SIZE;
      bool stepOK = false;
      if (!playerInWall(newX, newY, stepZ)) {
        if (stepZ-player.z <= maximumStep) {
          newZ = stepZ; // Step success
          stepOK = true;
        }
      }
      if (!stepOK) {
        newZ = player.z; // Step failed
      }
    }
    if (newZ < 0) {
      newZ = 0;
    }
  }

  // Movement blocked
  if (playerInWall(newX, newY, newZ)) {
    // Try moving east west only
    if (!playerInWall(newX, player.y, player.z)) {
      player.x = newX;
    }
    // Try moving north south only
    if (!playerInWall(player.x, newY, player.z)) {
      player.y = newY;
    }
    // Try moving vertically only
    if (!playerInWall(player.x, player.y, newZ)) {
      player.z = newZ;
    }
    // Probably bumped our head on a ceiling, start falling
    else {
      if (player.jumping && player.heightJumped<HALF_JUMP_DISTANCE) {
        player.heightJumped = HALF_JUMP_DISTANCE;
      }
    }
  }
  // Movement okay!
  else {
    player.x = newX;
    player.y = newY;
    player.z = newZ;
  }
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
      float newX = newProjectile.x+ cosine(newProjectile.rot) *player.moveSpeed;
      float newY = newProjectile.y+ -sine(newProjectile.rot) *player.moveSpeed;
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
    float newX = projectile.x +  cosine(projectile.rot) * moveStep;
    float newY = projectile.y + -sine(projectile.rot) * moveStep;
    float newZ = projectile.z;
    int wallX = (int)newX / TILE_SIZE;
    int wallY = (int)newY / TILE_SIZE;

    // add a bit of height to the Z because projectile is floating
    int wallZ = (int)(newZ+TILE_SIZE/4) / TILE_SIZE;

    bool wallHit = false;
    bool outOfBounds = false;
    bool hitOtherSprite = false;
    if (isWallCell(wallX, wallY, wallZ)) {
      newX = projectile.x +  cosine(projectile.rot) * moveStep/4;
      newY = projectile.y + -sine(projectile.rot) * moveStep/4;
      wallX = newX / TILE_SIZE;
      wallY = newY / TILE_SIZE;
      if (isWallCell(wallX, wallY, wallZ)) {
        if (!projectile.cleanup) {
          addProjectile(SpriteTypeProjectileSplash, projectile.x, projectile.y,
                        projectile.z,
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
                            projectile.y, projectile.z, TILE_SIZE,
                            projectile.rot);
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

float Game::slopeHeightAt(float worldX, float worldY)
{
  if (worldX<0 || worldY<0 ||
      worldX>=TILE_SIZE*MAP_WIDTH || worldY>=TILE_SIZE*MAP_HEIGHT )
  {
    return 0.0f;
  }
  for (size_t i=0; i<slopedWalls.size(); ++i) {
    ThickWall* slopedWall = slopedWalls[i];
    if (slopedWall->containsPoint(worldX, worldY) && !slopedWall->invertedSlope)
    {
      float floorX = worldX - slopedWall->x;
      float floorY = worldY - slopedWall->y;
      if (slopedWall->slopeType == SLOPE_TYPE_WEST_EAST) {
        return slopedWall->startHeight + slopedWall->slope * floorX;
      }
      if (slopedWall->slopeType == SLOPE_TYPE_NORTH_SOUTH) {
        return slopedWall->startHeight + slopedWall->slope * floorY;
      }
    }
  }
  int cellX = worldX / TILE_SIZE;
  int cellY = worldY / TILE_SIZE;
  if (raycaster3D.safeCellAt(cellX, cellY, 0, 0)) {
    return TILE_SIZE;
  }
  return 0.0f;
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
    RayHit& rayHit = rayHits[i];
    if (rayHit.wallType && rayHit.level==0){
      drawRay(rayHit.x, rayHit.y);
    }
  }
}

Uint32 Game::fogPixel( Uint32 pixel, float distance ) {
  float fogFactor = distance / FOG_START_DISTANCE;
  if ( fogFactor <= 1 ) {
    return pixel;
  }
  Uint32 pixel2 = pixel;
  Uint8* pixels = (Uint8*)(void*)&pixel2;
  // Little endian - access RGB in reverse order
  pixels[ 0 ] = std::min(FOG_B, pixels[ 0 ] * fogFactor); // Blue
  pixels[ 1 ] = std::min(FOG_G, pixels[ 1 ] * fogFactor); // Green
  pixels[ 2 ] = std::min(FOG_R, pixels[ 2 ] * fogFactor); // Red
  return pixel2;
}

void Game::fogWallStrip( SDL_Rect* dstrect, float distance ) {
  int startY = dstrect->y;
  int endY = dstrect->y + dstrect->h;
  Uint32* screenPixels = (Uint32*) screenSurface->pixels;
  int startX = dstrect->x;
  int endX = startX + stripWidth;
  float fogFactor = distance / FOG_START_DISTANCE;
  if ( fogFactor <= 1 ) {
    return;
  }
  for ( int x=startX; x>=0 && x<endX && x<displayWidth;  ++x ) {
    for ( int y=startY; y>=0 && y<endY && y<displayHeight; ++y ) {
      Uint32 pixel = screenPixels[ x + y * displayWidth ];
      Uint8* pixel8 = (Uint8*)(void*)&pixel;
      pixel8[ 0 ] = std::min(FOG_B, pixel8[ 0 ] * fogFactor);
      pixel8[ 1 ] = std::min(FOG_G, pixel8[ 1 ] * fogFactor);
      pixel8[ 2 ] = std::min(FOG_R, pixel8[ 2 ] * fogFactor);
      screenPixels[ x + y * displayWidth ] = pixel;
    }
  }
}

void Game::drawFloor(vector<RayHit>& rayHits)
{
  // If floor texture mapping off, just draw a solid color
  if (!drawTexturedFloorOn) {
    SDL_Rect rc;
    rc.x = 0;
    rc.y = displayHeight/2;
    rc.w = displayWidth;
    rc.h = displayHeight/2;
    SDL_FillRect(screenSurface, &rc,SDL_MapRGB(screenSurface->format,52,158,0));
    return;
  }

  vector<bool> stripsDrawn;
  stripsDrawn.resize( rayCount + 1 );

  Uint32* screenPixels = (Uint32*) screenSurface->pixels;
  for (int i=0; i<(int)rayHits.size(); ++i) {
    RayHit& rayHit = rayHits[i];

    // Must be a wall, not a sprite
    if (!rayHit.wallType) {
      continue;
    }

    // Only draw below lowest wall
    if (rayHit.level>0) {
      continue;
    }

    // Ignore doors
    if (Raycaster::isDoor(rayHit.wallType)) {
      continue;
    }

    // Already drew this strip. This should work because we draw the farthest
    // ground walls first.
    if (skipDrawnFloorStrips) {
      if (stripsDrawn[rayHit.strip]) {
        continue;
      }
      stripsDrawn[rayHit.strip] = true;
    }

    int wallScreenHeight = Raycaster::stripScreenHeight(viewDist,
                                                        rayHit.correctDistance,
                                                        TILE_SIZE);

    float centerPlane = displayHeight / 2;
    float eyeHeight = TILE_SIZE/2 + player.z;
    int screenX = rayHit.strip * stripWidth;
    int screenY = (displayHeight - wallScreenHeight)/2 + wallScreenHeight;
    if (screenY < centerPlane) {
      screenY = centerPlane;
    }

    // Specifies many times a texture is repeated on one side. E.g.
    // If set to 2, a texture will repeat 4 times (because 2x2) inside itself.
    int textureRepeat = 2;

    const float cosFactor = 1/cos(player.rot-rayHit.rayAngle);

    for (; screenY<displayHeight-pitch; screenY++)
    {
      float ratio= (eyeHeight) /(screenY-centerPlane);
      float straightDistance = viewDist * ratio;
      float diagonalDistance = straightDistance * cosFactor;

      float xEnd = (diagonalDistance *  cosine(rayHit.rayAngle));
      float yEnd = (diagonalDistance * -sine(rayHit.rayAngle));

      xEnd += player.x;
      yEnd += player.y;
      int x = (int)(xEnd*textureRepeat) % TILE_SIZE;
      int y = (int)(yEnd*textureRepeat) % TILE_SIZE;
      int tileX = xEnd / TILE_SIZE;
      int tileY = yEnd / TILE_SIZE;
      if ( x<0 || y<0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT ) {
        continue;
      }
      int floorTileType = g_floormap[ tileY ][ tileX ];
      bool wallTextureExists = floorTileType>=0 &&
                               floorTileType<(int)floorCeilingBitmaps.size();
      if (!wallTextureExists) {
        continue;
      }
      Bitmap& bitmap = floorCeilingBitmaps[ floorTileType ];
      Uint32* pix = (Uint32*)bitmap.getPixels();
      if (!pix) {
        continue;
      }
      int textureX = (float) x / TILE_SIZE * TEXTURE_SIZE;
      int textureY = (float) y / TILE_SIZE * TEXTURE_SIZE;
      int dstPixel = screenX + (screenY+pitch) * displayWidth;
      int srcPixel = textureY * bitmap.getWidth() + textureX;
      bool pixelOK = srcPixel>=0 && dstPixel>=0 &&
                     srcPixel<bitmap.getWidth()*bitmap.getHeight() &&
                     dstPixel<displayWidth*displayHeight;

      if (pixelOK) {
        Uint32 srcPixelValue = pix[srcPixel];
        if (fogOn) {
          srcPixelValue = fogPixel(srcPixelValue, diagonalDistance);
        }
        switch (stripWidth) {
          case 4:
            screenPixels[dstPixel+3] = srcPixelValue;
          case 3:
            screenPixels[dstPixel+2] = srcPixelValue;
          case 2:
            screenPixels[dstPixel+1] = srcPixelValue;
          default:
            screenPixels[dstPixel] = srcPixelValue;
            break;
        }
      }
    }
  }
}

void Game::drawSkyboxAndHighestCeiling(vector<RayHit>& rayHits)
{
  if (!drawCeilingOn) {
    SDL_Rect rc;
    rc.x = 0;
    rc.y = 0;
    rc.w = displayWidth;
    rc.h = displayHeight;
    SDL_FillRect(screenSurface, &rc,
                 SDL_MapRGB(screenSurface->format,139, 185, 249));
    return;
  }

  vector<int> drawnStrips;
  drawnStrips.resize( rayCount + 1 );

  Uint32* screenPixels = (Uint32*) screenSurface->pixels;
  for (int i=0; i<(int)rayHits.size(); i++) {
    RayHit& rayHit = rayHits[i];
      // Only draw above furthest wall
    if (!rayHit.wallType) {
      continue;
    }

    // Ignore doors
    if (Raycaster::isDoor(rayHit.wallType)) {
      continue;
    }

    // Only draw above highest wall
    if (rayHit.level!=highestCeilingLevel-1) {
      if (raycaster3D.safeCellAt(rayHit.wallX, rayHit.wallY, rayHit.level+1)) {
        continue;
      }
    }

    int wallScreenHeight = Raycaster::stripScreenHeight(viewDist,
                                                        rayHit.correctDistance,
                                                        TILE_SIZE);
    float playerScreenZ = Raycaster::stripScreenHeight(viewDist,
                                                     rayHit.correctDistance,
                                                     player.z);

    int screenX = rayHit.strip * stripWidth;
    int screenY = (displayHeight - wallScreenHeight)/2 + playerScreenZ + pitch;
    if (screenY >= displayHeight) {
      screenY = displayHeight-1;
    }
    float eyeHeight = TILE_SIZE / 2 + player.z;
    float highestCeilingTop = highestCeilingLevel*TILE_SIZE;

    // Player can't see above the highest ceiling
    if (eyeHeight>=highestCeilingTop) {
      return;
    }

    if (skipDrawnSkyboxStrips) {
      // Only draw this strip if we have not drawn it or we drew at a
      // higher point previously
      if (drawnStrips[rayHit.strip] < screenY) {
        drawnStrips[rayHit.strip] = screenY;
      }
      else {
        continue;
      }
    }

    // Draw skybox first
    for (;screenY>=0;screenY--)
    {
      int dstPixel = screenX + (screenY) * displayWidth;
      if (dstPixel >= displayWidth*displayHeight) {
        continue;
      }

      const int PIXEL_LENGTH = SKYBOX_WIDTH * SKYBOX_HEIGHT;
      int skyboxY = (screenY / (displayHeight/2.0f) * SKYBOX_HEIGHT);
      Uint32* pix2 = (Uint32*) skyboxSurface->pixels;
      int skyboxX = (float)screenX / displayWidth * SKYBOX_WIDTH;
      float rotation = player.rot;
      int skyboxOffsetX = -((rotation/TWO_PI)*SKYBOX_WIDTH)*4;
      skyboxX += skyboxOffsetX;
      int offset = (skyboxX%SKYBOX_WIDTH +skyboxY*SKYBOX_WIDTH);
      if (offset < 0) {
        offset = 0;
      }
      if (offset >= PIXEL_LENGTH) {
        offset = PIXEL_LENGTH - 1;
      }
      bool pixelOK = dstPixel >=0 && dstPixel < displayWidth * displayHeight;
      if (!pixelOK) {
        continue;
      }
      Uint32 pixel = pix2[ offset ];
      switch (stripWidth) {
        case 4:
          screenPixels[dstPixel+3] = pixel;
        case 3:
          screenPixels[dstPixel+2] = pixel;
        case 2:
          screenPixels[dstPixel+1] = pixel;
        default:
          screenPixels[dstPixel] = pixel;
          break;
      }
    }
  }

  vector<int> drawnCeilingStrips;
  drawnCeilingStrips.resize( rayCount + 1 );

  for (int i=0; i<(int)rayHits.size(); i++) {
    RayHit& rayHit = rayHits[i];
      // Only draw above furthest wall
    if (!rayHit.wallType) {
      continue;
    }

    // Ignore doors
    if (Raycaster::isDoor(rayHit.wallType)) {
      continue;
    }

    // Only draw above highest wall
    if (rayHit.level!=highestCeilingLevel-1) {
      if (raycaster3D.safeCellAt(rayHit.wallX, rayHit.wallY, rayHit.level+1)) {
        continue;
      }
    }

    int wallScreenHeight = Raycaster::stripScreenHeight(viewDist,
                                                        rayHit.correctDistance,
                                                        TILE_SIZE);
    float playerScreenZ = Raycaster::stripScreenHeight(viewDist,
                                                     rayHit.correctDistance,
                                                     player.z);
    int screenX = rayHit.strip * stripWidth;
    float screenY = (displayHeight - wallScreenHeight)/2 + playerScreenZ;
    if (screenY >= displayHeight/2) {
      screenY = displayHeight/2-1;
    }

    if (skipDrawnHighestCeilingStrips) {
      // Only draw this strip if we have not drawn it or we drew at a
      // higher point previously
      if (drawnCeilingStrips[rayHit.strip] < screenY) {
        drawnCeilingStrips[rayHit.strip] = screenY;
      }
      else {
        continue;
      }
    }

    float eyeHeight = TILE_SIZE / 2 + player.z;
    float centerPlane = displayHeight / 2;
    float highestCeilingTop = highestCeilingLevel*TILE_SIZE;

    // Player can't see above the highest ceiling
    if (eyeHeight>=highestCeilingTop) {
      return;
    }

    const float cosFactor = 1/cos(player.rot-rayHit.rayAngle);

    // Draw highest ceiling
    for (;screenY>=0-pitch;screenY--)
    {
      float ceilingHeight = TILE_SIZE * highestCeilingLevel;
      float ratio = (ceilingHeight - eyeHeight) / (centerPlane - screenY);
      float straightDistance = viewDist * ratio;
      float diagonalDistance = straightDistance * cosFactor;

      float xEnd = (diagonalDistance *  cosine(rayHit.rayAngle));
      float yEnd = (diagonalDistance * -sine(rayHit.rayAngle));
      xEnd += player.x;
      yEnd += player.y;

      bool outOfBounds = xEnd<0 || xEnd>=MAP_WIDTH*TILE_SIZE ||
                         yEnd<0 || yEnd>=MAP_HEIGHT*TILE_SIZE;
      int x = (int)(xEnd) % TILE_SIZE;
      int y = (int)(yEnd) % TILE_SIZE;
      int tileX = xEnd / TILE_SIZE;
      int tileY = yEnd / TILE_SIZE;
      int textureX = (float) x / TILE_SIZE * TEXTURE_SIZE;
      int textureY = (float) y / TILE_SIZE * TEXTURE_SIZE;
      int tileType = outOfBounds ? 0 : g_ceilingmap[ tileY ][ tileX ];
      int dstPixel = screenX + (screenY+pitch) * displayWidth;
      if (dstPixel >= displayWidth*displayHeight) {
        continue;
      }

      // Draw highest ceiling if it is not out of bounds and above the center
      if (!outOfBounds && tileType) {
        Bitmap& bitmap = floorCeilingBitmaps[tileType];
        Uint32* pix = (Uint32*)bitmap.getPixels();
        if (!pix) {
          continue;
        }
        int srcPixel = textureY * TEXTURE_SIZE + textureX;
        switch (stripWidth) {
          case 4:
            screenPixels[dstPixel+3] = pix[srcPixel];
          case 3:
            screenPixels[dstPixel+2] = pix[srcPixel];
          case 2:
            screenPixels[dstPixel+1] = pix[srcPixel];
          default:
            screenPixels[dstPixel] = pix[srcPixel];
            break;
        }
      }
    }
  }
}

void Game::drawWallBottom(RayHit& rayHit, int wallScreenHeight,
                          float playerScreenZ)
{
  int screenX = rayHit.strip * stripWidth;
  float eyeHeight = TILE_SIZE/2 + player.z;
  float centerPlane = displayHeight / 2;
  bool wasInWall = false;
  const float cosFactor = 1/cos(player.rot-rayHit.rayAngle);

  // Older slower loop - render upwards from center plane
  // for (int screenY=centerPlane; screenY>0-pitch; screenY--)
  // {

  // Find bottom of wall, and render downwards towards center plane
  int screenY = (displayHeight-wallScreenHeight)/2 + wallScreenHeight;
  screenY = screenY - rayHit.level*wallScreenHeight + playerScreenZ - pitch;
  if (screenY < 0 - pitch ) {
    screenY = 0 - pitch;
  }
  for (; screenY<centerPlane; screenY++)
  {
    float ceilingHeight = TILE_SIZE * (rayHit.level);
    float ratio = (ceilingHeight - eyeHeight) / (centerPlane - screenY);
    float straightDistance = viewDist * ratio;
    float diagonalDistance = straightDistance * cosFactor;

    float xEnd = (diagonalDistance *  cosine(rayHit.rayAngle));
    float yEnd = (diagonalDistance * -sine(rayHit.rayAngle));
    xEnd += player.x;
    yEnd += player.y;
    int x = (int)(xEnd) % TILE_SIZE;
    int y = (int)(yEnd) % TILE_SIZE;
    int wallX = xEnd / TILE_SIZE;
    int wallY = yEnd / TILE_SIZE;

    bool wallTextureExists = rayHit.wallType < (int)floorCeilingBitmaps.size();
    bool outOfBounds = x < 0 || y < 0 || x>MAP_WIDTH*TILE_SIZE ||
                       y>MAP_HEIGHT*TILE_SIZE;
    bool sameWall = wallX==rayHit.wallX && wallY==rayHit.wallY;
    if (outOfBounds || !wallTextureExists || !sameWall ||
        !raycaster3D.cellAt(wallX,wallY,rayHit.level)) {
      if (wasInWall) {
        return;
      }
      continue;
    }
    Bitmap& bitmap = floorCeilingBitmaps[ rayHit.wallType ];
    Uint32* pix = (Uint32*)bitmap.getPixels();
    if (!pix) {
      continue;
    }
    int textureX = (float) x / TILE_SIZE * TEXTURE_SIZE;
    int textureY = (float) y / TILE_SIZE * TEXTURE_SIZE;
    Uint32* screenPixels = (Uint32*) screenSurface->pixels;
    int dstPixel = screenX + (screenY+pitch) * displayWidth;
    int srcPixel = textureY * bitmap.getWidth() + textureX;
    bool pixelOK = srcPixel>=0 && dstPixel>=0 &&
                   srcPixel<bitmap.getWidth()*bitmap.getHeight() &&
                   dstPixel<displayWidth*displayHeight;
    if (pixelOK) {
      wasInWall = true;
      switch (stripWidth) {
        case 4:
          screenPixels[dstPixel+3] = pix[srcPixel];
        case 3:
          screenPixels[dstPixel+2] = pix[srcPixel];
        case 2:
          screenPixels[dstPixel+1] = pix[srcPixel];
        default:
          screenPixels[dstPixel] = pix[srcPixel];
          break;
      }
    }
  }
}

void Game::drawWallTop(RayHit& rayHit, int wallScreenHeight,float playerScreenZ)
{
  Uint32* screenPixels = (Uint32*) screenSurface->pixels;
  float eyeHeight = TILE_SIZE/2 + player.z;
  float wallTop = (rayHit.level+1)*TILE_SIZE;
  float centerPlane = displayHeight/2;
  int textureRepeat = 1;
  bool wasInWall = false;
  int screenX = rayHit.strip * stripWidth;
  const float cosFactor = 1/cos(player.rot-rayHit.rayAngle);

  // Older slower loop, render downwards from center plane
  // for (int screenY=centerPlane; screenY<displayHeight-pitch; screenY++)
  // {

  // Find top of wall, and render upwards towards center plane
  int screenY = (displayHeight-wallScreenHeight)/2;
  screenY = screenY - rayHit.level * wallScreenHeight + playerScreenZ - pitch;
  if (screenY > displayHeight - pitch ) {
    screenY = displayHeight - pitch;
  }
  for (; screenY>=centerPlane; screenY--)
  {
    float ratio= (eyeHeight - wallTop) / (screenY-centerPlane);
    float straightDistance = viewDist * ratio;
    float diagonalDistance = straightDistance * cosFactor;

    float xEnd = (diagonalDistance *  cosine(rayHit.rayAngle));
    float yEnd = (diagonalDistance * -sine(rayHit.rayAngle));
    xEnd += player.x;
    yEnd += player.y;
    int x = (int)(xEnd*textureRepeat) % TILE_SIZE;
    int y = (int)(yEnd*textureRepeat) % TILE_SIZE;
    int wallX = xEnd / TILE_SIZE;
    int wallY = yEnd / TILE_SIZE;

    bool wallTextureExists = rayHit.wallType < (int)floorCeilingBitmaps.size();
    bool outOfBounds = x < 0 || y < 0 || x>MAP_WIDTH*TILE_SIZE ||
                       y>MAP_HEIGHT*TILE_SIZE;
    bool sameWall = wallX==rayHit.wallX && wallY==rayHit.wallY;
    if (outOfBounds || !wallTextureExists || !sameWall ||
        !raycaster3D.cellAt(wallX,wallY,rayHit.level)) {
      if (wasInWall) {
        return;
      }
      continue;
    }
    Bitmap& bitmap = floorCeilingBitmaps[ rayHit.wallType ];
    Uint32* pix = (Uint32*)bitmap.getPixels();
    if (!pix) {
      continue;
    }
    int textureX = (float) x / TILE_SIZE * TEXTURE_SIZE;
    int textureY = (float) y / TILE_SIZE * TEXTURE_SIZE;
    int dstPixel = screenX + (screenY+pitch) * displayWidth;
    int srcPixel = textureY * bitmap.getWidth() + textureX;
    bool pixelOK = srcPixel>=0 && dstPixel>=0 &&
                   srcPixel<bitmap.getWidth()*bitmap.getHeight() &&
                   dstPixel<displayWidth*displayHeight;
    if (pixelOK) {
      wasInWall = true;
      switch (stripWidth) {
        case 4:
          screenPixels[dstPixel+3] = pix[srcPixel];
        case 3:
          screenPixels[dstPixel+2] = pix[srcPixel];
        case 2:
          screenPixels[dstPixel+1] = pix[srcPixel];
        default:
          screenPixels[dstPixel] = pix[srcPixel];
          break;
      }
    }
  }
}

void Game::drawThinWallTop(RayHit& rayHit, int wallScreenHeight)
{
  if (!rayHit.thinWall || !rayHit.thinWall->thickWall) {
    return;
  }

  Uint32* screenPixels = (Uint32*) screenSurface->pixels;
  float eyeHeight = TILE_SIZE/2 + player.z;
  float wallTop = rayHit.thinWall->z + rayHit.thinWall->height;
  float centerPlane = displayHeight/2;
  bool wasInWall = false;
  int screenX = rayHit.strip * stripWidth;
  const float cosFactor = 1/cos(player.rot-rayHit.rayAngle);
  for (int screenY=centerPlane; screenY<displayHeight-pitch; screenY++)
  {
    float ratio= (eyeHeight - wallTop) / (screenY-centerPlane);
    float straightDistance = viewDist * ratio;
    float diagonalDistance = straightDistance * cosFactor;

    float xEnd = (diagonalDistance *  cosine(rayHit.rayAngle));
    float yEnd = (diagonalDistance * -sine(rayHit.rayAngle));
    xEnd += player.x;
    yEnd += player.y;
    int x = (int)(xEnd) % TILE_SIZE;
    int y = (int)(yEnd) % TILE_SIZE;

    int textureID = rayHit.thinWall->thickWall->ceilingTextureID;
    bool wallTextureExists = textureID < (int)floorCeilingBitmaps.size();
    bool outOfBounds = x < 0 || y < 0 || x>MAP_WIDTH*TILE_SIZE ||
                       y>MAP_HEIGHT*TILE_SIZE;
    if (outOfBounds || !wallTextureExists) {
      continue;
    }
    if (isinf(xEnd) || isinf(yEnd)) {
      continue;
    }
    if (!rayHit.thinWall->thickWall->containsPoint(xEnd,yEnd)) {
      if (wasInWall) {
        return;
      }
      continue;
    }
    Bitmap& bitmap = floorCeilingBitmaps[ textureID ];
    Uint32* pix = (Uint32*)bitmap.getPixels();
    if (!pix) {
      continue;
    }
    int textureX = (float) x / TILE_SIZE * TEXTURE_SIZE;
    int textureY = (float) y / TILE_SIZE * TEXTURE_SIZE;
    int dstPixel = screenX + (screenY+pitch) * displayWidth;
    int srcPixel = textureY * bitmap.getWidth() + textureX;
    bool pixelOK = srcPixel>=0 && dstPixel>=0 &&
                   srcPixel<bitmap.getWidth()*bitmap.getHeight() &&
                   dstPixel<displayWidth*displayHeight;
    if (pixelOK) {
      wasInWall = true;
      switch (stripWidth) {
        case 4:
          screenPixels[dstPixel+3] = pix[srcPixel];
        case 3:
          screenPixels[dstPixel+2] = pix[srcPixel];
        case 2:
          screenPixels[dstPixel+1] = pix[srcPixel];
        default:
          screenPixels[dstPixel] = pix[srcPixel];
          break;
      }
    }
  }
}

void Game::drawThinWallBottom(RayHit& rayHit, int wallScreenHeight)
{
  if (!rayHit.thinWall || !rayHit.thinWall->thickWall) {
    return;
  }

  Uint32* screenPixels = (Uint32*) screenSurface->pixels;
  float eyeHeight = TILE_SIZE/2 + player.z;
  float wallBottom = rayHit.thinWall->z;
  float centerPlane = displayHeight/2;
  bool wasInWall = false;
  int screenX = rayHit.strip * stripWidth;
  const float cosFactor = 1/cos(player.rot-rayHit.rayAngle);
  for (int screenY=centerPlane; screenY>=0-pitch; screenY--)
  {
    float ratio = (wallBottom - eyeHeight) / (centerPlane - screenY);
    float straightDistance = viewDist * ratio;
    float diagonalDistance = straightDistance * cosFactor;

    float xEnd = (diagonalDistance *  cosine(rayHit.rayAngle));
    float yEnd = (diagonalDistance * -sine(rayHit.rayAngle));
    xEnd += player.x;
    yEnd += player.y;
    int x = (int)(xEnd) % TILE_SIZE;
    int y = (int)(yEnd) % TILE_SIZE;

    int textureID = rayHit.thinWall->thickWall->floorTextureID;
    bool wallTextureExists = textureID < (int)floorCeilingBitmaps.size();
    bool outOfBounds = x < 0 || y < 0 || x>MAP_WIDTH*TILE_SIZE ||
                       y>MAP_HEIGHT*TILE_SIZE;
    if (outOfBounds || !wallTextureExists) {
      continue;
    }
    if (isinf(xEnd) || isinf(yEnd)) {
      continue;
    }
    if (!rayHit.thinWall->thickWall->containsPoint(xEnd,yEnd)) {
      if (wasInWall) {
        return;
      }
      continue;
    }
    Bitmap& bitmap = floorCeilingBitmaps[ textureID ];
    Uint32* pix = (Uint32*)bitmap.getPixels();
    if (!pix) {
      continue;
    }
    int textureX = (float) x / TILE_SIZE * TEXTURE_SIZE;
    int textureY = (float) y / TILE_SIZE * TEXTURE_SIZE;
    int dstPixel = screenX + (screenY+pitch) * displayWidth;
    int srcPixel = textureY * bitmap.getWidth() + textureX;
    bool pixelOK = srcPixel>=0 && dstPixel>=0 &&
                   srcPixel<bitmap.getWidth()*bitmap.getHeight() &&
                   dstPixel<displayWidth*displayHeight;
    if (pixelOK) {
      wasInWall = true;
      switch (stripWidth) {
        case 4:
          screenPixels[dstPixel+3] = pix[srcPixel];
        case 3:
          screenPixels[dstPixel+2] = pix[srcPixel];
        case 2:
          screenPixels[dstPixel+1] = pix[srcPixel];
        default:
          screenPixels[dstPixel] = pix[srcPixel];
          break;
      }
    }
  }
}

bool Game::drawSlope(RayHit& rayHit, float playerScreenZ)
{
  Uint32* screenPixels = (Uint32*) screenSurface->pixels;
  RayHit sibling;

  // No siblings means player is inside the slope. The sibling is behind
  // the player, so we do a backwards raycast to find it.
  if (rayHit.siblingDistance == 0) {
    if (raycaster3D.findSiblingAtAngle(sibling, *rayHit.thinWall,
                                       rayHit.rayAngle-M_PI, player.rot,
                                       rayHit.strip, player.x, player.y,
                                       player.x, player.y))
    {
      // Sibling distance to player is negated because it is behind.
      sibling.correctDistance = -sibling.distance;
      rayHit.copySibling(sibling);
    }
  }

  // Only draw slope if current wall is further than sibling wall
  if (!rayHit.siblingDistance ||
       rayHit.correctDistance < rayHit.siblingCorrectDistance) {
    return false;
  }

  // Cross section values of current strip
  float farWallX = rayHit.correctDistance;
  float farWallY = rayHit.thinWall->z + rayHit.wallHeight;
  float nearWallX = rayHit.siblingCorrectDistance;
  float nearWallY = rayHit.siblingThinWallZ + rayHit.siblingWallHeight;
  float eyeX = 0; // relative to player eye, so always 0
  float eyeY = TILE_SIZE/2 + player.z;

  float centerPlane = displayHeight / 2;
  float cosFactor = 1/cos(player.rot-rayHit.rayAngle);
  float screenX = rayHit.strip * stripWidth;
  float wasInWall = false; // used to stop drawing early

  SDL_Rect rc = stripScreenRect(rayHit, rayHit.wallHeight);
  float screenY = rc.y + playerScreenZ;

  // Raycast vertically from top to bottom to find slope intersection
  for (; screenY<displayHeight-pitch; screenY++) {
    float wallTop = 0;
    bool hitSlope = false;

    // Angle is below eye
    float divisor = screenY-centerPlane;
    if (screenY >= centerPlane ) {
      float dy = screenY - centerPlane;
      if (dy==0) {
        dy = screenY+1 - centerPlane;
        divisor = screenY+1 - centerPlane;
      }
      float angle = atan( dy / viewDist );
      float floorX = eyeY / tan( angle );
      float floorY = 0; // should always be 0
      Point hit;
      hitSlope = Shape::linesIntersect(
        eyeX, eyeY, floorX, floorY,
        farWallX, farWallY, nearWallX, nearWallY,
        &hit.x, &hit.y
      );
      if (hitSlope) {
        wallTop = hit.y;
      }
    }
    // Angle is above eye
    else {
      float dy = centerPlane - screenY;
      float angle = atan( dy / viewDist );
      float ceilingY = TILE_SIZE*99; // imaginary high ceiling
      float ceilingX = ceilingY / tan( angle  );
      Point hit;
      hitSlope = Shape::linesIntersect(
        eyeX, eyeY, ceilingX, ceilingY,
        farWallX, farWallY, nearWallX, nearWallY,
        &hit.x, &hit.y
      );
      if (hitSlope) {
        wallTop = hit.y;
      }
    }

    if (!hitSlope) {
      if (wasInWall) {
        return true;
      }
      continue;
    }

    float ratio = (eyeY - wallTop) / divisor;
    float straightDistance = viewDist * ratio;
    float diagonalDistance = straightDistance * cosFactor;

    float xEnd = (diagonalDistance *  cosine(rayHit.rayAngle));
    float yEnd = (diagonalDistance * -sine(rayHit.rayAngle));
    if (isinf(xEnd) || isinf(yEnd)) {
      if (wasInWall) {
        return true;
      }
      continue;
    }
    xEnd += player.x;
    yEnd += player.y;
    int x = (int)(xEnd) % TILE_SIZE;
    int y = (int)(yEnd) % TILE_SIZE;

    int textureID = rayHit.thinWall->thickWall->floorTextureID;
    bool wallTextureExists = textureID < (int)floorCeilingBitmaps.size();
    bool outOfBounds = x < 0 || y < 0 || x>MAP_WIDTH*TILE_SIZE ||
                       y>MAP_HEIGHT*TILE_SIZE;
    if (outOfBounds || !wallTextureExists) {
      if (wasInWall) {
        return true;
      }
      continue;
    }

    Bitmap& bitmap = floorCeilingBitmaps[ textureID ];
    Uint32* pix = (Uint32*)bitmap.getPixels();
    if (!pix) {
      continue;
    }
    int textureX = (float) x / TILE_SIZE * TEXTURE_SIZE;
    int textureY = (float) y / TILE_SIZE * TEXTURE_SIZE;
    int dstPixel = screenX + (screenY+pitch) * displayWidth;
    int srcPixel = textureY * bitmap.getWidth() + textureX;
    bool pixelOK = srcPixel>=0 && dstPixel>=0 &&
                   srcPixel<bitmap.getWidth()*bitmap.getHeight() &&
                   dstPixel<displayWidth*displayHeight;
    if (pixelOK) {
      wasInWall = true;
      switch (stripWidth) {
        case 4:
          screenPixels[dstPixel+3] = pix[srcPixel];
        case 3:
          screenPixels[dstPixel+2] = pix[srcPixel];
        case 2:
          screenPixels[dstPixel+1] = pix[srcPixel];
        default:
          screenPixels[dstPixel] = pix[srcPixel];
          break;
      }
    }
  }

  return true;
}

bool Game::drawSlopeInverted(RayHit& rayHit, float playerScreenZ)
{
  Uint32* screenPixels = (Uint32*) screenSurface->pixels;

  RayHit sibling;

  // No siblings means player is inside the slope. The sibling is behind
  // the player, so we do a backwards raycast to find it.
  if (rayHit.siblingDistance == 0) {
    if (raycaster3D.findSiblingAtAngle(sibling, *rayHit.thinWall,
                                       rayHit.rayAngle-M_PI, player.rot,
                                       rayHit.strip, player.x, player.y,
                                       player.x, player.y))
    {
      // Sibling distance to player is negated because it is behind.
      sibling.correctDistance = -sibling.distance;
      rayHit.copySibling(sibling);
    }
  }

  // Only draw slope if current wall is further than sibling wall
  if (!rayHit.siblingDistance ||
       rayHit.correctDistance < rayHit.siblingCorrectDistance) {
    return false;
  }

  // Cross section values of current strip
  float farWallX = rayHit.correctDistance;
  float farWallY = rayHit.thinWall->z + rayHit.invertedZ;
  float nearWallX = rayHit.siblingCorrectDistance;
  float nearWallY = rayHit.siblingThinWallZ + rayHit.siblingInvertedZ;
  float eyeX = 0;
  float eyeY = TILE_SIZE/2 + player.z;

  float centerPlane = displayHeight / 2;
  float cosFactor = 1/cos(player.rot-rayHit.rayAngle);
  float wasInWall = false;
  float screenX = rayHit.strip * stripWidth;
  float screenY = 0-pitch;
  for (; screenY<displayHeight-pitch; screenY++) {
    float wallTop = 0;
    bool hitSlope = false;

    // Angle is below eye
    float divisor = screenY-centerPlane;
    if (screenY >= centerPlane ) {
      float dy = screenY - centerPlane;
      if (dy==0) {
        dy = screenY+1 - centerPlane;
        divisor = screenY+1 - centerPlane;
      }
      float angle = atan( dy / viewDist );
      float floorX = eyeY / tan( angle );
      float floorY = 0; // should always be 0
      Point hit;
      hitSlope = Shape::linesIntersect(
        eyeX, eyeY, floorX, floorY,
        farWallX, farWallY, nearWallX, nearWallY,
        &hit.x, &hit.y
      );
      if (hitSlope) {
        wallTop = hit.y;
      }
    }
    // Angle is above eye
    else {
      float dy = centerPlane - screenY;
      float angle = atan( dy / viewDist );
      float ceilingY = TILE_SIZE*99; // imaginary high ceiling
      float ceilingX = ceilingY / tan( angle  );
      Point hit;
      hitSlope = Shape::linesIntersect(
        eyeX, eyeY, ceilingX, ceilingY,
        farWallX, farWallY, nearWallX, nearWallY,
        &hit.x, &hit.y
      );
      if (hitSlope) {
        wallTop = hit.y;
      }
    }

    if (!hitSlope) {
      if (wasInWall) {
        return true;
      }
      continue;
    }

    float ratio = (eyeY - wallTop) / divisor;
    float straightDistance = viewDist * ratio;
    float diagonalDistance = straightDistance * cosFactor;

    float xEnd = (diagonalDistance *  cosine(rayHit.rayAngle));
    float yEnd = (diagonalDistance * -sine(rayHit.rayAngle));

    if (isinf(xEnd) || isinf(yEnd)) {
      if (wasInWall) {
        return true;
      }
      continue;
    }

    xEnd += player.x;
    yEnd += player.y;
    int x = (int)(xEnd) % TILE_SIZE;
    int y = (int)(yEnd) % TILE_SIZE;

    int textureID = rayHit.thinWall->thickWall->ceilingTextureID;
    bool wallTextureExists = textureID < (int)floorCeilingBitmaps.size();
    bool outOfBounds = x < 0 || y < 0 || x>MAP_WIDTH*TILE_SIZE ||
                       y>MAP_HEIGHT*TILE_SIZE;
    if (outOfBounds || !wallTextureExists) {
      if (wasInWall) {
        return true;
      }
      continue;
    }

    Bitmap& bitmap = floorCeilingBitmaps[ textureID ];
    Uint32* pix = (Uint32*)bitmap.getPixels();
    if (!pix) {
      continue;
    }
    int textureX = (float) x / TILE_SIZE * TEXTURE_SIZE;
    int textureY = (float) y / TILE_SIZE * TEXTURE_SIZE;
    int dstPixel = screenX + (screenY+pitch) * displayWidth;
    int srcPixel = textureY * bitmap.getWidth() + textureX;
    bool pixelOK = srcPixel>=0 && dstPixel>=0 &&
                   srcPixel<bitmap.getWidth()*bitmap.getHeight() &&
                   dstPixel<displayWidth*displayHeight;
    if (pixelOK) {
      wasInWall = true;
      switch (stripWidth) {
        case 4:
          screenPixels[dstPixel+3] = pix[srcPixel];
        case 3:
          screenPixels[dstPixel+2] = pix[srcPixel];
        case 2:
          screenPixels[dstPixel+1] = pix[srcPixel];
        default:
          screenPixels[dstPixel] = pix[srcPixel];
          break;
      }
    }
  }

  return true;
}

void Game::drawWorld(vector<RayHit>& rayHits)
{
  RayHitSorter rayHitSorter(&raycaster3D, TILE_SIZE/2+player.z);
  std::sort(rayHits.begin(), rayHits.end(), rayHitSorter);

  drawSkyboxAndHighestCeiling(rayHits);
  drawFloor(rayHits);

  //-----------------------
  // Draw Walls and Sprites
  //-----------------------
  if (!drawWallsOn) {
    return;
  }

  for (int i=0; i<(int)rayHits.size(); ++i) {
    RayHit& rayHit = rayHits[i];

    // Wall
    if (rayHit.wallType) {
      int wallScreenHeight = Raycaster::stripScreenHeight(viewDist,
                                                        rayHit.correctDistance,
                                                        TILE_SIZE);
      float playerScreenZ = Raycaster::stripScreenHeight(viewDist,
                                                       rayHit.correctDistance,
                                                       player.z);

      float sx = rayHit.tileX/TILE_SIZE*TEXTURE_SIZE;
      if (sx >= TEXTURE_SIZE) {
        sx = TEXTURE_SIZE-1;
      }
      float sy = TEXTURE_SIZE * (rayHit.wallType-1);
      bool wallAboveWall = false;
      bool wallBelowWall = false;
      if (!rayHit.thinWall) {
        if (rayHit.level) {
          int wallBelow = raycaster3D.cellAt(rayHit.wallX, rayHit.wallY,
                                             rayHit.level-1);
          wallAboveWall = wallBelow && !Raycaster::isDoor(wallBelow);
        }
        wallBelowWall= raycaster3D.safeCellAt(rayHit.wallX,rayHit.wallY,
                                              rayHit.level+1);
      }

      SurfaceTexture* img = rayHit.horizontal ? &wallsImageDark : &wallsImage;

      //------------------------------------------------------------------------
      // Corner Checking Start
      //
      // I use different textures for horizontal and vertical lines.
      // However my raycasting algorithm has problems with some corners
      // where 2 identical blocks touching each other with the same texture
      // have a "tear" caused by a perpendicular line.
      //
      // This block of code checks each possible corner where 2 blocks meet
      // and makes sure the perpendicular line drawn is the right texture.
      //
      // If you use the same texture for all sides of a block, you don't need
      // this check at all and can set cornerCheck to false.
      //
      // As for the actual cause I believe it's something to do with int
      // calculations during raycasting. If TILE_SIZE is a large number
      // (e.g. 12800 instead of 128), this check is not necessary.
      // But a large TILE_SIZE seems to cause frame drops because many
      // modulus (%) operations use TILE_SIZE.
      //------------------------------------------------------------------------
      bool cornerCheck = !rayHit.thinWall && true;
      int sxi = (int) sx;
      bool isLeftEdge = sxi==0;
      bool isRightEdge = sxi == TEXTURE_SIZE-1;
      if (cornerCheck && (isLeftEdge||isRightEdge))
      {
        const int wallX = rayHit.wallX;
        const int wallY = rayHit.wallY;
        const int level = rayHit.level;
        int rightWall = raycaster3D.safeCellAt(wallX+1, wallY, level);
        int leftWall = raycaster3D.safeCellAt(wallX-1, wallY, level);
        int bottomWall = raycaster3D.safeCellAt(wallX, wallY+1, level);
        int topWall = raycaster3D.safeCellAt(wallX, wallY-1, level);
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

      // Wall is a door
      bool wallIsDoor = Raycaster::isDoor(rayHit.wallType);
      if (wallIsDoor) {
        sy = 0;
        img = doors[rayHit.wallX+rayHit.wallY*MAP_WIDTH] ? &gatesOpenImage :
                                                           &gatesImage;
      }

      bool isSlope = rayHit.thinWall && rayHit.thinWall->thickWall &&
                     rayHit.thinWall->thickWall->slope;

      // Draw the wall
      if (isSlope) {
        drawSlopeStrip(rayHit,*img,sx,sy);
        if (rayHit.thinWall->thickWall->invertedSlope) {
          drawSlopeInverted(rayHit, playerScreenZ);
        }
        else {
          drawSlope(rayHit, playerScreenZ);
        }
      }
      else if (rayHit.thinWall) {
        drawThinWallStrip(rayHit,*img,sx,sy,wallAboveWall, wallBelowWall);
      }
      else {
        if (!ignoreWallStrip(rayHit)) {
          drawWallStrip(rayHit,*img,sx,sy, wallScreenHeight,
                      wallAboveWall, wallBelowWall);
        }
      }

      // Draw top/bottom faces of wall if player's eye is below/above the wall
      // and no other wall is directly above/below it.
      float eyeHeight  = TILE_SIZE/2 + player.z;
      float wallBottom = rayHit.level * TILE_SIZE;
      float wallTop    = wallBottom + TILE_SIZE;
      if (rayHit.thinWall) {
        wallBottom = rayHit.thinWall->z ;
        wallTop = rayHit.thinWall->z + rayHit.thinWall->height;
      }
      if (eyeHeight<wallBottom && !wallAboveWall) {
        if (drawCeilingOn) {
          if (rayHit.thinWall && rayHit.thinWall->thickWall) {
            if (!isSlope) {
              drawThinWallBottom(rayHit, wallScreenHeight);
            }
          }
          else {
            drawWallBottom(rayHit, wallScreenHeight, playerScreenZ);
          }
        }
      }
      else if (eyeHeight>wallTop && !wallBelowWall) {
        if (drawTexturedFloorOn) {
          if (rayHit.thinWall && rayHit.thinWall->thickWall) {
            if (!isSlope) {
              drawThinWallTop(rayHit, wallScreenHeight);
            }
          }
          else {
            drawWallTop(rayHit, wallScreenHeight, playerScreenZ);
          }
        }
      }

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

float Game::wallScreenY(RayHit&rayHit, float wallHeight)
{
  float defaultWallScreenHeight =
    Raycaster::stripScreenHeight(viewDist, rayHit.correctDistance, TILE_SIZE);
  float wallScreenHeight =
    Raycaster::stripScreenHeight(viewDist, rayHit.correctDistance, wallHeight);

  float y = ((displayHeight - defaultWallScreenHeight)/2);
  if (wallHeight != TILE_SIZE) {
    y += (defaultWallScreenHeight - wallScreenHeight);
  }
  if (rayHit.thinWall && rayHit.thinWall->z) {
    y -= Raycaster::stripScreenHeight(viewDist, rayHit.correctDistance,
                                      rayHit.thinWall->z);
  }
  return y;
}

SDL_Rect Game::stripScreenRect(RayHit& rayHit, float wallHeight)
{
  // Height of 1 tile
  float defaultWallScreenHeight =
    Raycaster::stripScreenHeight(viewDist, rayHit.correctDistance, TILE_SIZE);

  // Height of the wall
  float wallScreenHeight = wallHeight==TILE_SIZE
    ? defaultWallScreenHeight
    : Raycaster::stripScreenHeight(viewDist, rayHit.correctDistance,wallHeight);

  // Clamp height because SDL_Rect uses short int
  static const float MAX_WALL_HEIGHT = SDL_MAX_SINT16;
  if (defaultWallScreenHeight > MAX_WALL_HEIGHT) {
    defaultWallScreenHeight = MAX_WALL_HEIGHT;
  }
  if (wallScreenHeight > MAX_WALL_HEIGHT) {
    wallScreenHeight = MAX_WALL_HEIGHT;
  }

  SDL_Rect rc;
  rc.x = rayHit.strip * stripWidth;
  rc.w = stripWidth;
  rc.h = wallScreenHeight;
  rc.y = (displayHeight-defaultWallScreenHeight)/2 +
         (defaultWallScreenHeight-wallScreenHeight);

  // Thin wall is not lying on the ground
  if (rayHit.thinWall && rayHit.thinWall->z) {
    rc.y -= Raycaster::stripScreenHeight(viewDist, rayHit.correctDistance,
                                         rayHit.thinWall->z);
  }
  // The wall is part of an inverted slope
  if (rayHit.thinWall && rayHit.thinWall->thickWall &&
      rayHit.thinWall->thickWall->invertedSlope)
  {
    rc.y -= Raycaster::stripScreenHeight(viewDist, rayHit.correctDistance,
                                         rayHit.invertedZ);
  }
  return rc;
}


void Game::drawSlopeStrip(RayHit& rayHit, SurfaceTexture& img,
                          float textureX, float textureY)
{
  drawThinWallStrip(rayHit, img, textureX, textureY, false, false);
}

void Game::drawThinWallStrip(RayHit& rayHit, SurfaceTexture& img,
                             float textureX, float textureY,
                             bool aboveWall, bool beloWall)
{
  static const float MAX_WALL_HEIGHT = SDL_MAX_SINT16;

  float playerScreenZ = 0;
  if (player.z) {
    playerScreenZ = Raycaster::stripScreenHeight(viewDist,
                                                 rayHit.correctDistance,
                                                 player.z);
  }

  float heightToDraw = TILE_SIZE;
  float dstY = 0;
  float heightDrawn=0;
  for (; heightDrawn<rayHit.wallHeight; heightDrawn+=heightToDraw) {
    float heightRemaining = rayHit.wallHeight - heightDrawn;
    if (heightRemaining < TILE_SIZE) {
      heightToDraw = heightRemaining;
    }

    SDL_Rect srcrect;
    srcrect.x = textureX;
    srcrect.y = textureY;
    srcrect.w = 1;
    srcrect.h = (heightToDraw/TILE_SIZE) * TEXTURE_SIZE;

    SDL_Rect dstrect = stripScreenRect(rayHit, heightToDraw);
    if (heightDrawn == 0) {
      dstY = dstrect.y + playerScreenZ + pitch;
    }
    else {
      dstY -= dstrect.h;
    }
    dstrect.y = dstY;

    // Hack: Make thick walls longer hide seams and tears caused by ceiling
    // drawing
    if (rayHit.thinWall->thickWall) {
      dstrect.y--;
      dstrect.h+=3;
    }

    if (dstrect.h > MAX_WALL_HEIGHT) {
      continue;
    }

    SDL_BlitScaled(img.getSurface(), &srcrect, screenSurface, &dstrect);
  }
}

void Game::drawWallStrip(RayHit& rayHit, SurfaceTexture& img,
                         float textureX, float textureY,
                         int wallScreenHeight, bool aboveWall, bool belowWall)
{
  // Sometimes a wall that is right in front of the player has a very large
  // height. SDL2 cannot draw it so we have to clamp the value.
  // SDL_MAX_SINT16 is the max for SDL_Rect.h
  static const float MAX_WALL_HEIGHT = SDL_MAX_SINT16;
  if (wallScreenHeight > MAX_WALL_HEIGHT) {
    wallScreenHeight = MAX_WALL_HEIGHT;
  }
  float playerScreenZ = 0;
  if (player.z) {
    playerScreenZ = Raycaster::stripScreenHeight(viewDist,
                                                 rayHit.correctDistance,
                                                 player.z);
    // If player is too high, don't draw the wall at all
    if (playerScreenZ+wallScreenHeight>MAX_WALL_HEIGHT) {
      return;
    }
  }

  SDL_Rect srcrect;
  srcrect.x = textureX;
  srcrect.y = textureY;
  srcrect.w = 1;
  srcrect.h = TEXTURE_SIZE;

  SDL_Rect dstrect;
  dstrect.x = rayHit.strip * stripWidth;
  dstrect.w = stripWidth;
  dstrect.h = wallScreenHeight;
  dstrect.y = (displayHeight-wallScreenHeight)/2 + playerScreenZ + pitch;

  // Hack: Make the highest blocks and walls with space below/above them
  // slightly taller to hide seams and tears caused by ceiling drawing
  if (rayHit.level==highestCeilingLevel-1 ||
     (rayHit.level && (!aboveWall||!belowWall))) {
    dstrect.y--;
    dstrect.h+=3;
  }

  dstrect.y -= rayHit.level * wallScreenHeight;
  SDL_BlitScaled(img.getSurface(), &srcrect, screenSurface, &dstrect);
  if (fogOn) {
    fogWallStrip(&dstrect, rayHit.correctDistance);
  }
  int floors = 1;
  while (floors-- > 1) {
    dstrect.y -= wallScreenHeight;
    SDL_BlitScaled(img.getSurface(), &srcrect, screenSurface, &dstrect);
  }
}

bool Game::ignoreWallStrip(RayHit& rayHit)
{
  // This particular wall side facing north beside one of the slopes
  // should never be visible. If we don't hide it, sometimes parts of it will
  // "poke" through the slope
  bool nextToSlope = rayHit.wallX == 11 && rayHit.wallY == 20 &&
                     rayHit.horizontal && player.y < 20*TILE_SIZE;
  return nextToSlope;
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
  float spriteScreenWidth = TILE_SIZE * viewDist / spriteDistance;

  // X-position on screen
  float x = tan(spriteAngle) * viewDist;

  SDL_Rect rc;
  rc.x = (displayWidth/2) + x - (spriteScreenWidth/2);
  rc.y = (displayHeight - spriteScreenWidth)/2.0f;
  rc.w = spriteScreenWidth;
  rc.h = spriteScreenWidth;

  // Sprite not on the ground
  if (sprite.z) {
    float spriteScreenZ = Raycaster::stripScreenHeight(viewDist,
                                                       spriteDistance,
                                                       sprite.z);
    rc.y -= spriteScreenZ;
  }

  // Player not on the ground
  if (player.z) {
    float playerScreenZ = Raycaster::stripScreenHeight(viewDist,
                                                       spriteDistance,
                                                       player.z);
    rc.y += playerScreenZ;
  }

  rc.y += pitch;

  return rc;
}

void Game::raycastWorld(vector<RayHit>& rayHits)
{
  vector<Sprite*> spritesFound;
  rayHitsCount = 0;
  for (int i=0; i<(int)sprites.size(); ++i) {
    sprites[i].rayhit = false;
  }

  for (int strip=0; strip<rayCount; strip++) {
    float screenX = (rayCount/2 - strip) * stripWidth;
    const float stripAngle = Raycaster::stripAngle(screenX, viewDist);
    raycaster3D.raycast(rayHits, player.x, player.y, player.z, player.rot,
                        stripAngle, strip, 0);

    raycaster3D.raycastThinWalls(rayHits, this->thinWalls,
                                 player.x, player.y, player.z, player.rot,
                                 stripAngle, strip);

    raycaster3D.raycastSprites(rayHits,
                               raycaster3D.grids,
                               raycaster3D.gridWidth, raycaster3D.gridHeight, TILE_SIZE,
                               player.x, player.y, player.z, player.rot,
                               stripAngle, strip, &sprites);
  }
  rayHitsCount = rayHits.size();
}

bool Game::isWallCell(int x, int y, int level) {
  // first make sure that we cannot move outside the boundaries of the level
  if (y < 0 || y >= MAP_HEIGHT || x < 0 || x >= MAP_WIDTH)
    return true;

  if (level==0) {
    if (Raycaster::isDoor(g_map[y][x])) {
      if (doors[x + y * MAP_WIDTH]) {
        return false;
      }
      return true;
    }
  }

  // return true if the map block is not 0, ie. if there is a blocking wall.
  return raycaster3D.safeCellAt(x,y,level);
}

bool Game::playerInWall(float playerX, float playerY, float playerZ) {
  if (playerX<0 || playerY<0 || playerX>MAP_WIDTH*TILE_SIZE ||
      playerY>MAP_HEIGHT*TILE_SIZE) {
    return true;
  }
  float playerWidth = TILE_SIZE/5;
  int playerTileX = playerX / TILE_SIZE;
  int playerTileY = playerY / TILE_SIZE;
  int playerTileLeft   = (int)(playerX  - playerWidth) / TILE_SIZE;
  int playerTileRight  = (int)(playerX  + playerWidth) / TILE_SIZE;
  int playerTileTop    = (int)(playerY  - playerWidth) / TILE_SIZE;
  int playerTileBottom = (int)(playerY  + playerWidth) / TILE_SIZE;
  int playerTileFeet = (1 + playerZ) / TILE_SIZE;
  int playerTileHead = (TILE_SIZE*0.8 + playerZ) / TILE_SIZE;
  if (playerTileFeet<0) {
    playerTileFeet = 0;
  }
  if (playerTileHead>=highestCeilingLevel) {
    return true;
  }

  // Top-Left
  if (raycaster3D.cellAt(playerTileLeft, playerTileTop, playerTileFeet)) {
    if (!Raycaster::isDoor(g_map[playerTileTop][playerTileLeft])) {
      return true;
    }
  }

  // Top-Right
  if (raycaster3D.cellAt(playerTileRight, playerTileTop, playerTileFeet)) {
    if (!Raycaster::isDoor(g_map[playerTileTop][playerTileRight])) {
      return true;
    }
  }

  // Bottom-Left
  if (raycaster3D.cellAt(playerTileLeft, playerTileBottom, playerTileFeet)) {
    if (!Raycaster::isDoor(g_map[playerTileBottom][playerTileLeft])) {
      return true;
    }
  }

  // Bottom-Right
  if (raycaster3D.cellAt(playerTileRight, playerTileBottom, playerTileFeet)) {
    if (!Raycaster::isDoor(g_map[playerTileBottom][playerTileRight])) {
      return true;
    }
  }

  // Feet
  if (raycaster3D.cellAt(playerTileX, playerTileY, playerTileFeet)) {
    if (Raycaster::isDoor(g_map[playerTileY][playerTileX])) {
      if (!doors[playerTileX + playerTileY * MAP_WIDTH]) {
        return true;
      }
    }
    else {
      return true;
    }
  }

  // Head
  if (raycaster3D.cellAt(playerTileX, playerTileY, playerTileHead)) {
    if ( playerTileHead == 0 ) {
      if (Raycaster::isDoor(g_map[playerTileY][playerTileX])) {
        if (!doors[playerTileX + playerTileY * MAP_WIDTH]) {
          return true;
        }
      }
    }
    else {
      return true;
    }
  }

  return false;
}

void Game::onKeyDown( SDL_Event* evt ) {
}

void Game::onKeyUp( SDL_Event* evt ) {
    int sym = evt->key.keysym.sym;
    switch(sym) {
      case SDLK_m: {
        drawMiniMapOn = !drawMiniMapOn;
        printf("drawMiniMapOn = %s\n", drawMiniMapOn?"true":"false");
        break;
      }
      case SDLK_LCTRL: {
        if (!player.jumping) {
          player.jumping = true;
        }
        break;
      }
      case SDLK_r: {
        reset();
        printf("Game reset!\n");
        break;
      }
      case SDLK_t: {
        drawTexturedFloorOn = !drawTexturedFloorOn;
        printf("drawTexturedFloorOn = %s\n",drawTexturedFloorOn?"true":"false");
        break;
      }
      case SDLK_c: {
        drawCeilingOn = !drawCeilingOn;
        printf("drawCeilingOn = %s\n", drawCeilingOn?"true":"false");
        break;
      }
      case SDLK_f: {
        fogOn = !fogOn;
        printf("fogOn = %s\n", fogOn?"true":"false");
        break;
      }
      case SDLK_1: {
        drawWallsOn = !drawWallsOn;
        printf("drawWallsOn = %s\n", drawWallsOn?"true":"false");
        break;
      }
      case SDLK_2: {
        skipDrawnFloorStrips = !skipDrawnFloorStrips;
        printf("skipDrawnFloorStrips = %s\n",
               skipDrawnFloorStrips?"true":"false");
        break;
      }
      case SDLK_3: {
        skipDrawnSkyboxStrips = !skipDrawnSkyboxStrips;
        printf("skipDrawnSkyboxStrips = %s\n",
               skipDrawnSkyboxStrips?"true":"false");
        break;
      }
      case SDLK_4: {
        skipDrawnHighestCeilingStrips = !skipDrawnHighestCeilingStrips;
        printf("skipDrawnHighestCeilingStrips = %s\n",
               skipDrawnHighestCeilingStrips?"true":"false");
        break;
      }
      case SDLK_h: {
        printHelp();
        break;
      }
      case SDLK_RETURN2:
      case SDLK_RETURN: {
        toggleDoorPressed();
        break;
      }
      case SDLK_o: {
        printf("Rotation set to 0\n");
        player.rot = 0;
        break;
      }
      case SDLK_p: {
        printf("Rotation set to pi degrees\n");
        player.rot = M_PI;
        break;
      }
      case SDLK_g: {
        drawWeaponOn = !drawWeaponOn;
        printf("drawWeaponOn = %s\n", drawWeaponOn?"true":"false");
        break;
      }
      case SDLK_SPACE: {
        Mix_PlayChannel( -1, projectileFireSound, 0 );
        addProjectile(SpriteTypeProjectile, player.x, player.y, player.z,
                      TILE_SIZE, player.rot);
        break;
      }
    }
}

void Game::toggleDoor( int x, int y ) {
  int offset = x + y * MAP_WIDTH;
  doors[offset] = !doors[offset];
  if (doors[offset]) {
    Mix_PlayChannel( -1, doorOpenSound, 0 );
  }
  else {
    Mix_PlayChannel( -1, doorCloseSound, 0 );
  }
}

void Game::toggleDoorPressed()
{
  const int wallX = player.x / TILE_SIZE;
  const int wallY = player.y / TILE_SIZE;
  const int level = 0;
  int rightWall = raycaster3D.safeCellAt(wallX+1, wallY, level);
  if (Raycaster::isDoor(rightWall)) {
    printf("Triggering east door\n");
    toggleDoor(wallX+1, wallY);
    return;
  }
  int leftWall = raycaster3D.safeCellAt(wallX-1, wallY, level);
  if (Raycaster::isDoor(leftWall)) {
    printf("Triggering west door\n");
    toggleDoor(wallX-1, wallY);
    return;
  }
  int bottomWall = raycaster3D.safeCellAt(wallX, wallY+1, level);
  if (Raycaster::isDoor(bottomWall)) {
    printf("Triggering south door\n");
    toggleDoor(wallX, wallY+1);
    return;
  }
  int topWall = raycaster3D.safeCellAt(wallX, wallY-1, level);
  if (Raycaster::isDoor(topWall)) {
    printf("Triggering north door\n");
    toggleDoor(wallX, wallY-1);
    return;
  }
}

int main(int argc, char** argv){
    Game game;
    game.start();
    return 0;
}
