#ifndef RAYCASTING_H
#define RAYCASTING_H

#include "lua.hpp"
#include "LuaBridge.h"

#ifdef __APPLE__
#include "box2d.h"
#else
#include "../Box2D/include/box2d/box2d.h"
#endif

#include "RigidBody.h"
#include "scene.hpp"

inline std::vector<HitResult> raycastResults;

class singleRaycast : public b2RayCastCallback {

	float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
		raycastResults.clear();
		
		auto* actor = reinterpret_cast<Actor*>(fixture->GetUserData().pointer);
		if (!actor) return 1.0f;
		raycastResults.emplace_back(actor, point, normal, (fixture->GetFilterData().categoryBits & 4) != 0);
		return fraction;
	}
};

inline singleRaycast singleInstance;

class multiRaycast : public b2RayCastCallback {

	float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
		auto* actor = reinterpret_cast<Actor*>(fixture->GetUserData().pointer);
		if (!actor) return 1.0f;
		raycastResults.emplace_back(actor, point, normal, (fixture->GetFilterData().categoryBits & 4) != 0);
		return 1.0f;
	}
};

inline multiRaycast multiInstance;

luabridge::LuaRef raycast(b2Vec2 pos, b2Vec2 dir, float dist) {
	luabridge::LuaRef ref = luabridge::LuaRef(luaState);
	if (!RigidBody::world) return ref;
	raycastResults.clear();
	dir.Normalize();
	RigidBody::world->RayCast(&singleInstance, pos, pos + (dist * dir));
	if (!raycastResults.empty())
		ref = raycastResults[0];
	return ref;
}

b2Vec2 base;

luabridge::LuaRef raycastAll(b2Vec2 pos, b2Vec2 dir, float dist) {
	luabridge::LuaRef ref = luabridge::newTable(luaState);
	if (!RigidBody::world) return ref;
	base = pos;
	raycastResults.clear();
	dir.Normalize();
	RigidBody::world->RayCast(&multiInstance, pos, pos + (dist * dir));
	std::sort(raycastResults.begin(), raycastResults.end(), [](const HitResult& a, const HitResult& b) {
		return b2DistanceSquared(base, a.point) < b2DistanceSquared(base, b.point);
		});
	for (int i = 0; i < raycastResults.size(); i++) {
		ref[i + 1] = raycastResults[i];
	}
	return ref;
}

#endif