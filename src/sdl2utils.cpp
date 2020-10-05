#include "sdl2utils.h"

SDL_Color al::sdl2utils::getRGBAPixelColor(Uint8* pixels, int x, int y, int w) {
  SDL_Color color;
  // Little endian - access RGB in reverse order
  color.b = pixels[4 * (y * w + x) + 0]; // Blue
  color.g = pixels[4 * (y * w + x) + 1]; // Green
  color.r = pixels[4 * (y * w + x) + 2]; // Red
  color.a = pixels[4 * (y * w + x) + 3]; // Alpha
  return color;
}

Uint8* al::sdl2utils::copySurfacePixels( SDL_Surface* surface,
                                     Uint32 pixelFormat,
                                     SDL_Renderer* renderer,
                                     int* width,
                                     int* height,
                                     int* pitch)
{
  Uint8* pixels = 0;
  SDL_Surface* tmpSurface = 0;
  SDL_Texture* texture = 0;
  int sizeInBytes = 0;

  tmpSurface = SDL_ConvertSurfaceFormat(surface, pixelFormat, 0);
  if (tmpSurface) {
    texture = SDL_CreateTexture( renderer, pixelFormat,
                                 SDL_TEXTUREACCESS_STATIC,
                                 tmpSurface->w, tmpSurface->h );
  }

  if (texture) {
    if (width) {
      *width = tmpSurface->w;
    }
    if (height) {
      *height = tmpSurface->h;
    }
    if (pitch) {
      *pitch = tmpSurface->pitch;
    }
    sizeInBytes = tmpSurface->pitch * tmpSurface->h;
    pixels = (Uint8*)malloc( sizeInBytes );
    // SDL_LockTexture( texture, 0, &tmpPixels, &tmpPitch );
    memcpy( pixels, tmpSurface->pixels, sizeInBytes);
    // SDL_UnlockTexture( texture );
  }

  // Cleanup
  if (texture) {
    SDL_DestroyTexture(texture);
  }
  if (tmpSurface) {
    SDL_FreeSurface(tmpSurface);
  }

  return pixels;
}

void al::sdl2utils::StreamingTexture::destroy() {
  unlockTexture();
  if (texture) {
    SDL_DestroyTexture(texture);
    texture = NULL;
  }
}

bool al::sdl2utils::StreamingTexture::create( SDL_Window* window,
                                              SDL_Renderer* renderer,
                                              Uint32 pixelFormat,
                                              int width, int height)
{
  destroy();
  texture = SDL_CreateTexture( renderer, pixelFormat,
                               SDL_TEXTUREACCESS_STREAMING,
                               width, height );

  this->width = width;
  this->height = height;
  pitch = 0;
  pixels = NULL;
  return texture != 0;
}

bool al::sdl2utils::StreamingTexture::lockTexture()
{
    bool success = true;

    //Texture is already locked
    if( pixels != NULL )
    {
        success = false;
    }
    //Lock texture
    else
    {
        if( SDL_LockTexture( texture, NULL, &pixels, &pitch ) != 0 )
        {
            success = false;
        }
    }

    return success;
}

bool al::sdl2utils::StreamingTexture::unlockTexture()
{
    bool success = true;

    //Texture is not locked
    if( pixels == NULL )
    {
        success = false;
    }
    //Unlock texture
    else
    {
        SDL_UnlockTexture( texture );
        pixels = NULL;
        pitch = 0;
    }

    return success;
}

bool al::sdl2utils::Bitmap::load( const char* s, SDL_Renderer* renderer,
                                  Uint32 pixelFormat )
{
  destroy();
  al::sdl2utils::SurfaceTexture surfaceTexture;
  if (surfaceTexture.loadBitmap(s)) {
    void* pixelsTmp = al::sdl2utils::copySurfacePixels(surfaceTexture.getSurface(),
                                                       pixelFormat,
                                                       renderer,
                                                       &width,
                                                       &height,
                                                       &pitch);
    if (pixelsTmp) {
      int sizeInBytes = pitch * height;
      this->pixels = malloc( sizeInBytes );
      memcpy(this->pixels, pixelsTmp, sizeInBytes);
      return true;
    }
  }
  return false;
}
