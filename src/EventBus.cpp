#include "EventBus.h"

#include <algorithm>

#include "lua.hpp"
#include "LuaBridge.h"

std::unordered_map<std::string, std::vector<std::pair<luabridge::LuaRef, luabridge::LuaRef>>> Events::subscriptions;
std::vector<Event> Events::removedThisFrame;
std::vector<Event> Events::addedThisFrame;

Event::Event(std::string type, const luabridge::LuaRef& comp, const luabridge::LuaRef& func)
: type(std::move(type)), component(comp), function(func) {}

void Events::subscribe(const std::string& type, const luabridge::LuaRef& component, const luabridge::LuaRef& function) {
    addedThisFrame.emplace_back(type, component, function);
}

void Events::unsubscribe(const std::string& type, const luabridge::LuaRef& component, const luabridge::LuaRef& function) {
    removedThisFrame.emplace_back(type, component, function);
}

void Events::publish(const std::string& type, const luabridge::LuaRef& object) {
    const auto& vec = subscriptions[type];
    for (auto& pair : vec) {
        pair.second(pair.first, object);
    }
}

void Events::lateUpdate() {
    for (const auto& event : addedThisFrame) {
        subscriptions[event.type].emplace_back(event.component, event.function);
    }
    for (const auto& event : removedThisFrame) {
        auto& vec = subscriptions[event.type];
        std::pair<luabridge::LuaRef, luabridge::LuaRef> pair = {event.component, event.function};
        vec.erase(std::find(vec.begin(), vec.end(), pair));
    }
    addedThisFrame.clear();
    removedThisFrame.clear();
}
