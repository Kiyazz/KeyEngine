//
// Created by kiyazz on 2/11/25.
//

#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <string>
#include <vector>

#include "SDL.h"

#include "../glm/glm.hpp"

enum KeyState {
    KeyUP, KeyDown, KeyJustDown, KeyJustUp
};

class InputManager {
public:
    static void init();
    static void lateUpdate();
    static void processEvent(SDL_Event* e);

    static bool getKey(const std::string& key);
    static bool getKeyDown(const std::string& key);
    static bool getKeyUp(const std::string& key);

    static bool getMouseButton(uint8_t num);
    static bool getMouseButtonDown(uint8_t num);
    static bool getMouseButtonUp(uint8_t num);

    static glm::vec2 getMousePosition();
    static float getMouseScroll();

    static void showCursor();
    static void hideCursor();

private:
    static std::vector<KeyState> keys;
    static glm::vec2 lastMousePos;
    static KeyState mouseKeys[4];
    static float mouseWheel;
};

#endif //INPUTMANAGER_H
