#pragma once
#include <iostream>
#include "SDL.h"
#include "SDL_image.h"
#include "color.h"

// Note: Declaring globals in a header can cause "multiple definition" errors 
// if included in multiple .cpp files. Consider using 'extern' or 'inline'.
SDL_Window* window;
SDL_Renderer* renderer;

using namespace std;

inline void setColor(float r, float g, float b, float a = 1.0) {
    // Clamp values between 0.0 and 1.0
    r = (r > 1.0f) ? 1.0f : ((r < 0.0f) ? 0.0f : r);
    g = (g > 1.0f) ? 1.0f : ((g < 0.0f) ? 0.0f : g);
    b = (b > 1.0f) ? 1.0f : ((b < 0.0f) ? 0.0f : b);
    a = (a > 1.0f) ? 1.0f : ((a < 0.0f) ? 0.0f : a);
    SDL_SetRenderDrawColor(renderer, Uint8(r * 255.0), Uint8(g * 255.0), Uint8(b * 255.0), Uint8(a * 255.0));
}

inline void setPixel(int x, int y) {
    if (x < 0 || y < 0) {
        std::cerr << "[Error] Negative pixel coordinates: (" << x << ", " << y << ")" << std::endl;
        return;
    }
    SDL_RenderDrawPoint(renderer, x, y);
}

inline void setPixel(int x, int y, float r, float g, float b, float a = 1.0) {
    setColor(r, g, b, a);
    setPixel(x, y);
}

inline void drawLine(int x1, int y1, int x2, int y2) {
    if (x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0) {
        std::cerr << "[Error] Negative line coordinates detected." << std::endl;
        return;
    }
    // Note: The -1 offset is unusual for standard lines; 
    // ensure this matches your coordinate system logic.
    SDL_RenderDrawLine(renderer, x1, y1 - 1, x2, y2 - 1);
}

Uint32 getpixel(SDL_Surface* surface, int x, int y) {
    if (!surface) {
        throw std::runtime_error("[Error] Null surface passed to getpixel!");
    }
    if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) {
        throw std::runtime_error("[Error] Coordinates out of surface bounds!");
    }

    int bpp = surface->format->BytesPerPixel;
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
    case 1:
        return *p;
    case 2:
        return *(Uint16*)p;
    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
    case 4:
        return *(Uint32*)p;
    default:
        throw std::runtime_error("[Error] Unsupported pixel format!");
    }
}

SDL_Surface* loadTexture(const char* image_path, int& imageWidth, int& imageHeight) {
    SDL_Surface* image = IMG_Load(image_path);
    if (!image) {
        std::cerr << "[Error] Failed to load image: " << IMG_GetError() << std::endl;
        return nullptr;
    }

    imageWidth = image->w;
    imageHeight = image->h;

    // Be careful with strict format checking; many textures are loaded in different formats.
    if (image->format->format != SDL_PIXELFORMAT_RGB24 && image->format->format != SDL_PIXELFORMAT_RGBA8888) {
        std::cout << "[Info] Converting texture format for: " << image_path << std::endl;
        // Consider using SDL_ConvertSurfaceFormat here instead of just returning nullptr
    }

    return image;
}

bool saveScreenshotBMP(std::string filepath, SDL_Window* SDLWindow = window, SDL_Renderer* SDLRenderer = renderer) {
    if (!SDLWindow || !SDLRenderer) {
        std::cerr << "[Error] Window or Renderer not initialized!" << std::endl;
        return false;
    }

    SDL_Surface* infoSurface = SDL_GetWindowSurface(SDLWindow);
    if (!infoSurface) {
        std::cerr << "[Error] Could not get window surface: " << SDL_GetError() << std::endl;
        return false;
    }

    int width, height;
    SDL_GetRendererOutputSize(SDLRenderer, &width, &height);

    unsigned char* pixels = new (std::nothrow) unsigned char[width * height * infoSurface->format->BytesPerPixel];
    if (!pixels) {
        std::cerr << "[Error] Memory allocation failed for screenshot buffer!" << std::endl;
        return false;
    }

    if (SDL_RenderReadPixels(SDLRenderer, nullptr, infoSurface->format->format, pixels, width * infoSurface->format->BytesPerPixel) != 0) {
        std::cerr << "[Error] Failed to read pixels from renderer: " << SDL_GetError() << std::endl;
        delete[] pixels;
        return false;
    }

    SDL_Surface* saveSurface = SDL_CreateRGBSurfaceFrom(
        pixels, width, height,
        infoSurface->format->BitsPerPixel,
        width * infoSurface->format->BytesPerPixel,
        infoSurface->format->Rmask, infoSurface->format->Gmask,
        infoSurface->format->Bmask, infoSurface->format->Amask
    );

    if (!saveSurface) {
        std::cerr << "[Error] Failed to create surface from pixel data: " << SDL_GetError() << std::endl;
        delete[] pixels;
        return false;
    }

    if (SDL_SaveBMP(saveSurface, filepath.c_str()) != 0) {
        std::cerr << "[Error] Failed to save BMP file: " << SDL_GetError() << std::endl;
    }
    else {
        std::cout << "[Success] Screenshot saved to: " << filepath << std::endl;
    }

    SDL_FreeSurface(saveSurface);
    // Note: infoSurface from GetWindowSurface shouldn't always be freed manually, 
    // but the pixel buffer must be.
    delete[] pixels;

    return true;
}