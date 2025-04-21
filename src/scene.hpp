#ifndef SCENE_HPP
#define SCENE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <unordered_set>

#include "../glm/glm.hpp"
#include "SDL_render.h"
#include "SDL_ttf.h"

#include "lua.hpp"
#include "LuaBridge.h"

#ifdef __APPLE__
#include "box2d.h"
#else
#include "box2d/box2d.h"
#endif


#ifndef _WIN32
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
#endif

#include "../rapidjson/document.h"
#include "../rapidjson/filereadstream.h"

#ifndef _WIN32
#pragma clang diagnostic pop
#endif

#include <algorithm>

#include "SDL_mixer.h"

struct Reference;
class Deserializer;
class Serializer;
extern int WIDTH;
extern int HEIGHT;
extern float HALFWIDTH;
extern float HALFHEIGHT;
extern float ZOOMFACTOR;
extern float ZOOMINVERSE;
extern float HALFHZOOM;
extern float HALFWZOOM;

inline float getZoom() {
	return ZOOMFACTOR;
}

inline void setZoom(float zoom) {
	ZOOMFACTOR = zoom;
	ZOOMINVERSE = 1.0f / zoom;
	HALFWZOOM = ZOOMINVERSE * HALFWIDTH;
	HALFHZOOM = ZOOMINVERSE * HALFHEIGHT;
}

inline void ReadJsonFile(const std::string& path, rapidjson::Document& out_document)
{
	FILE* file_pointer = nullptr;
#ifdef _WIN32
	fopen_s(&file_pointer, path.c_str(), "rb");
#else
	file_pointer = fopen(path.c_str(), "rb");
#endif
	char buffer[65536];
	rapidjson::FileReadStream stream(file_pointer, buffer, sizeof(buffer));
	out_document.ParseStream(stream);
	std::fclose(file_pointer);

	if (out_document.HasParseError()) {
		std::cout << "error parsing json at [" << path << "]" << std::endl;
		exit(0);
	}
}

inline void ReportError (const std::string& name, const luabridge::LuaException& e) {
	std::string errorMessage = e.what();

	std::replace(errorMessage.begin(), errorMessage.end(), '\\', '/');

	std::cout << "\033[31m" << name << " : " << errorMessage << "\033[0m" << std::endl;
}

class Actor;

struct Collision {
	Actor* other = nullptr;
	b2Vec2 point;
	b2Vec2 relativeVelocity;
	b2Vec2 normal;
};

struct HitResult {
	Actor* actor;
	b2Vec2 point;
	b2Vec2 normal;
	bool isTrigger;
	HitResult(Actor* actor, const b2Vec2& point, const b2Vec2& normal, bool isTrigger) 
		: actor(actor), point(point), normal(normal), isTrigger(isTrigger) {}
};

typedef void (*lifecycleFunction) (luabridge::LuaRef);
typedef void (*collisionFunction) (luabridge::LuaRef, Collision&);

class Component {
public:
	luabridge::LuaRef first;
    std::string type = "Rigidbody";
    lifecycleFunction onStart = nullptr;
    lifecycleFunction onUpdate = nullptr;
    lifecycleFunction onLateUpdate = nullptr;
    lifecycleFunction onDestroyed = nullptr;
    collisionFunction onCollisionEnter = nullptr;
    collisionFunction onCollisionExit = nullptr;
	collisionFunction onTriggerEnter = nullptr;
	collisionFunction onTriggerExit = nullptr;
	bool initialized = false;
	Component();
	Component(const Component& other);
	virtual Component* clone();
	virtual void serialize(Serializer& serial);
    void kindaADestructor() const;
	virtual ~Component();
};


class Actor {

public:
	std::map<std::string, Component*> components;
	std::unordered_map<std::string, Component*> componentsByKey;
	std::unordered_map<std::string, std::vector<Component*>> componentsByType;
	std::vector<Component*> addedThisFrame, removedThisFrame;
	std::string name;
	unsigned long long uuid = 0;
	bool dontDestroy = false;
	bool serialize = false;

