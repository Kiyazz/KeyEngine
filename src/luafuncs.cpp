#include "luafuncs.h"

#include <iostream>
#include <string>
#include <filesystem>
#include <thread>

#include "Audio.h"
#include "Helper.h"
#include "InputManager.h"
#include "ParticleSystem.h"
#include "Rendering.h"
#include "scene.hpp"
#include "serializer.h"

#ifdef __APPLE__
#include "box2d.h"
#else
#include "../Box2D/include/box2d/box2d.h"
#endif

using namespace std;

#include "lua.hpp"
#include "LuaBridge.h"

#include "raycasting.h"
#include "EventBus.h"

using namespace luabridge;

void cppLog(const string& message) {
    cout << message << endl;
}

void cppLogError(const string& message) {
    cerr << message << endl;
}

const string componentBase = "resources/component_types/";

void loadLuaFiles() {
    if (std::filesystem::exists(componentBase)) {
        for (const auto& file : filesystem::directory_iterator(componentBase)) {
            if (luaL_dofile(luaState, file.path().string().c_str()) != LUA_OK) {
                cout << "problem with lua file " << file.path().stem().string();
                exit(0);
            }
            Serializer::addToExcludeSet(file.path().stem().string());
        }
    }
}

int getFrame() {
    return Helper::GetFrameNumber();
}

