#include <iostream>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <vector>

#include "SDL.h"
#include "SDL_ttf.h"

#include "Rendering.h"
#include "Audio.h"
#include "EventBus.h"
#include "InputManager.h"
#include "serializer.h"

#include "luafuncs.h"

#include "lua.hpp"
#include "RigidBody.h"

using std::cout;
using std::endl;
using std::strlen;
using std::string;
using std::vector;

string game_title;

glm::u8vec3 clearColor{255, 255, 255};

int WIDTH = 640;
int HEIGHT = 360;
float HALFWIDTH = 320.0f;
float HALFHEIGHT = 180.0f;
float ZOOMFACTOR = 1.0f;
float ZOOMINVERSE = 1.0f;
float HALFHZOOM = 320.0f;
float HALFWZOOM = 180.0f;

lua_State* luaState = nullptr;
SDL_Renderer* renderer = nullptr;

void cleanUp(SDL_Renderer *renderer, SDL_Window *window) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    exit(0);
}

void readRendering(glm::vec2& camOffset) {
    if (std::filesystem::exists("resources/rendering.config")) {
        rapidjson::Document renderConfig;
        ReadJsonFile("resources/rendering.config", renderConfig);
        if (renderConfig.HasMember("x_resolution"))
            WIDTH = renderConfig["x_resolution"].GetInt();
        HALFWIDTH = static_cast<float>(WIDTH) / 2;
        if (renderConfig.HasMember("y_resolution"))
            HEIGHT = renderConfig["y_resolution"].GetInt();
        HALFHEIGHT = static_cast<float>(HEIGHT) / 2;
        if (renderConfig.HasMember("clear_color_r"))
            clearColor.r = renderConfig["clear_color_r"].GetInt();
        if (renderConfig.HasMember("clear_color_b"))
            clearColor.b = renderConfig["clear_color_b"].GetInt();
        if (renderConfig.HasMember("clear_color_g"))
            clearColor.g = renderConfig["clear_color_g"].GetInt();
        if (renderConfig.HasMember("cam_offset_x"))
            camOffset.x = renderConfig["cam_offset_x"].GetFloat()*100.0f;
        if (renderConfig.HasMember("cam_offset_y"))
            camOffset.y = renderConfig["cam_offset_y"].GetFloat()*100.0f;
        if (renderConfig.HasMember("zoom_factor"))
            ZOOMFACTOR = renderConfig["zoom_factor"].GetFloat();
        ZOOMINVERSE = 1.0f / ZOOMFACTOR;
    }
    HALFHZOOM = HALFHEIGHT * ZOOMINVERSE;
    HALFWZOOM = HALFWIDTH * ZOOMINVERSE;
}

bool compActors(Actor* a, Actor* b) {
    return a->uuid < b->uuid;
}

