#include <string>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>
#include <set>

#include "scene.hpp"
#include "RigidBody.h"
#include "luafuncs.h"

#include <algorithm>

#include "Helper.h"

#include "lua.hpp"
#include "LuaBridge.h"
#include "ParticleSystem.h"
#include "Rendering.h"
#include "serializer.h"

using rapidjson::Document;
using rapidjson::SizeType;
using std::vector;
using std::string;
using std::set;

using namespace luabridge;

const string basePath = "resources/scenes/";

Scene* Scene::globalSceneRef = nullptr;

bool comp(const Actor* a, const Actor* b) {
    return a->uuid < b->uuid;
}

void Scene::afterFrame(){
	for (Actor* actor : addedThisFrame) {
		for (Component* com : actor->addedThisFrame) {
			actor->components.emplace(com->first["key"], com);
		}
	}
	actors.insert(actors.end(), addedThisFrame.begin(), addedThisFrame.end());
	addedThisFrame.clear();

    std::sort(removedThisFrame.begin(), removedThisFrame.end(), comp);
    // removes everything from output that isn't needed
    std::vector<Actor*> output;
    std::set_difference(actors.begin(), actors.end(), removedThisFrame.begin(), removedThisFrame.end(), std::back_inserter(output), comp);
    std::swap(output, actors);
    // now remove everything from search container
    for (auto& container : actorsByName) {
        auto& vec = container.second;
        output.clear();
        std::set_difference(vec.begin(), vec.end(), removedThisFrame.begin(), removedThisFrame.end(), std::back_inserter(output), comp);
        std::swap(output, vec);
    }
    // all references to actor are removed, so delete the actors
    for (Actor* act : removedThisFrame) {
        delete act;
    }
    removedThisFrame.clear();
}

bool renderComp(const RenderRequest& a, const RenderRequest& b) {
	return a.sortingOrder < b.sortingOrder;
}

