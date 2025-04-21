#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include "lua.hpp"
#include "LuaBridge.h"

#include <vector>
#include <unordered_map>
#include <string>

struct Event {
	std::string type;
	luabridge::LuaRef component;
	luabridge::LuaRef function;
	Event(std::string, const luabridge::LuaRef&, const luabridge::LuaRef&);
};

class Events {
public:

	static void subscribe(const std::string& type, const luabridge::LuaRef& component, const luabridge::LuaRef& function);

	static void unsubscribe(const std::string& type, const luabridge::LuaRef& component, const luabridge::LuaRef& function);

	static void publish(const std::string& type, const luabridge::LuaRef& object);

	static void lateUpdate();

	static std::unordered_map<std::string, std::vector<std::pair<luabridge::LuaRef, luabridge::LuaRef>>> subscriptions;
	static std::vector<Event> addedThisFrame, removedThisFrame;

};

#endif