void sleep(long millis) {
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

void Quit() {
    exit(0);
}

void OpenURL(const string& url) {
#ifdef _WIN32
    // windows
    std::system(("start " + url).c_str());
#else
    #ifdef __APPLE__
    // macos
        std::system(("open " + url).c_str());
    #else
        // linux
        std::system(("xdg-open " + url).c_str());
    #endif
#endif
}

void setActorSaving(Actor* actor, bool saving) {
    actor->serialize = saving;
}

const string savesPath = "resources/saves/";

void ensureSavesDirectory() {
    std::filesystem::create_directory(savesPath);
}

void saveState(const string& saveFile) {
    std::filesystem::create_directory(savesPath);
    Serializer serial(savesPath+saveFile);
    serial.writeScene(*Scene::globalSceneRef);
    serial.writeGlobalTable();
}

void loadState(const string& saveFile) {
    std::filesystem::create_directory(savesPath);
    Scene::globalSceneRef->nextScene = savesPath+saveFile;
    Scene::globalSceneRef->loadedSave = true;
    Scene::globalSceneRef->saveType = 1;
}

void saveScene(const string& saveFile, const LuaRef& previewData) {
    std::filesystem::create_directory(savesPath);
    Serializer serial(savesPath+saveFile);
    size_t count = 0;
    for (Actor* act : Scene::globalSceneRef->actors) {
        if (act->serialize) {
            count++;
        }
    }
    if (previewData.isNil()) {
        serial.writeBool(false);
    }
    else {
        serial.writeBool(true);
        serial.writeTable(previewData);
    }
    serial.writeSizeT(count);
    for (Actor* act : Scene::globalSceneRef->actors) {
        if (act->serialize) {
            serial.writeActor(act);
        }
    }
}

void loadSceneWithFile(const string& saveFile, const string& scene) {
    std::filesystem::create_directory(savesPath);
    Scene::globalSceneRef->nextScene2 = savesPath+saveFile;
    Scene::globalSceneRef->nextScene = scene;
    Scene::globalSceneRef->loadedSave = true;
    Scene::globalSceneRef->saveType = 2;
}

void loadSceneCurrent(const string& saveFile) {
    std::filesystem::create_directory(savesPath);
    Scene::globalSceneRef->nextScene = savesPath+saveFile;
    Scene::globalSceneRef->loadedSave = true;
    Scene::globalSceneRef->saveType = 3;
}

LuaRef getAllSaves() {
    LuaRef table = newTable(luaState);
    if (std::filesystem::exists(savesPath)) {
        for (const auto& it : std::filesystem::directory_iterator(savesPath)) {
            string filename = it.path().filename().string();
            Deserializer serial(it.path().string());
            bool preview = serial.readBool();
            LuaRef ref(luaState);
            if (preview) {
                std::vector<Reference> relocTable;
                ref = serial.readTable(relocTable);
            }
            table[filename] = ref;
        }
    }
    return table;
}

void initializeGlobalFunctions() {
    getGlobalNamespace(luaState)
        .beginNamespace("Debug")
        .addFunction("Log", cppLog)
        .addFunction("LogError", cppLogError)
        .endNamespace();
    getGlobalNamespace(luaState)
        .beginNamespace("Text")
        .addFunction("Draw", drawText)
        .endNamespace();
    getGlobalNamespace(luaState)
        .beginClass<b2Vec2>("Vector2")
        .addProperty("x", &b2Vec2::x)
        .addProperty("y", &b2Vec2::y)
        .addFunction("Normalize", &b2Vec2::Normalize)
        .addFunction("Length", &b2Vec2::Length)
        .addConstructor<void(*)(float, float)>()
        .addFunction("__add", &b2Vec2::operatorAdd)
        .addFunction("__sub", &b2Vec2::operatorSub)
        .addFunction("__mul", &b2Vec2::operatorMul)
        .addStaticFunction("Dot", static_cast<float(*)(const b2Vec2&, const b2Vec2&)>(&b2Dot))
        .addStaticFunction("Distance", &b2Distance)
        .endClass();
    getGlobalNamespace(luaState)
        .beginClass<Actor>("Actor")
        .addFunction("GetName", &Actor::getName)
        .addFunction("GetID", &Actor::getUUID)
        .addFunction("GetComponentByKey", &Actor::getComponentByKey)
        .addFunction("GetComponent", &Actor::getComponentType)
        .addFunction("GetComponents", &Actor::getComponentTypeAll)
        .addFunction("AddComponent", &Actor::addComponent)
        .addFunction("RemoveComponent", &Actor::removeComponent)
        .endClass()
        .beginClass<glm::vec2>("vec2")
        .addProperty("x", &glm::vec2::x)
        .addProperty("y", &glm::vec2::y)
        .endClass()
        .beginClass<RigidBody>("Rigidbody")
        .addFunction("GetPosition", &RigidBody::getPosition)
        .addFunction("GetRotation", &RigidBody::getRotation)
        .addProperty("x", &RigidBody::x)
        .addProperty("y", &RigidBody::y)
        .addProperty("enabled", &RigidBody::enabled)
        .addProperty("key", &RigidBody::key)
        .addProperty("width", &RigidBody::width)
        .addProperty("height", &RigidBody::height)
        .addProperty("radius", &RigidBody::radius)
        .addProperty("friction", &RigidBody::friction)
        .addProperty("bounciness", &RigidBody::bounciness)
        .addProperty("angular_friction", &RigidBody::angularDampening)
        .addProperty("gravity_scale", &RigidBody::gravityScale)
        .addProperty("body_type", &RigidBody::bodyType)
        .addProperty("precise", &RigidBody::precise)
        .addProperty("rotation", &RigidBody::rotation)
        .addProperty("has_collider", &RigidBody::has_collider)
        .addProperty("has_trigger", &RigidBody::has_trigger)
        .addProperty("actor", &RigidBody::actor)
        .addFunction("AddForce", &RigidBody::AddForce)
        .addFunction("GetGravityScale", &RigidBody::getGravityScale)
        .addFunction("GetVelocity", &RigidBody::getVelocity)
        .addFunction("GetAngularVelocity", &RigidBody::getAngularVelocity)
        .addFunction("GetUpDirection", &RigidBody::getUpDirection)
        .addFunction("GetRightDirection", &RigidBody::getRightDirection)
        .addFunction("SetVelocity", &RigidBody::setVelocity)
        .addFunction("SetUpDirection", &RigidBody::setUpDirection)
        .addFunction("SetRightDirection", &RigidBody::setRightDirection)
        .addFunction("SetGravityScale", &RigidBody::setGravityScale)
        .addFunction("SetPosition", &RigidBody::setPosition)
        .addFunction("SetRotation", &RigidBody::setRotation)
        .addFunction("SetAngularVelocity", &RigidBody::setAngularVelocity)
        .endClass()
        .beginClass<Collision>("Collision")
        .addProperty("other", &Collision::other)
        .addProperty("point", &Collision::point)
        .addProperty("normal", &Collision::normal)
        .addProperty("relative_velocity", &Collision::relativeVelocity)
        .endClass()
        .beginClass<ParticleSystem>("ParticleSystem")
        .addProperty("enabled", &ParticleSystem::enabled)
        .addProperty("actor", &ParticleSystem::actor)
        .addProperty("key", &ParticleSystem::key)
        .addProperty("x", &ParticleSystem::getX, &ParticleSystem::setX)
        .addProperty("y", &ParticleSystem::getY, &ParticleSystem::setY)
        .addProperty("start_scale_min", &ParticleSystem::getScaleMin, &ParticleSystem::setScaleMin)
        .addProperty("start_scale_max", &ParticleSystem::getScaleMax, &ParticleSystem::setScaleMax)
        .addProperty("start_color_a", &ParticleSystem::getA, &ParticleSystem::setA)
        .addProperty("start_color_r", &ParticleSystem::getR, &ParticleSystem::setR)
        .addProperty("start_color_g", &ParticleSystem::getG, &ParticleSystem::setG)
        .addProperty("start_color_b", &ParticleSystem::getB, &ParticleSystem::setB)
        .addProperty("end_color_a", &ParticleSystem::getEndA, &ParticleSystem::setEndA)
        .addProperty("end_color_r", &ParticleSystem::getEndR, &ParticleSystem::setEndR)
        .addProperty("end_color_g", &ParticleSystem::getEndG, &ParticleSystem::setEndG)
        .addProperty("end_color_b", &ParticleSystem::getEndB, &ParticleSystem::setEndB)
        .addProperty("angular_drag_factor", &ParticleSystem::angularDragFactor)
        .addProperty("drag_factor", &ParticleSystem::dragFactor)
        .addProperty("duration_frames", &ParticleSystem::durationFrames)
        .addProperty("end_scale", &ParticleSystem::endScale)
        .addProperty("image", &ParticleSystem::image)
        .addProperty("sorting_order", &ParticleSystem::sortingOrder)
        .addProperty("frames_between_bursts", &ParticleSystem::framesBetweenBursts)
        .addProperty("burst_quantity", &ParticleSystem::burstQuantity)
        .addFunction("Burst", &ParticleSystem::burst)
        .addFunction("Play", &ParticleSystem::play)
        .addFunction("Stop", &ParticleSystem::stop)
        .endClass();
    getGlobalNamespace(luaState)
        .beginNamespace("Actor")
        .addFunction("Find", Scene::getActorByName)
        .addFunction("FindAll", Scene::getAllActorByName)
        .addFunction("Destroy", Scene::destroyActor)
        .addFunction("Instantiate", Scene::createActor)
        .endNamespace();
    getGlobalNamespace(luaState)
        .beginNamespace("Application")
        .addFunction("GetFrame", getFrame)
        .addFunction("Sleep", sleep)
        .addFunction("Quit", Quit)
        .addFunction("OpenURL", OpenURL)
        .endNamespace();
    getGlobalNamespace(luaState)
        .beginNamespace("Input")
        .addFunction("GetKey", InputManager::getKey)
        .addFunction("GetKeyDown", InputManager::getKeyDown)
        .addFunction("GetKeyUp", InputManager::getKeyUp)
        .addFunction("GetMousePosition", InputManager::getMousePosition)
        .addFunction("GetMouseButton", InputManager::getMouseButton)
        .addFunction("GetMouseButtonDown", InputManager::getMouseButtonDown)
        .addFunction("GetMouseButtonUp", InputManager::getMouseButtonUp)
        .addFunction("GetMouseScrollDelta", InputManager::getMouseScroll)
        .addFunction("HideCursor", InputManager::hideCursor)
        .addFunction("ShowCursor", InputManager::showCursor)
        .endNamespace();
    getGlobalNamespace(luaState)
        .beginNamespace("Audio")
        .addFunction("Play", Audio::playAudio)
        .addFunction("Halt", Audio::haltChannel)
        .addFunction("SetVolume", Audio::setVol)
        .endNamespace()
        .beginNamespace("Physics")
        .addFunction("Raycast", &raycast)
        .addFunction("RaycastAll", raycastAll)
        .endNamespace()
        .beginClass<HitResult>("HitResult")
        .addProperty("actor", &HitResult::actor)
        .addProperty("point", &HitResult::point)
        .addProperty("normal", &HitResult::normal)
        .addProperty("is_trigger", &HitResult::isTrigger)
        .endClass();
    getGlobalNamespace(luaState)
        .beginNamespace("Image")
        .addFunction("DrawUI", drawUI)
        .addFunction("DrawUIEx", drawUIEx)
        .addFunction("Draw", drawImage)
        .addFunction("DrawEx", drawImageEx)
        .addFunction("DrawPixel", drawPixel)
        .endNamespace();
    getGlobalNamespace(luaState)
        .beginNamespace("Camera")
        .addFunction("GetZoom", getZoom)
        .addFunction("SetZoom", setZoom)
        .addFunction("SetPosition", setCamPos)
        .addFunction("GetPositionX", getCamX)
        .addFunction("GetPositionY", getCamY)
        .endNamespace();
    getGlobalNamespace(luaState)
        .beginNamespace("Scene")
        .addFunction("Load", Scene::load)
        .addFunction("GetCurrent", Scene::getCurrent)
        .addFunction("DontDestroy", Scene::dontDestroy)
        .addFunction("SetActorSaving", &setActorSaving)
        .endNamespace()
        .beginNamespace("Event")
        .addFunction("Subscribe", &Events::subscribe)
        .addFunction("Unsubscribe", &Events::unsubscribe)
        .addFunction("Publish", &Events::publish)
        .endNamespace()
        .beginNamespace("Saving")
        .addFunction("SaveState", &saveState)
        .addFunction("LoadState", &loadState)
        .addFunction("SaveScene", &saveScene)
        .addFunction("LoadSaveFromFile", &loadSceneWithFile)
        .addFunction("LoadSaveCurrentScene", &loadSceneCurrent)
        .addFunction("GetAllSaves", &getAllSaves)
        .endNamespace();
}

template<> struct std::hash<LuaRef> {
    size_t operator()(const LuaRef& ref) const noexcept {
        return ref.get_m_ref();
    }
};

LuaRef deepCopy(const LuaRef& obj, unordered_set<LuaRef>& seen) {
    if (seen.find(obj) != seen.end()) return obj;
    if (obj.isNil()) return obj;
    LuaRef copy = newTable(luaState);
    seen.insert(copy);
    for (Iterator it(obj); !it.isNil(); ++it) {
        LuaRef key = it.key();
        LuaRef val = it.value();
        key = key.isTable() ? deepCopy(key, seen) : key;
        val = val.isTable() ? deepCopy(val, seen) : val;
        copy[key] = val;
    }
    return copy;
}

void establishInheritance(LuaRef &instance, const LuaRef &parent) {
    // switch to doing a deep copy from parent to child table
    std::unordered_set seen{LuaRef(luaState)};
    instance = deepCopy(parent, seen);
}


LuaRef getComponent(const std::string &str) {
    LuaRef ref = getGlobal(luaState, str.c_str());
    if (ref.isNil()) {
        std::cout << "error: failed to locate component " << str;
        exit(0);
    }
    auto instance = newTable(luaState);
    establishInheritance(instance, ref);
    return instance;
}

LuaRef getBaseComponent(const std::string &str) {
    LuaRef ref = getGlobal(luaState, str.c_str());
    if (ref.isNil()) {
        std::cout << "error: failed to locate component " << str;
        exit(0);
    }
    return ref;
}