void Scene::renderFrame() {
	// sort scene render in order
	std::stable_sort(renderQueue.begin(), renderQueue.end(), renderComp);
	std::stable_sort(UIRenderQueue.begin(), UIRenderQueue.end(), renderComp);
	SDL_RenderSetScale(renderer, ZOOMFACTOR, ZOOMFACTOR);

	for (const RenderRequest& request : renderQueue) {
		SDL_FRect rect;
		Helper::SDL_QueryTexture(request.texture, &rect.w, &rect.h);
		int flip = SDL_FLIP_NONE;
		if (request.scaleX < 0) flip = SDL_FLIP_HORIZONTAL;
		if (request.scaleY < 0) flip = SDL_FLIP_VERTICAL;

		rect.w *= glm::abs(request.scaleX);
		rect.h *= glm::abs(request.scaleY);

		SDL_FPoint pivot = {request.pivotX*rect.w, request.pivotY*rect.h};

		rect.x = (request.x-cameraPos.x) * 100.0f + (HALFWZOOM) - pivot.x;
		rect.y = (request.y-cameraPos.y) * 100.0f + (HALFHZOOM) - pivot.y;

		if (rect.x > WIDTH || rect.y > HEIGHT || rect.x + rect.w < 0 || rect.y + rect.h < 0) continue;

		SDL_SetTextureColorMod(request.texture, request.r, request.g, request.b);
		SDL_SetTextureAlphaMod(request.texture, request.a);

		Helper::SDL_RenderCopyEx(0, "", renderer, request.texture, nullptr, &rect, request.rotation, &pivot, static_cast<SDL_RendererFlip>(flip));

		SDL_SetTextureColorMod(request.texture, 255, 255, 255);
		SDL_SetTextureAlphaMod(request.texture, 255);
	}

	SDL_RenderSetScale(renderer, 1.0f, 1.0f);

	for (const RenderRequest& request : UIRenderQueue) {
		SDL_FRect rect;
		Helper::SDL_QueryTexture(request.texture, &rect.w, &rect.h);
		rect.x = request.x;
		rect.y = request.y;

		SDL_SetTextureColorMod(request.texture, request.r, request.g, request.b);
		SDL_SetTextureAlphaMod(request.texture, request.a);

		Helper::SDL_RenderCopy(renderer, request.texture, nullptr, &rect);

		SDL_SetTextureColorMod(request.texture, 255, 255, 255);
		SDL_SetTextureAlphaMod(request.texture, 255);
	}

	for (const TextRequest& request : textRenderQueue) {
		SDL_FRect rect;
		rect.x = static_cast<float>(request.x);
		rect.y = static_cast<float>(request.y);
		Helper::SDL_QueryTexture(request.texture, &rect.w, &rect.h);
		Helper::SDL_RenderCopy(renderer, request.texture, nullptr, &rect);
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	for (const PointRequest& point : pointQueue) {
		SDL_SetRenderDrawColor(renderer, point.r, point.g, point.b, point.a);
		SDL_RenderDrawPoint(renderer, point.x, point.y);
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

	renderQueue.clear();
	UIRenderQueue.clear();
	textRenderQueue.clear();
	pointQueue.clear();
} // apples
void Scene::resolveRelocTable(std::vector<Reference> &relocTable) {
	for (const Reference& ref : relocTable) {
		if (ref.type == 0) {
			// reference to actor
			ref.ref[ref.key] = getActorByID(ref.actor);
		}
		else {
			// reference to component
			LuaRef actorRef = getActorByID(ref.actor);
			if (actorRef.isNil()) ref.ref[ref.key] = LuaRef(luaState);
			else ref.ref[ref.key] = actorRef.cast<Actor*>()->getComponentByKey(ref.component);
		}
	}
}

LuaRef Scene::getActorByName(const std::string& name) {
	auto ref = LuaRef(luaState);
	if (auto vecRef = globalSceneRef->actorsByName[name]; !vecRef.empty()) {
	    ref = vecRef[0];
	}
	return ref;
}

bool finderComp(Actor* act, size_t id) {
	return act->uuid < id;
}

bool finderEqual(Actor* act, size_t id) {
	return act->uuid == id;
}

template <typename T, typename val> typename vector<T>::iterator binarySearch(vector<T>& vec, val v, bool (*comp)(T, val), bool (*equalComp)(T, val)) {
	size_t start = 0, end = vec.size()-1;
	while (start <= end) {
		size_t mid = start + (end-start) / 2;
		if (equalComp(vec[mid], v)) {
			return vec.begin()+mid;
		}
		if (comp(vec[mid], v)) {
			start = mid+1;
		}
		else {
			end = mid-1;
		}
	}
	return vec.end();
}

LuaRef Scene::getActorByID(size_t id) {
	auto it = binarySearch(globalSceneRef->actors, id, finderComp, finderEqual);
	if (it == globalSceneRef->actors.end()) {
		return {luaState};
	}
	return {luaState, *it};
}

LuaRef Scene::createActor(const std::string &templateName){
    auto* actor = new Actor(globalSceneRef->templates[templateName]);
	actor->uuid = Actor::lastUUID+1;
	Actor::lastUUID++;
    globalSceneRef->addedThisFrame.push_back(actor);
    globalSceneRef->actorsByName[actor->name].push_back(actor);
    auto ref = LuaRef(luaState, actor);
    return ref;
}

void Scene::destroyActor(Actor* actor){
    for (const auto& component : actor->components) {
        (component.second->first)["enabled"] = false;
    }
    globalSceneRef->removedThisFrame.push_back(actor);
	// remove from search container
	auto& ref = globalSceneRef->actorsByName[actor->name];
	ref.erase(std::find(ref.begin(), ref.end(), actor));
}

LuaRef Scene::getAllActorByName(const std::string& name) {
	LuaRef table = newTable(luaState);
    if (auto vecRef = globalSceneRef->actorsByName[name]; !vecRef.empty()) {
        int counter = 1;
        for (Actor* i : vecRef) {
            table[counter] = i;
            counter++;
        }
    }
	return table;
}

Scene::Scene(const std::string& filename) {
	std::string path = basePath + filename;
	name = filename.substr(0, filename.length() - 6);
	if (!std::filesystem::exists(path)) {
		std::cout << "error: scene " + name + " is missing";
		exit(0);
	}
	Document doc;
	ReadJsonFile(path, doc);

	auto& arr = doc["actors"];

	// create templates
	string templatePath = "resources/actor_templates";
	if (std::filesystem::exists(templatePath)) {
		for (const auto& entry : std::filesystem::directory_iterator(templatePath)) {
			Document templat;
			ReadJsonFile(entry.path().string(), templat);
			templates.emplace(entry.path().stem().string(), templat);
		}
	}

	for (unsigned int i = 0; i < arr.Size(); i++) {
		// create actor
		auto& obj = arr[i];
	    auto* actor = new Actor(obj, templates);
		actors.push_back(actor);
		actorsByName[actor->name].push_back(actor);
	}
}

Scene::Scene(const std::string& filename, std::vector<Actor*>& acts, std::unordered_map<std::string, Actor>& temps) {
	std::string path = basePath + filename;
	name = filename.substr(0, filename.length() - 6);
	if (!std::filesystem::exists(path)) {
		std::cout << "error: scene " + name + " is missing";
		exit(0);
	}
	Document doc;
	ReadJsonFile(path, doc);
	std::swap(templates, temps);
	auto& arr = doc["actors"];
	Actor::lastUUID = 0;
	for (Actor* actor : acts) {
		if (actor->dontDestroy) {
			actors.push_back(actor);
			actorsByName[actor->name].push_back(actor);
		}
	}

	for (unsigned int i = 0; i < arr.Size(); i++) {
		// create actor
		auto& obj = arr[i];
		auto* actor = new Actor(obj, templates);
		actors.emplace_back(actor);
		actorsByName[actor->name].push_back(actor);
	}
}

unsigned long long Actor::lastUUID = 0;


Actor::Actor(rapidjson::Value& json, std::unordered_map<std::string, Actor>& templates) : uuid(lastUUID + 1) {
	lastUUID++;
	auto end = json.MemberEnd();
	if (const auto it = json.FindMember("template"); it != end) {
		const std::string templateName = it->value.GetString();
		if (const auto iterator = templates.find(templateName); iterator != templates.end()) {
			*this = iterator->second;
			// give all inherited components a reference to this actor
			for (auto& [fst, snd] : this->components) {
				snd->first["actor"] = this;
			}
		}
		else {
			Document templat;
			if (!std::filesystem::exists("resources/actor_templates/" + templateName + ".template")) {
				std::cout << "error: template " + templateName + " is missing";
				exit(0);
			}
			ReadJsonFile("resources/actor_templates/" + templateName + ".template", templat);
			Actor other{templat};
			*this = other;
			templates[templateName] = other;
			uuid = lastUUID;
		}
	}
	if (auto it = json.FindMember("name"); it != end) {
		name = it->value.GetString();
	}
	if (auto it = json.FindMember("components"); it != end) {
		for (auto it2 = it->value.MemberBegin(); it2 != it->value.MemberEnd(); ++it2) {
			// load component
			LuaRef ref = LuaRef(luaState);
			Component* compon;
			if (auto mapit = componentsByKey.find(it2->name.GetString()); mapit != componentsByKey.end()) {
				// if component of key already exists because of template, get it's reference
				ref = mapit->second->first;
				compon = mapit->second;
			}
			else {
				// else get a new component of that type
				if (strcmp(it2->value["type"].GetString(), "Rigidbody") == 0) {
					auto* compone = new RigidBody();
					components[it2->name.GetString()] = compone;
					compone->first = compone;
					componentsByKey[it2->name.GetString()] = compone;
					componentsByType[it2->value["type"].GetString()].push_back(compone);
					compone->initialized = false;
					compone->actor = this;
					compone->key = it2->name.GetString();
					compone->enabled = true;
					compon = compone;
				}
				else if (strcmp(it2->value["type"].GetString(), "ParticleSystem") == 0) {
					auto* compone = new ParticleSystem(it2->value);
					compone->first = compone;
					components[it2->name.GetString()] = compone;
					componentsByKey[it2->name.GetString()] = compone;
					componentsByType[it2->value["type"].GetString()].push_back(compone);
					compone->initialized = false;
					compone->actor = this;
					compone->key = it2->name.GetString();
					compone->enabled = true;
					compon = compone;
				}
				else {
					ref = getComponent(it2->value["type"].GetString());
					compon = new Component();
					components[it2->name.GetString()] = compon;
					compon->first = ref;
					componentsByKey[it2->name.GetString()] = compon;
					componentsByType[it2->value["type"].GetString()].push_back(compon);
					compon->type = it2->value["type"].GetString();
					compon->initialized = false;
					LuaRef start = compon->first["OnStart"];
					if (start.isFunction()) compon->onStart = [](LuaRef ref) {
						ref["OnStart"](ref);
						};
					LuaRef update = compon->first["OnUpdate"];
					if (update.isFunction()) compon->onUpdate = [](LuaRef ref) {
						ref["OnUpdate"](ref);
						};
					LuaRef lateupdate = compon->first["OnLateUpdate"];
					if (lateupdate.isFunction()) compon->onLateUpdate = [](LuaRef ref) {
						ref["OnLateUpdate"](ref);
						};
					LuaRef onDestroy = compon->first["OnDestroy"];
					if (onDestroy.isFunction()) compon->onDestroyed = [](LuaRef ref) {
						ref["OnDestroy"](ref);
						};
					LuaRef collisionEnter = compon->first["OnCollisionEnter"];
					if (collisionEnter.isFunction()) compon->onCollisionEnter = [](LuaRef ref, Collision& col) {
						ref["OnCollisionEnter"](ref, col);
						};
					LuaRef collisionExit = compon->first["OnCollisionExit"];
					if (collisionExit.isFunction()) compon->onCollisionExit = [](LuaRef ref, Collision& col) {
						ref["OnCollisionExit"](ref, col);
						};
					LuaRef triggerEnter = compon->first["OnTriggerEnter"];
					if (triggerEnter.isFunction()) compon->onTriggerEnter = [](LuaRef ref, Collision& col) {
						ref["OnTriggerEnter"](ref, col);
						};
					LuaRef triggerExit = compon->first["OnTriggerExit"];
					if (triggerExit.isFunction()) compon->onTriggerExit = [](LuaRef ref, Collision& col) {
						ref["OnTriggerExit"](ref, col);
						};
				}
			}
			// override member variables
			if (compon->type == "Rigidbody") {
				auto* rigid = dynamic_cast<RigidBody*>(compon);
				if (auto it3 = it2->value.FindMember("x"); it3 != it2->value.MemberEnd()) {
					rigid->x = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("y"); it3 != it2->value.MemberEnd()) {
					rigid->y = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("density"); it3 != it2->value.MemberEnd()) {
					rigid->density = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("angular_friction"); it3 != it2->value.MemberEnd()) {
					rigid->angularDampening = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("gravity_scale"); it3 != it2->value.MemberEnd()) {
					rigid->gravityScale = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("rotation"); it3 != it2->value.MemberEnd()) {
					rigid->rotation = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("body_type"); it3 != it2->value.MemberEnd()) {
					rigid->bodyType = it3->value.GetString();
				}
				if (auto it3 = it2->value.FindMember("has_collider"); it3 != it2->value.MemberEnd()) {
					rigid->has_collider = it3->value.GetBool();
				}
				if (auto it3 = it2->value.FindMember("has_trigger"); it3 != it2->value.MemberEnd()) {
					rigid->has_trigger = it3->value.GetBool();
				}
				if (auto it3 = it2->value.FindMember("precise"); it3 != it2->value.MemberEnd()) {
					rigid->precise = it3->value.GetBool();
				}
				if (auto it3 = it2->value.FindMember("width"); it3 != it2->value.MemberEnd()) {
					rigid->width = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("height"); it3 != it2->value.MemberEnd()) {
					rigid->height = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("radius"); it3 != it2->value.MemberEnd()) {
					rigid->radius = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("friction"); it3 != it2->value.MemberEnd()) {
					rigid->friction = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("bounciness"); it3 != it2->value.MemberEnd()) {
					rigid->bounciness = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("collider_type"); it3 != it2->value.MemberEnd()) {
					rigid->colliderType = it3->value.GetString();
				}
				if (auto it3 = it2->value.FindMember("trigger_type"); it3 != it2->value.MemberEnd()) {
					rigid->triggerType = it3->value.GetString();
				}
				if (auto it3 = it2->value.FindMember("trigger_width"); it3 != it2->value.MemberEnd()) {
					rigid->triggerWidth = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("trigger_height"); it3 != it2->value.MemberEnd()) {
					rigid->triggerHeight = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("trigger_radius"); it3 != it2->value.MemberEnd()) {
					rigid->triggerRadius = it3->value.GetFloat();
				}
			} else if (compon->type == "ParticleSystem") {
				// do nothing
			}
			else {
				for (auto it3 = it2->value.MemberBegin(); it3 != it2->value.MemberEnd(); ++it3) {
					if (strcmp(it3->name.GetString(), "type") != 0) {
						if (it3->value.IsString())
							(ref)[it3->name.GetString()] = it3->value.GetString();
						else if (it3->value.IsInt())
							(ref)[it3->name.GetString()] = it3->value.GetInt();
						else if (it3->value.IsFloat())
							(ref)[it3->name.GetString()] = it3->value.GetFloat();
						else if (it3->value.IsBool())
							(ref)[it3->name.GetString()] = it3->value.GetBool();
					}
				}
				(ref)["key"] = it2->name.GetString();
				(ref)["enabled"] = true;
				ref["actor"] = this;
			}
		}
	}
}

Actor::Actor(rapidjson::Value& json) : uuid(lastUUID+1) {
	const auto end = json.MemberEnd();
	if (auto it = json.FindMember("name"); it != end) {
		name = it->value.GetString();
	}
	if (auto it = json.FindMember("components"); it != end) {
		for (auto it2 = it->value.MemberBegin(); it2 != it->value.MemberEnd(); ++it2) {
			// load component
			LuaRef ref = LuaRef(luaState);
			Component* compon;
			if (auto mapit = componentsByKey.find(it2->name.GetString()); mapit != componentsByKey.end()) {
				// if component of key already exists because of template, get it's reference
				ref = mapit->second->first;
				compon = mapit->second;
			}
			else {
				// else get a new component of that type
				if (strcmp(it2->value["type"].GetString(), "Rigidbody") == 0) {
					auto* compone = new RigidBody();
					components[it2->name.GetString()] = compone;
					compone->first = compone;
					componentsByKey[it2->name.GetString()] = compone;
					componentsByType[it2->value["type"].GetString()].push_back(compone);
					compone->initialized = false;
					compone->actor = this;
					compone->key = it2->name.GetString();
					compone->enabled = true;
					compon = compone;
				}
				else if (strcmp(it2->value["type"].GetString(), "ParticleSystem") == 0) {
					auto* compone = new ParticleSystem(it2->value);
					compone->first = compone;
					components[it2->name.GetString()] = compone;
					componentsByKey[it2->name.GetString()] = compone;
					componentsByType[it2->value["type"].GetString()].push_back(compone);
					compone->initialized = false;
					compone->actor = this;
					compone->key = it2->name.GetString();
					compone->enabled = true;
					compon = compone;
				}
				else {
					ref = getComponent(it2->value["type"].GetString());
					compon = new Component();
					components[it2->name.GetString()] = compon;
					compon->first = ref;
					componentsByKey[it2->name.GetString()] = compon;
					componentsByType[it2->value["type"].GetString()].push_back(compon);
					compon->type = it2->value["type"].GetString();
					compon->initialized = false;
					LuaRef start = compon->first["OnStart"];
					if (start.isFunction()) compon->onStart = [](LuaRef ref) {
						ref["OnStart"](ref);
						};
					LuaRef update = compon->first["OnUpdate"];
					if (update.isFunction()) compon->onUpdate = [](LuaRef ref) {
						ref["OnUpdate"](ref);
						};
					LuaRef lateupdate = compon->first["OnLateUpdate"];
					if (lateupdate.isFunction()) compon->onLateUpdate = [](LuaRef ref) {
						ref["OnLateUpdate"](ref);
						};
					LuaRef onDestroy = compon->first["OnDestroy"];
					if (onDestroy.isFunction()) compon->onDestroyed = [](LuaRef ref) {
						ref["OnDestroy"](ref);
						};
					LuaRef collisionEnter = compon->first["OnCollisionEnter"];
					if (collisionEnter.isFunction()) compon->onCollisionEnter = [](LuaRef ref, Collision& col) {
						ref["OnCollisionEnter"](ref, col);
						};
					LuaRef collisionExit = compon->first["OnCollisionExit"];
					if (collisionExit.isFunction()) compon->onCollisionExit = [](LuaRef ref, Collision& col) {
						ref["OnCollisionExit"](ref, col);
						};
					LuaRef triggerEnter = compon->first["OnTriggerEnter"];
					if (triggerEnter.isFunction()) compon->onTriggerEnter = [](LuaRef ref, Collision& col) {
						ref["OnTriggerEnter"](ref, col);
						};
					LuaRef triggerExit = compon->first["OnTriggerExit"];
					if (triggerExit.isFunction()) compon->onTriggerExit = [](LuaRef ref, Collision& col) {
						ref["OnTriggerExit"](ref, col);
						};
				}
			}
			// override member variables
			if (compon->type == "Rigidbody") {
				auto* rigid = dynamic_cast<RigidBody*>(compon);
				if (auto it3 = it2->value.FindMember("x"); it3 != it2->value.MemberEnd()) {
					rigid->x = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("y"); it3 != it2->value.MemberEnd()) {
					rigid->y = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("density"); it3 != it2->value.MemberEnd()) {
					rigid->density = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("angular_friction"); it3 != it2->value.MemberEnd()) {
					rigid->angularDampening = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("gravity_scale"); it3 != it2->value.MemberEnd()) {
					rigid->gravityScale = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("rotation"); it3 != it2->value.MemberEnd()) {
					rigid->rotation = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("body_type"); it3 != it2->value.MemberEnd()) {
					rigid->bodyType = it3->value.GetString();
				}
				if (auto it3 = it2->value.FindMember("has_collider"); it3 != it2->value.MemberEnd()) {
					rigid->has_collider = it3->value.GetBool();
				}
				if (auto it3 = it2->value.FindMember("has_trigger"); it3 != it2->value.MemberEnd()) {
					rigid->has_trigger = it3->value.GetBool();
				}
				if (auto it3 = it2->value.FindMember("precise"); it3 != it2->value.MemberEnd()) {
					rigid->precise = it3->value.GetBool();
				}
				if (auto it3 = it2->value.FindMember("width"); it3 != it2->value.MemberEnd()) {
					rigid->width = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("height"); it3 != it2->value.MemberEnd()) {
					rigid->height = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("radius"); it3 != it2->value.MemberEnd()) {
					rigid->radius = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("friction"); it3 != it2->value.MemberEnd()) {
					rigid->friction = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("bounciness"); it3 != it2->value.MemberEnd()) {
					rigid->bounciness = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("collider_type"); it3 != it2->value.MemberEnd()) {
					rigid->colliderType = it3->value.GetString();
				}
				if (auto it3 = it2->value.FindMember("trigger_type"); it3 != it2->value.MemberEnd()) {
					rigid->triggerType = it3->value.GetString();
				}
				if (auto it3 = it2->value.FindMember("trigger_width"); it3 != it2->value.MemberEnd()) {
					rigid->triggerWidth = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("trigger_height"); it3 != it2->value.MemberEnd()) {
					rigid->triggerHeight = it3->value.GetFloat();
				}
				if (auto it3 = it2->value.FindMember("trigger_radius"); it3 != it2->value.MemberEnd()) {
					rigid->triggerRadius = it3->value.GetFloat();
				}
			} else if (compon->type == "ParticleSystem") {
				// do nothing cause it's already set
			}
			else {
				for (auto it3 = it2->value.MemberBegin(); it3 != it2->value.MemberEnd(); ++it3) {
					if (strcmp(it3->name.GetString(), "type") != 0) {
						if (it3->value.IsString())
							(ref)[it3->name.GetString()] = it3->value.GetString();
						else if (it3->value.IsInt())
							(ref)[it3->name.GetString()] = it3->value.GetInt();
						else if (it3->value.IsFloat())
							(ref)[it3->name.GetString()] = it3->value.GetFloat();
						else if (it3->value.IsBool())
							(ref)[it3->name.GetString()] = it3->value.GetBool();
					}
				}
				(ref)["key"] = it2->name.GetString();
				(ref)["enabled"] = true;
			}
		}
	}
}

LuaRef Actor::getComponentType(const std::string& key) {
	LuaRef ref(luaState);
	if (auto it = componentsByType.find(key); it != componentsByType.end() && !it->second.empty()) {
		ref = it->second[0]->first;
	}
	return ref;
}

LuaRef Actor::getComponentByKey(const std::string& key) {
	LuaRef ref(luaState);
	if (auto it = componentsByKey.find(key); it != componentsByKey.end()) {
		ref = it->second->first;
	}
	return ref;
}

Component * Actor::getCompPointerByKey(const std::string &key) {
	Component* ptr = nullptr;
	if (auto it = componentsByKey.find(key); it != componentsByKey.end()) {
		ptr = it->second;
	}
	return ptr;
}

LuaRef Actor::getComponentTypeAll(const std::string& key) {
	LuaRef ref = newTable(luaState);
    auto component = componentsByType[key];
    int counter = 1;
    for (const auto i : component) {
        ref[counter] = i->first;
        counter++;
    }
	return ref;
}

LuaRef Actor::addComponent(const std::string &type){
    static int componentsAdded = 0;
	std::ostringstream key;
    key << 'r' << componentsAdded;
	if (type == "Rigidbody") {
		auto* rb = new RigidBody();
		rb->key = key.str();
		rb->enabled = true;
		rb->initialized = false;
		componentsByKey[key.str()] = rb;
		componentsByType[type].push_back(rb);
		addedThisFrame.push_back(rb);
		rb->first = rb;
		rb->actor = this;
		return rb->first;
	}
	if (type == "ParticleSystem") {
		auto* ps = new ParticleSystem();
		ps->key = key.str();
		ps->initialized = false;
		componentsByKey[key.str()] = ps;
		componentsByType[type].push_back(ps);
		addedThisFrame.push_back(ps);
		ps->first = ps;
		ps->actor = this;
		return ps->first;
	}
	auto* newComponent = new Component();
    newComponent->first = getComponent(type);
    
    (newComponent->first)["key"] = key.str();
	newComponent->initialized = false;
	newComponent->type = type;
	(newComponent->first)["enabled"] = true;
	(newComponent->first)["actor"] = this;
	LuaRef start = newComponent->first["OnStart"];
	if (start.isFunction()) newComponent->onStart = [](LuaRef ref) {
		ref["OnStart"](ref);
		};
	LuaRef update = newComponent->first["OnUpdate"];
	if (update.isFunction()) newComponent->onUpdate = [](LuaRef ref) {
		ref["OnUpdate"](ref);
		};
	LuaRef lateupdate = newComponent->first["OnLateUpdate"];
	if (lateupdate.isFunction()) newComponent->onLateUpdate = [](LuaRef ref) {
		ref["OnLateUpdate"](ref);
		};
	LuaRef onDestroy = newComponent->first["OnDestroy"];
	if (onDestroy.isFunction()) newComponent->onDestroyed = [](LuaRef ref) {
		ref["OnDestroy"](ref);
		};
	LuaRef collisionEnter = newComponent->first["OnCollisionEnter"];
	if (collisionEnter.isFunction()) newComponent->onCollisionEnter = [](LuaRef ref, Collision& col) {
		ref["OnCollisionEnter"](ref, col);
		};
	LuaRef collisionExit = newComponent->first["OnCollisionExit"];
	if (collisionExit.isFunction()) newComponent->onCollisionExit = [](LuaRef ref, Collision& col) {
		ref["OnCollisionExit"](ref, col);
		};
	LuaRef triggerEnter = newComponent->first["OnTriggerEnter"];
	if (triggerEnter.isFunction()) newComponent->onTriggerEnter = [](LuaRef ref, Collision& col) {
		ref["OnTriggerEnter"](ref, col);
		};
	LuaRef triggerExit = newComponent->first["OnTriggerExit"];
	if (triggerExit.isFunction()) newComponent->onTriggerExit = [](LuaRef ref, Collision& col) {
		ref["OnTriggerExit"](ref, col);
		};


    componentsByKey[key.str()] = newComponent;
    componentsByType[type].push_back(newComponent);
    addedThisFrame.push_back(newComponent);
	componentsAdded++;
    return newComponent->first;
}

void Actor::removeComponent(const LuaRef& component) {
	for (auto&[fst, snd] : components) {
		if (snd->first == component) {
			removedThisFrame.push_back(snd);
			break;
		}
	}
	componentsByKey.erase(component["key"].cast<string>());
	component["enabled"] = false;
    auto& vec = componentsByType[removedThisFrame.back()->type];
    vec.erase(std::find(vec.begin(), vec.end(), removedThisFrame.back()));
}

void Scene::onStart() {
	for (Actor* actor : actors) {
		for (auto& componentPair : actor->components) {
			LuaRef component = componentPair.second->first;
			try {
				if (!componentPair.second->initialized && (component)["enabled"] && componentPair.second->onStart) {
					(componentPair.second->onStart)(component);
					componentPair.second->initialized = true;
				}
			}
			catch (const LuaException& e) {
				ReportError(actor->name, e);
				componentPair.second->initialized = true;
			}
		}
	}
}


void Scene::dontDestroy(Actor* actor) {
	actor->dontDestroy = true;
}

string Scene::getCurrent() {
	return globalSceneRef->name;
}

void Scene::load(const std::string& newScene) {
	globalSceneRef->nextScene = newScene;
}

Scene::~Scene() {
	actorsByName.clear();
	for (const Actor* actor : actors) {
		if (!actor->dontDestroy) {
			delete actor;
		}
	}
}

Scene& Scene::operator=(const Scene& other) {
	actors = other.actors;
	actorsByName = other.actorsByName;
	name = other.name;
	nextScene = "";
	cameraPos = other.cameraPos;
	templates = other.templates;
	return *this;
}

void Actor::update() {
	
    for (auto& componentPair : components) {
        LuaRef& component = componentPair.second->first;
		try {
			if ((component)["enabled"] && componentPair.second->onUpdate) {
				(componentPair.second->onUpdate)(component);
			}
		}
		catch (const LuaException& e) {
			ReportError(name, e);
		}
	}
}

void Actor::lateUpdate() {
    for (auto& componentPair : components) {
        LuaRef& component = componentPair.second->first;
		try {
			if ((component)["enabled"] && componentPair.second->onLateUpdate) {
				(componentPair.second->onLateUpdate)(component);
			}
		}
		catch (const LuaException& e) {
			ReportError(name, e);
		}
	}
    for (Component* ref : addedThisFrame) {
		components[ref->first["key"]] = ref;
    }
    for (const auto it : removedThisFrame) {
		const auto component = components.find(it->first["key"].cast<string>());
#ifdef __APPLE__
        component->second->kindaADestructor();
#endif
        components.erase(component->first);
		delete component->second;
    }
	addedThisFrame.clear();
	removedThisFrame.clear();
}

// windows try

Actor::Actor(const Actor& other) {
	name = other.name;
	for (const auto& component : other.components) {
		auto* compon = component.second->clone();
		compon->first["actor"] = this;
		components[(component.second->first)["key"]] = compon;
	}
	for (auto& pair : components) {
	    Component* ref = pair.second;
		string key = (ref->first)["key"];
		string type = ref->type;
		componentsByKey[key] = ref;
	    componentsByType[type].push_back(ref);
	}
}

Actor& Actor::operator=(const Actor &other){
    auto act = Actor(other);
    std::swap(components, act.components);
    std::swap(componentsByKey, act.componentsByKey);
    std::swap(componentsByType, act.componentsByType);
    std::swap(name, act.name);
    return *this;
}

Actor::~Actor(){
    for (auto&[fst, snd] : components) {
        delete snd;
    }
}

Component::Component() : first(LuaRef(luaState)) {
	
}

Component::Component(const Component& other) : first (newTable(luaState)) {
	// copy constructor with establishInheritance
	establishInheritance(first, other.first);
	onStart = other.onStart;
	onUpdate = other.onUpdate;
	onLateUpdate = other.onLateUpdate;
	onCollisionEnter = other.onCollisionEnter;
	onCollisionExit = other.onCollisionExit;
	onTriggerEnter = other.onTriggerEnter;
	onTriggerExit = other.onTriggerExit;
	onDestroyed = other.onDestroyed;
	type = other.type;
	initialized = false;
}

Component* Component::clone() {
	return new Component(*this);
}

void Component::serialize(Serializer &serial) {
	serial.writeTable(first);
}


Component::~Component() {
	if (onDestroyed) onDestroyed(first);
}

void Component::kindaADestructor() const {
    if (onDestroyed) onDestroyed(first);
}
