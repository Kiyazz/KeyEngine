#ifndef RAYCASTING_H
#define RAYCASTING_H

#include "lua.hpp"
#include "LuaBridge.h"

#include "Box2D/Box2D.hpp"

#include "RigidBody.h"
#include "scene.hpp"

inline std::vector<HitResult> raycastResults;

class singleRaycast final : public b2::RayCastCallback {

	float ReportFixture(b2::Fixture* fixture, const b2::Vec2& point, const b2::Vec2& normal, float fraction) override {
		raycastResults.clear();
		
		auto* actor = static_cast<Actor*>(fixture->GetUserData());
		if (!actor) return 1.0f;
		raycastResults.emplace_back(actor, point, normal, (fixture->GetFilterData().categoryBits & 4) != 0);
		return fraction;
	}
};

inline singleRaycast singleInstance;

class multiRaycast final : public b2::RayCastCallback {

	float ReportFixture(b2::Fixture* fixture, const b2::Vec2& point, const b2::Vec2& normal, float fraction) override {
		auto* actor = static_cast<Actor*>(fixture->GetUserData());
		if (!actor) return 1.0f;
		raycastResults.emplace_back(actor, point, normal, (fixture->GetFilterData().categoryBits & 4) != 0);
		return 1.0f;
	}
};

inline multiRaycast multiInstance;

inline luabridge::LuaRef raycast(b2::Vec2 pos, b2::Vec2 dir, float dist) {
	auto ref = luabridge::LuaRef(luaState);
	if (!RigidBody::world) return ref;
	raycastResults.clear();
	dir.Normalize();
	RigidBody::world->RayCast(&singleInstance, pos, pos + (dist * dir));
	if (!raycastResults.empty())
		ref = raycastResults[0];
	return ref;
}

inline luabridge::LuaRef raycastAll(b2::Vec2 pos, b2::Vec2 dir, float dist) {
	luabridge::LuaRef ref = luabridge::newTable(luaState);
	if (!RigidBody::world) return ref;
	b2::Vec2 base = pos;
	raycastResults.clear();
	dir.Normalize();
	RigidBody::world->RayCast(&multiInstance, pos, pos + (dist * dir));
	std::sort(raycastResults.begin(), raycastResults.end(), [base](const HitResult& a, const HitResult& b) {
		return b2::DistanceSquared(base, a.point) < b2::DistanceSquared(base, b.point);
		});
	for (int i = 0; i < raycastResults.size(); i++) {
		ref[i + 1] = raycastResults[i];
	}
	return ref;
}

#endif