	static unsigned long long lastUUID;
	Actor(rapidjson::Value& json, std::unordered_map<std::string, Actor>& templates);
	explicit Actor(rapidjson::Value& json);

	void update();
	void lateUpdate();
	size_t getUUID() const { return uuid; }
	std::string getName() const {return name;}
	luabridge::LuaRef getComponentByKey(const std::string& key);
	Component* getCompPointerByKey(const std::string& key);
	luabridge::LuaRef getComponentType(const std::string& key);
	luabridge::LuaRef getComponentTypeAll(const std::string& key);
	luabridge::LuaRef addComponent(const std::string& type);
	void removeComponent(const luabridge::LuaRef& component);
	bool operator<(const Actor& other) const {
		return uuid < other.uuid;
	}
	Actor() = default;
	Actor(const Actor& other);
	Actor& operator=(const Actor& other);
	~Actor();
};

// REQUEST TYPE: 0=scene-space, 1=UI, 2=text, 3=pixels
struct RenderRequest {
	SDL_Texture* texture;
	float x, y;
	float scaleX, scaleY;
	float rotation;
	float pivotX, pivotY;
	int sortingOrder;
	uint8_t r, g, b, a;
	RenderRequest(SDL_Texture* t, float x, float y, float sx, float sy, float rot, float px, float py, int order, uint8_t r, uint8_t g, uint8_t b, uint8_t a) :
	texture(t), x(x), y(y), scaleX(sx), scaleY(sy), rotation(rot), pivotX(px), pivotY(py), sortingOrder(order),
	r(r), g(g), b(b), a(a) {}

	RenderRequest(SDL_Texture* t, float x, float y) :
	texture(t), x(x), y(y), scaleX(1.0f), scaleY(1.0f), rotation(0), pivotX(0.5f), pivotY(0.5f), sortingOrder(0),
	r(255), g(255), b(255), a(255) {}
};

struct TextRequest {
	SDL_Texture* texture;
	int x, y;
};

struct PointRequest {
	int x, y;
	uint8_t r, g, b, a;
	PointRequest(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) : x(x), y(y), r(r), g(g), b(b), a(a) {

	}
};

class Scene {
public:
	std::unordered_map<std::string, Actor> templates;
	std::unordered_map<std::string, std::vector<Actor*>> actorsByName;
	std::vector<Actor*> actors;
	std::vector<Actor*> addedThisFrame;
	std::vector<Actor*> removedThisFrame;
	std::vector<RenderRequest> renderQueue, UIRenderQueue;
	std::vector<TextRequest> textRenderQueue;
	std::vector<PointRequest> pointQueue;
	glm::vec2 cameraPos = { 0, 0 };
	std::string name;
	std::string nextScene;
	std::string nextScene2;
	bool loadedSave = false;
	int saveType = 0;

	void onStart();
	void afterFrame();
	void renderFrame();
	void resolveRelocTable(std::vector<Reference>& relocTable);
	static luabridge::LuaRef getActorByName(const std::string& name);
	static luabridge::LuaRef getActorByID(size_t id);
	static luabridge::LuaRef createActor(const std::string& templateName);
	static void destroyActor(Actor* actor);
	static luabridge::LuaRef getAllActorByName(const std::string& name);
	static void dontDestroy(Actor* actor);
	static std::string getCurrent();
	static void load(const std::string& newScene);
	static Scene* globalSceneRef;
	explicit Scene(const std::string& filename);
	Scene(const std::string& filename, std::vector<Actor*>& acts, std::unordered_map<std::string, Actor>& temps);
	Scene() = default;
	Scene& operator=(const Scene& other);
	~Scene();
};

template<> struct std::hash<Actor> {
	size_t operator()(const Actor& actor) const noexcept {
		return actor.uuid;
	}
};

#endif
