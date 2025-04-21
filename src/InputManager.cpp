#include "InputManager.h"
#include "SDL.h"

#include <unordered_map>
#include <string>

const std::unordered_map<std::string, SDL_Scancode> keycode_to_scancode = {
    // Directional (arrow) Keys
    {"up", SDL_SCANCODE_UP},
    {"down", SDL_SCANCODE_DOWN},
    {"right", SDL_SCANCODE_RIGHT},
    {"left", SDL_SCANCODE_LEFT},
    // Misc Keys
    {"escape", SDL_SCANCODE_ESCAPE},
    // Modifier Keys
    {"lshift", SDL_SCANCODE_LSHIFT},
    {"rshift", SDL_SCANCODE_RSHIFT},
    {"lctrl", SDL_SCANCODE_LCTRL},
    {"rctrl", SDL_SCANCODE_RCTRL},
    {"lalt", SDL_SCANCODE_LALT},
    {"ralt", SDL_SCANCODE_RALT},
    // Editing Keys
    {"tab", SDL_SCANCODE_TAB},
    {"return", SDL_SCANCODE_RETURN},
    {"enter", SDL_SCANCODE_RETURN},
    {"backspace", SDL_SCANCODE_BACKSPACE},
    {"delete", SDL_SCANCODE_DELETE},
    {"insert", SDL_SCANCODE_INSERT},
    // Character Keys
    {"space", SDL_SCANCODE_SPACE},
    {"a", SDL_SCANCODE_A},
    {"b", SDL_SCANCODE_B},
    {"c", SDL_SCANCODE_C},
    {"d", SDL_SCANCODE_D},
    {"e", SDL_SCANCODE_E},
    {"f", SDL_SCANCODE_F},
    {"g", SDL_SCANCODE_G},
    {"h", SDL_SCANCODE_H},
    {"i", SDL_SCANCODE_I},
    {"j", SDL_SCANCODE_J},
    {"k", SDL_SCANCODE_K},
    {"l", SDL_SCANCODE_L},
    {"m", SDL_SCANCODE_M},
    {"n", SDL_SCANCODE_N},
    {"o", SDL_SCANCODE_O},
    {"p", SDL_SCANCODE_P},
    {"q", SDL_SCANCODE_Q},
    {"r", SDL_SCANCODE_R},
    {"s", SDL_SCANCODE_S},
    {"t", SDL_SCANCODE_T},
    {"u", SDL_SCANCODE_U},
    {"v", SDL_SCANCODE_V},
    {"w", SDL_SCANCODE_W},
    {"x", SDL_SCANCODE_X},
    {"y", SDL_SCANCODE_Y},
    {"z", SDL_SCANCODE_Z},
    {"0", SDL_SCANCODE_0},
    {"1", SDL_SCANCODE_1},
    {"2", SDL_SCANCODE_2},
    {"3", SDL_SCANCODE_3},
    {"4", SDL_SCANCODE_4},
    {"5", SDL_SCANCODE_5},
    {"6", SDL_SCANCODE_6},
    {"7", SDL_SCANCODE_7},
    {"8", SDL_SCANCODE_8},
    {"9", SDL_SCANCODE_9},
    {"/", SDL_SCANCODE_SLASH},
    {";", SDL_SCANCODE_SEMICOLON},
    {"=", SDL_SCANCODE_EQUALS},
    {"-", SDL_SCANCODE_MINUS},
    {".", SDL_SCANCODE_PERIOD},
    {",", SDL_SCANCODE_COMMA},
    {"[", SDL_SCANCODE_LEFTBRACKET},
    {"]", SDL_SCANCODE_RIGHTBRACKET},
    {"\\", SDL_SCANCODE_BACKSLASH},
    {"'", SDL_SCANCODE_APOSTROPHE}
};

std::vector<KeyState> InputManager::keys = std::vector<KeyState>();
glm::vec2 InputManager::lastMousePos = glm::vec2(0, 0);
KeyState InputManager::mouseKeys[4] = {KeyUP, KeyUP, KeyUP, KeyUP};
float InputManager::mouseWheel = 0.0f;

void InputManager::init() {
  keys.resize(SDL_NUM_SCANCODES);
}

void InputManager::lateUpdate() {
    for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
        if (keys[i] == KeyJustDown) keys[i] = KeyDown;
        if (keys[i] == KeyJustUp) keys[i] = KeyUP;
    }
    for (auto& mouseKey : mouseKeys) {
        if (mouseKey == KeyJustDown) mouseKey = KeyDown;
        if (mouseKey == KeyJustUp) mouseKey = KeyUP;
    }
    mouseWheel = 0.0f;
}

void InputManager::processEvent(SDL_Event *e) {
    if (e->type == SDL_KEYDOWN) {
        keys[e->key.keysym.scancode] = KeyJustDown;
    }
    else if (e->type == SDL_KEYUP) {
        keys[e->key.keysym.scancode] = KeyJustUp;
    }
    else if (e->type == SDL_MOUSEMOTION) {
        lastMousePos.x = e->motion.x; //NOLINT narrowing conversion
        lastMousePos.y = e->motion.y; //NOLINT narrowing conversion
    }
    else if (e->type == SDL_MOUSEBUTTONDOWN) {
        mouseKeys[e->button.button] = KeyJustDown;
    }
    else if (e->type == SDL_MOUSEBUTTONUP) {
        mouseKeys[e->button.button] = KeyJustUp;
    }
    else if (e->type == SDL_MOUSEWHEEL) {
        mouseWheel = e->wheel.preciseY;
    }
}

bool InputManager::getKey(const std::string& key) {
    auto it = keycode_to_scancode.find(key);
    if (it == keycode_to_scancode.end()) return false;
    SDL_Scancode code = it->second;
    return keys[code] == KeyDown || keys[code] == KeyJustDown;
}

bool InputManager::getKeyDown(const std::string& key) {
    auto it = keycode_to_scancode.find(key);
    if (it == keycode_to_scancode.end()) return false;
    SDL_Scancode code = it->second;
    return keys[code] == KeyJustDown;
}

bool InputManager::getKeyUp(const std::string& key) {
    auto it = keycode_to_scancode.find(key);
    if (it == keycode_to_scancode.end()) return false;
    SDL_Scancode code = it->second;
    return keys[code] == KeyJustUp;
}

bool InputManager::getMouseButton(uint8_t num) {
    if (num > 3) return false;
    return mouseKeys[num] == KeyJustDown || mouseKeys[num] == KeyDown;
}
bool InputManager::getMouseButtonDown(uint8_t num) {
    if (num > 3) return false;
    return mouseKeys[num] == KeyJustDown;
}
bool InputManager::getMouseButtonUp(uint8_t num) {
    if (num > 3) return false;
    return mouseKeys[num] == KeyJustUp;
}

glm::vec2 InputManager::getMousePosition() {
    return lastMousePos;
}
float InputManager::getMouseScroll() {
    return mouseWheel;
}
void InputManager::showCursor() {
    SDL_ShowCursor(SDL_ENABLE);
}
void InputManager::hideCursor() {
    SDL_ShowCursor(SDL_DISABLE);
}
