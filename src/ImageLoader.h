//
// Created by kiyazz on 2/5/25.
//

#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#ifdef __linux__
#include "SDL_image.h"
#else
#include "../SDL_image/SDL_image.h"
#endif

#include <filesystem>
#include <iostream>
#include <unordered_map>


inline std::unordered_map<std::string, SDL_Texture*> cache;

inline SDL_Texture* getImage(SDL_Renderer* renderer, const std::string& file) {
    if (const auto it = cache.find(file); it != cache.end()) {
        return it->second;
    }
    std::string path = "resources/images/";
    path.append(file);
    path.append(".png");
    if (!std::filesystem::exists(path)) {
        std::cout << "error: missing image " + file;
        exit(0);
    }
    SDL_Texture* texture = IMG_LoadTexture(renderer, path.c_str());
    cache[file] = texture;
    return texture;
}

inline void createDefaultParticle(SDL_Renderer* renderer, const std::string& name) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 8, 8, 32, SDL_PIXELFORMAT_RGBA8888);

    const uint32_t white = SDL_MapRGBA(surface->format, 255, 255, 255, 255);
    SDL_FillRect(surface, nullptr, white);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    cache[name] = texture;
}


#endif //IMAGELOADER_H
