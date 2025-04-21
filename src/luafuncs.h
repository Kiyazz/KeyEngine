//
// Created by kiyazz on 2/26/25.
//

#ifndef LUAFUNCS_H
#define LUAFUNCS_H

#include "lua.hpp"
#include "LuaBridge.h"

extern lua_State* luaState;

void initializeGlobalFunctions();

luabridge::LuaRef getComponent(const std::string &str);

luabridge::LuaRef getBaseComponent(const std::string& str);

void loadLuaFiles();

void establishInheritance(luabridge::LuaRef &instance, const luabridge::LuaRef &parent);



#endif //LUAFUNCS_H
