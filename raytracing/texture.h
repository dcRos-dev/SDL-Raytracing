#pragma once
#include <cmath>
#include <string>
#include <iostream>
#include "SDL.h"
#include "SDL_image.h"
#include "vec3.h"
#include "raster.h"

using namespace std;

class texture {
public:
    virtual color value(float u, float v, const point3& p) const = 0;
};

class constant_texture : public texture {
public:
    constant_texture() {}
    constant_texture(color c) : color(c) {}

    virtual color value(float u, float v, const point3& p) const {
        return color;
    }

    color color;
};

class checker_texture : public texture {
public:
    checker_texture() {}
    checker_texture(texture* t0, texture* t1) : even(t0), odd(t1) {}

    virtual color value(float u, float v, const point3& p) const {
        float sines = sin(2 * p[0]) * sin(2 * p[1]) * sin(2 * p[2]);
        if (sines < 0)
            return odd->value(u, v, p);
        else
            return even->value(u, v, p);
    }

    texture* odd;
    texture* even;
};

class image_texture : public texture {
public:
    image_texture(const char* filename) {
        SDL_Surface* loaded_surface = IMG_Load(filename);

        if (loaded_surface == nullptr) {
            std::cerr << "[Error] Failed to load texture: " << filename << std::endl;
            std::cerr << "   IMG_Load Error: " << IMG_GetError() << std::endl;

            // Create 1x1 magenta fallback texture
            surface = SDL_CreateRGBSurface(0, 1, 1, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
            imageWidth = 1;
            imageHeight = 1;
            return;
        }

        // Always convert to RGB888 (standard format)
        surface = SDL_ConvertSurfaceFormat(loaded_surface, SDL_PIXELFORMAT_RGB888, 0);

        if (surface == nullptr) {
            std::cerr << "[Error] Failed to convert format for: " << filename << std::endl;
            std::cerr << "   SDL Error: " << SDL_GetError() << std::endl;
            surface = loaded_surface;  // Use original (magenta-ish fallback logic)
        }
        else {
            SDL_FreeSurface(loaded_surface);
        }

        imageWidth = surface->w;
        imageHeight = surface->h;
    }

    image_texture(const std::string& filename) : image_texture(filename.c_str()) {}

    virtual color value(float u, float v, const point3& p) const;

    color* pixels;
    SDL_Surface* surface;
    int imageWidth, imageHeight;
};

color image_texture::value(float u, float v, const point3& p) const {
    if (surface == nullptr) {
        return color(1.0f, 0.0f, 1.0f);  // Magenta if texture is missing
    }

    int i = (u) * float(imageWidth);
    int j = (1.0f - v) * imageHeight - 0.001f;

    if (i < 0) i = 0;
    if (j < 0) j = 0;
    if (i > imageWidth - 1) i = imageWidth - 1;
    if (j > imageHeight - 1) j = imageHeight - 1;

    Uint32 pixel_value = getpixel(surface, i, j);

    // Use SDL_GetRGB to handle all formats automatically
    SDL_PixelFormat* format = surface->format;
    Uint8 r, g, b;
    SDL_GetRGB(pixel_value, format, &r, &g, &b);

    float red = float(r) / 255.0f;
    float green = float(g) / 255.0f;
    float blue = float(b) / 255.0f;

    return color(red, green, blue);
}

class image_texture_no_repeat : public image_texture {
public:
    image_texture_no_repeat(const char* filename) : image_texture(filename) {}
    image_texture_no_repeat(const std::string& filename) : image_texture(filename) {}

    virtual color value(float u, float v, const point3& p) const override {
        u = (p.x() + 50.0f) / 100.0f;
        v = (p.z() + 50.0f) / 100.0f;

        u = clamp(u, 0.0f, 1.0f);
        v = clamp(v, 0.0f, 1.0f);

        return image_texture::value(u, v, p);
    }
};