int main(int argc, char* argv[]) { // NOLINT main should return 0
    std::ios_base::sync_with_stdio(false);
    // read json files and initialize
    rapidjson::Document config;
    // auto startTime = std::chrono::system_clock::now();
    InputManager::init();
    if (!std::filesystem::exists("resources")) {
        cout << "error: resources/ missing";
        exit(0);
    }
    if (!std::filesystem::exists("resources/game.config")) {
        cout << "error: resources/game.config missing";
        exit(0);
    }
    glm::vec2 camOffset = {0, 0};
    readRendering(camOffset);
    // leonardo(100);

    ReadJsonFile("resources/game.config", config);
    if (config.HasMember("game_title"))
        game_title = config["game_title"].GetString();
    if (!config.HasMember("initial_scene")) {
        cout << "error: initial_scene unspecified";
        exit(0);
    }

    // initialize SDL
    SDL_Window *window = nullptr;

    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow(game_title.c_str(), 200, 200, WIDTH, HEIGHT,
                                      SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawColor(renderer, clearColor.r, clearColor.g, clearColor.b, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    Mix_Init(MIX_INIT_OGG);
    Mix_OpenAudio(48000, AUDIO_F32SYS, 2, 2048);
    Mix_AllocateChannels(50);

    TTF_Init();

    // initialize LUA
    luaState = luaL_newstate();
    luaL_openlibs(luaState);

    initializeGlobalFunctions();
    loadLuaFiles();
    createDefaultParticle(renderer, "");

    // load scene
    Scene scene(string(config["initial_scene"].GetString()) + ".scene");
    Scene::globalSceneRef = &scene; //NOLINT address can't escape main

    ContactListener listener;
    if (RigidBody::world) RigidBody::world->SetContactListener(&listener);

    bool exitFlag = true;
    while (exitFlag) {
        SDL_SetRenderDrawColor(renderer, clearColor.r, clearColor.g, clearColor.b, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
        // main game loop
        // updates
        SDL_Event event;
        // input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exitFlag = false;
            }
            InputManager::processEvent(&event);
        }
        // check for new scenes
        if (!scene.nextScene.empty()) {
            if (scene.loadedSave) {
                if (scene.saveType == 1) {
                    // load save state
                    Deserializer serial(scene.nextScene);
                    std::vector<Reference> relocTable;
                    serial.readBool();
                    std::unordered_map<string, Actor> templates = std::move(scene.templates);
                    scene.~Scene();
                    new(&scene) Scene(serial.readScene(relocTable, std::move(templates)));
                    scene.resolveRelocTable(relocTable);
                } else if (scene.saveType == 2) {
                    // load saved scene with scene file and overwriting with the saved actors
                    std::vector<Actor*> acts = scene.actors;
                    string nextScene = scene.nextScene;
                    Deserializer serial(scene.nextScene2);
                    if (serial.readBool()) {
                        std::vector<Reference> relocTable;
                        serial.readTable(relocTable);
                    }
                    std::unordered_map<string, Actor> templates = std::move(scene.templates);
                    scene.~Scene();
                    new(&scene) Scene(nextScene + ".scene", acts, templates);
                    size_t num = serial.readSizeT();
                    std::vector<Reference> relocTable;
                    for (size_t i = 0; i < num; ++i) {
                        Actor* act = serial.readActor(relocTable);
                        luabridge::LuaRef actor = Scene::getActorByID(act->uuid);
                        if (!actor.isNil()) {
                            auto* a = actor.cast<Actor*>();
                            scene.actors.erase(std::find(scene.actors.begin(), scene.actors.end(), a));
                            scene.actorsByName[a->name].erase(std::find(scene.actorsByName[a->name].begin(), scene.actorsByName[a->name].end(), a));
                            delete a;
                        }
                        scene.actors.push_back(act);
                        scene.actorsByName[act->name].push_back(act);
                    }
                    smoothSort(scene.actors, compActors);
                    scene.resolveRelocTable(relocTable);
                } else if (scene.saveType == 3) {
                    Deserializer serial(scene.nextScene);
                    if (serial.readBool()) {
                        std::vector<Reference> relocTable;
                        serial.readTable(relocTable);
                    }
                    size_t num = serial.readSizeT();
                    std::vector<Reference> relocTable;
                    for (size_t i = 0; i < num; ++i) {
                        Actor* act = serial.readActor(relocTable);
                        luabridge::LuaRef actor = Scene::getActorByID(act->uuid);
                        if (!actor.isNil()) {
                            auto* a = actor.cast<Actor*>();
                            scene.actors.erase(std::find(scene.actors.begin(), scene.actors.end(), a));
                            scene.actorsByName[a->name].erase(std::find(scene.actorsByName[a->name].begin(), scene.actorsByName[a->name].end(), a));
                            delete a;
                        }
                        scene.actors.push_back(act);
                        scene.actorsByName[act->name].push_back(act);
                    }
                    smoothSort(scene.actors, compActors);
                    scene.resolveRelocTable(relocTable);
                }
            }
            else {
                // save info needed from old scene
                std::vector<Actor*> acts = scene.actors;
                string nextScene = scene.nextScene;
                std::unordered_map<string, Actor> templates = std::move(scene.templates);
                scene.~Scene();
                new(&scene) Scene(nextScene + ".scene", acts, templates);
            }
            scene.loadedSave = false;
            scene.nextScene = "";
        }
        // actor processing
        scene.onStart();
        for (Actor* actor : scene.actors) {
            // actor updating
            actor->update();
        }
        for (Actor* actor : scene.actors) {
            actor->lateUpdate();
        }

        scene.afterFrame();
        Events::lateUpdate();
        if (RigidBody::world)
            RigidBody::world->Step(1.0f / 60.0f, 8, 3);
        // rendering
        scene.renderFrame();
        // processing finished
        Helper::SDL_RenderPresent(renderer);
        InputManager::lateUpdate();
    }

    // auto endTime = std::chrono::system_clock::now();
    // auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(endTime-startTime);
    // cout << diff.count() << " milliseconds" << endl;

    return 0;
}
