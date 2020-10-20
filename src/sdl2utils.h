/*
Utility functions and classes for SDL2.

Author: Andrew Lim
https://github.com/andrew-lim
*/
#ifndef SDL2_UTILS_H
#define SDL2_UTILS_H
#include <SDL2\SDL.h>

namespace al {
namespace sdl2utils {

/**
 * Copies pixels from surface.
 * You should free() the returned pixels when you're done with them.
 */
Uint8* copySurfacePixels( SDL_Surface* surface,  // surface to copy from
                          Uint32 pixelFormat,    // pixel format
                          SDL_Renderer* renderer,// main SDL2 renderer
                          int* width,            // stores result width
                          int* height,           // stores result height
                          int* pitch);           // stores result pitch


/**
 * Gets the color at the specified (x,y) point in an array of pixels with
 * with w.
 */
SDL_Color getRGBAPixelColor(Uint8* pixels, int x, int y, int w);

/**
 * Wraps a SDL_Texture with SDL_TEXTUREACCESS_STREAMING for pixel manipulation.
 */
class StreamingTexture
{
  int width, height, pitch;
  SDL_Texture* texture;
  void* pixels;
public:
  StreamingTexture() : width(0), height(0), pitch(0), texture(0), pixels(0) {}
  void destroy();
  bool create( SDL_Window* window, SDL_Renderer* renderer, Uint32 pixelFormat,
               int width, int height);
  int getWidth() const {return width;}
  int getHeight() const {return height;}
  int getPitch() const {return pitch;};
  void* getPixels() { return pixels; }
  bool isLocked() const;
  bool lockTexture();
  bool unlockTexture();
  SDL_Texture* getTexture() {return texture;}
}; // class StreamingTexture

/**
 * Wraps a SDL_Surface and SDL_Texture combo.
 */
class SurfaceTexture {
  SDL_Surface* surface;
  SDL_Texture* texture;
public:
  SurfaceTexture() : surface(0), texture(0) {}
  ~SurfaceTexture() {
    destroy();
  }
  SDL_Surface* getSurface() {return surface;}
  SDL_Texture* getTexture() {return texture;}
  void destroy() {
    if (surface) {
      SDL_FreeSurface(surface);
      surface = NULL;
    }
    if (texture) {
      SDL_DestroyTexture(texture);
      texture = NULL;
    }
  }
  SDL_Surface* loadBitmap(const char* s) {
    destroy();
    surface = SDL_LoadBMP(s);
    return surface;
  }
  SDL_Texture* createTexture(SDL_Renderer* renderer) {
    if (texture) {
      SDL_DestroyTexture(texture);
      texture = NULL;
    }
    if (surface) {
      texture = SDL_CreateTextureFromSurface(renderer, surface);
    }
    return texture;
  }
};

/**
 * Holds bitmap pixel information.
 */
class Bitmap {
  int width, height, pitch;
  void* pixels;
public:
  Bitmap() : width(0), height(0), pitch(0), pixels(NULL) {}
  ~Bitmap() {
    destroy();
  }
  void destroy() {
    if(pixels) {
      free(pixels);
      pixels = NULL;
    }
  }
  void* getPixels() {return pixels;}
  int getWidth() const {return width;}
  int getHeight() const {return height;}
  int getPitch() const {return pitch;}
  bool load( const char* s, SDL_Renderer* renderer, Uint32 pixelFormat );
};

} // namespace sdl2utils
} // namespace al

#endif
