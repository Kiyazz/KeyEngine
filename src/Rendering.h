#ifndef RENDERING_H
#define RENDERING_H

#include <unordered_map>
#include <string>
#include <cstdlib>
#include <iostream>

#include "Helper.h"
#include "SDL.h"
#include "SDL_ttf.h"
#include "scene.hpp"
#include "ImageLoader.h"

namespace std {
    template<> struct hash<pair<string, int>> {
        size_t operator()(const pair<string, int>& font) const noexcept {
            return hash<string>()(font.first) + font.second;
        }
    };
    template <> struct hash<pair<string, SDL_Color>> {
        size_t operator()(const pair<string, SDL_Color>& font) const noexcept {
            return hash<string>()(font.first) + font.second.r * 11 + font.second.g * 17 + font.second.b * 7 + font.second.a * 13;
        }
    };
}

inline bool colorEquals(const SDL_Color& left, const SDL_Color& right) {
    return left.r == right.r && left.g == right.g && left.b == right.b && left.a == right.a;
}

struct colorEqual {
    bool operator()(const std::pair<std::string, SDL_Color>& left, const std::pair<std::string, SDL_Color>& right) const {
        return left.first == right.first && colorEquals(left.second, right.second);
    }
};

inline std::unordered_map<std::pair<std::string, SDL_Color>, SDL_Texture*, std::hash<std::pair<std::string, SDL_Color>>, colorEqual> textCache;
inline std::unordered_map<std::pair<std::string, int>, TTF_Font*> fontCache;

extern SDL_Renderer* renderer;

inline void drawText(const std::string &text, const int x, const int y, const std::string& font_name, int fontSize, float r, float g, float b, float a) {
    SDL_Texture *texture;
    TTF_Font* font;
    if (const auto it = fontCache.find({ font_name, fontSize }); it != fontCache.end()) {
        font = it->second;
    }
    else {
        font = TTF_OpenFont(("resources/fonts/" + font_name + ".ttf").c_str(), fontSize);
        if (font == nullptr) {
            std::cout << "error: font " + font_name + " missing";
            exit(0);
        }
        fontCache[{font_name, fontSize}] = font;
    }
    SDL_Color color = { (uint8_t)r,  (uint8_t)g,  (uint8_t)b,  (uint8_t)a };
    if (const auto it = textCache.find({ text , color}); it != textCache.end()) {
        texture = it->second;
    } else {
        SDL_Surface *surface = TTF_RenderUTF8_Solid(font, text.c_str(), color);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        textCache[{text, color}] = texture;
    }
    Scene::globalSceneRef->textRenderQueue.push_back({texture, x, y});
}

inline void drawUI(const std::string& image_name, int x, int y) {
    SDL_Texture* texture = getImage(renderer, image_name);
    // find texture if it exists, load it if not
    Scene::globalSceneRef->UIRenderQueue.emplace_back(texture, x, y);
}

inline void drawUIEx(const std::string& image_name, float x, float y, float r, float g, float b, float a, int order) {
    SDL_Texture* texture = getImage(renderer, image_name);
    Scene::globalSceneRef->UIRenderQueue.emplace_back(texture, x, y, 1.0f, 1.0f, 0, 1.0f, 1.0f, order, (uint8_t) r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
}

inline void drawImage(const std::string& image_name, float x, float y) {
    SDL_Texture* texture = getImage(renderer, image_name);
    // find texture if it exists, load it if not
    Scene::globalSceneRef->renderQueue.emplace_back(texture, x, y);
}

inline void drawImageEx(const std::string& image_name, float x, float y, float rotation, float scaleX, float scaleY, float pivotX, float pivotY, float r, float g, float b, float a, int order) {
    SDL_Texture* texture = getImage(renderer, image_name);
    Scene::globalSceneRef->renderQueue.emplace_back(texture, x, y, scaleX, scaleY, rotation, pivotX, pivotY, order, (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
}

inline void drawParticle(SDL_Texture* texture, float x, float y, float rotation, float scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int order) {
    Scene::globalSceneRef->renderQueue.emplace_back(texture, x, y, scale, scale, rotation, 0.5f, 0.5f, order, r, g, b, a);
}

inline void drawPixel(int x, int y, float r, float g, float b, float a) {
    Scene::globalSceneRef->pointQueue.emplace_back(x, y, (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
}

inline void setCamPos(float x, float y) {
    Scene::globalSceneRef->cameraPos = {x, y};
}

inline float getCamX() {
    return Scene::globalSceneRef->cameraPos.x;
}

inline float getCamY() {
    return Scene::globalSceneRef->cameraPos.y;
}

inline std::vector<int> memo;
// HAHA WE DO DYNAMIC PROGRAMMING HERE
inline int leonardo(const int k) {
    if (k < 2) return 1;
    // ensure memo is big enough for operation
    memo.reserve(k+1);
    if (memo.empty()) {
        // sentinel value, want memo to be 1-indexed
        memo.push_back(-1);
        memo.push_back(1);
        memo.push_back(1);
        // leonardo(1) and leonardo(2) are 1, so initialize memo
    }
    if (memo.size() < k) {
        return memo[k];
    }
    for (size_t i = memo.size(); i <= k; i++) {
        memo[i] = memo[i-1] + memo[i-2] + 1;
    }
    return memo[k];
}

// WHY DIJKSTRA WHY
template <typename T> void heapify(std::vector<T>& arr, int start, int end, bool(*comp)( T,  T)) {
    int i = start, j=0, k=0;
    while (k < end - start + 1) {
        if (k & 0xAAAAAAAA) {
            j++;
            i = i >> 1;
        }
        else {
            i += j;
            j = j >> 1;
        }
        k++;
    }
    while (i > 0) {
        j = j >> 1;
        k = i+j;
        while (k < end) {
            if (comp(arr[k-i], arr[k])) {
                break;
            }
            std::swap(arr[k], arr[k-i]);
            k++;
        }
        i=j;
    }
}

template<typename T> void smoothSort(std::vector<T>& arr, bool(*comp)( T,  T)) {
    const int n = arr.size();
    int p = n-1;
    int q=p;
    int r = 0;

    while (p > 0) {
        if ((r & 0x03) == 0) {
            heapify(arr, r, q, comp);
        }
        if (leonardo(r) == p) {
            r++;
        }
        else {
            r--;
            q = q - leonardo(r);
            heapify(arr, r, q, comp);
            q = r-1;
            r++;
        }

        std::swap(arr[0], arr[p]);
        --p;
    }

    for (int i = 0; i < n-1; i++) {
        int j = i+1;
        while (j > 0 && comp(arr[j], arr[j-1])) {
            std::swap(arr[j], arr[j-1]);
            --j;
        }
    }
}




#endif //RENDERING_H
