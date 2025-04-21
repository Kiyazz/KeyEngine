//
// Created by kiyazz on 3/24/25.
//

#include "RigidBody.h"

#include "lua.hpp"
#include "LuaBridge.h"
#include "serializer.h"

using namespace luabridge;


b2Vec2 RigidBody::getPosition() const {
	if (!body) return {x, y};
	return body->GetPosition();
}

float RigidBody::getRotation() const {
	if (!body) return rotation * (180 / b2_pi);
	return body->GetAngle() * (180 / b2_pi);
}

void onStartFun(LuaRef ref) {
	auto *rb = ref.cast<RigidBody *>();
	b2BodyDef def;
	def.position = {rb->x, rb->y};
	def.angle = rb->rotation * (b2_pi / 180.0f);

	def.angularDamping = rb->angularDampening;
	def.gravityScale = rb->gravityScale;
	def.bullet = rb->precise;
	if (rb->bodyType == "dynamic") def.type = b2_dynamicBody;
	else if (rb->bodyType == "static") def.type = b2_staticBody;
	else def.type = b2_kinematicBody;

	rb->body = RigidBody::world->CreateBody(&def);
	if (rb->has_collider) {
		b2FixtureDef fixture;
		b2PolygonShape shape;
		if (rb->colliderType == "box") {
			shape.SetAsBox(0.5f * rb->width, 0.5f * rb->height);
			fixture.shape = &shape;
		} else {
			auto circle = new b2CircleShape();
			circle->m_radius = rb->radius;
			fixture.shape = circle;
		}
		fixture.isSensor = false;
		fixture.friction = rb->friction;
		fixture.restitution = rb->bounciness;
		fixture.density = rb->density;
		fixture.filter.categoryBits = 2;
		fixture.filter.maskBits = 2;
		fixture.userData.pointer = reinterpret_cast<uintptr_t>(rb->actor);
		rb->body->CreateFixture(&fixture);
		if (rb->colliderType == "circle") {
			delete fixture.shape;
		}
	}
	if (rb->has_trigger) {
		b2FixtureDef fixture;
		b2PolygonShape shape;
		if (rb->triggerType == "box") {
			shape.SetAsBox(0.5f * rb->triggerWidth, 0.5f * rb->triggerHeight);
			fixture.shape = &shape;
		} else {
			auto circle = new b2CircleShape();
			circle->m_radius = rb->triggerRadius;
			fixture.shape = circle;
		}
		fixture.isSensor = true;
		fixture.density = rb->density;
		fixture.restitution = rb->bounciness;
		fixture.friction = rb->friction;
		fixture.filter.categoryBits = 4;
		fixture.filter.maskBits = 4;
		fixture.userData.pointer = reinterpret_cast<uintptr_t>(rb->actor);
		rb->body->CreateFixture(&fixture);
		if (rb->triggerType == "circle") {
			delete fixture.shape;
		}
	}
	if (!rb->has_collider && !rb->has_trigger) {
		b2PolygonShape shape;
		shape.SetAsBox(rb->width * 0.5f, rb->height * 0.5f);
		b2FixtureDef fixture;
		fixture.shape = &shape;
		fixture.density = rb->density;
		fixture.isSensor = true;
		fixture.filter.maskBits = 0;
		rb->body->CreateFixture(&fixture);
	}
}

RigidBody::RigidBody(float x, float y, float angle, float angularFriction, float density, std::string bodyType,
                     float gravityScale, bool precise, bool collider, bool trigger)
	: bodyType(std::move(bodyType)), x(x), y(y), rotation(angle), angularDampening(angularFriction), density(density),
	  gravityScale(gravityScale), precise(precise), has_collider(collider), has_trigger(trigger) {
	if (!world) {
		world = new b2World(b2Vec2(0.0f, 9.8f));
	}
	onStart = onStartFun;
}

RigidBody::RigidBody(Deserializer &serial, Actor* act) {
	actor = act;
	enabled = serial.readBool();
	x = serial.readFloat();
	y = serial.readFloat();
	bodyType = serial.readString();
	rotation = serial.readFloat();
	gravityScale = serial.readFloat();
	angularDampening = serial.readFloat();
	density = serial.readFloat();
	precise = serial.readBool();
	has_collider = serial.readBool();
	has_trigger = serial.readBool();
	colliderType = serial.readString();
	width = serial.readFloat();
	height = serial.readFloat();
	radius = serial.readFloat();
	friction = serial.readFloat();
	bounciness = serial.readFloat();
	triggerType = serial.readString();
	triggerWidth = serial.readFloat();
	triggerHeight = serial.readFloat();
	triggerRadius = serial.readFloat();
	initialized = serial.readBool();
	if (initialized) {
		onStartFun(LuaRef(luaState, this));
		setVelocity(serial.readb2Vec2());
		setAngularVelocity(serial.readFloat());
		AddForce(serial.readb2Vec2());
		body->ApplyTorque(serial.readFloat(), true);
	}
}

void RigidBody::serialize(Serializer &serial) {
	serial.writeBool(enabled);
	serial.writeFloat(getPosition().x);
	serial.writeFloat(getPosition().y);
	serial.writeString(bodyType);
	serial.writeFloat(getRotation());
	serial.writeFloat(getGravityScale());
	serial.writeFloat(angularDampening);
	serial.writeFloat(density);
	serial.writeBool(precise);
	serial.writeBool(has_collider);
	serial.writeBool(has_trigger);
	serial.writeString(colliderType);
	serial.writeFloat(width);
	serial.writeFloat(height);
	serial.writeFloat(radius);
	serial.writeFloat(friction);
	serial.writeFloat(bounciness);
	serial.writeString(triggerType);
	serial.writeFloat(triggerWidth);
	serial.writeFloat(triggerHeight);
	serial.writeFloat(triggerRadius);
	bool bodyExists = body != nullptr;
	serial.writeBool(bodyExists);
	if (bodyExists) {
		serial.writeVec2(getVelocity());
		serial.writeFloat(getAngularVelocity());
		serial.writeVec2(getForce());
		serial.writeFloat(getTorque());
	}
}

void RigidBody::AddForce(b2Vec2 vec) {
	body->ApplyForceToCenter(vec, true);
}

void RigidBody::setVelocity(b2Vec2 vec) {
	body->SetLinearVelocity(vec);
}

void RigidBody::setPosition(b2Vec2 vec) {
	if (body == nullptr) {
		x = vec.x;
		y = vec.y;
	} else body->SetTransform(vec, body->GetAngle());
}

void RigidBody::setRotation(float degrees) {
	if (body == nullptr) {
		rotation = degrees * (b2_pi / 180.0f);
	} else body->SetTransform(body->GetPosition(), degrees * (b2_pi / 180.0f));
}

void RigidBody::setAngularVelocity(float degrees) {
	body->SetAngularVelocity(degrees * (b2_pi / 180.0f));
}

void RigidBody::setGravityScale(float scale) {
	if (!body) gravityScale = scale;
	else body->SetGravityScale(scale);
}

void RigidBody::setUpDirection(b2Vec2 vec) {
	vec.Normalize();
	float angle = glm::atan(vec.x, -vec.y);
	if (!body) rotation = angle;
	else body->SetTransform(body->GetPosition(), angle);
}

void RigidBody::setRightDirection(b2Vec2 vec) {
	vec.Normalize();
	float angle = glm::atan(vec.x, -vec.y) - (b2_pi / 2.0f);
	if (!body) rotation = angle;
	else body->SetTransform(body->GetPosition(), angle);
}

b2Vec2 RigidBody::getForce() const {
	return body->GetForce();
}

b2Vec2 RigidBody::getVelocity() const {
	return body->GetLinearVelocity();
}

float RigidBody::getAngularVelocity() const {
	return body->GetAngularVelocity() * (180.0f / b2_pi);
}

float RigidBody::getGravityScale() const {
	if (!body) return gravityScale;
	return body->GetGravityScale();
}

b2Vec2 RigidBody::getUpDirection() const {
	float angle;
	if (!body) angle = rotation;
	else angle = body->GetAngle();
	angle -= (b2_pi * 0.5f);
	b2Vec2 result = {glm::cos(angle), glm::sin(angle)};
	result.Normalize();
	return result;
}

b2Vec2 RigidBody::getRightDirection() const {
	float angle;
	if (!body) angle = rotation;
	else angle = body->GetAngle();
	b2Vec2 result = {glm::cos(angle), glm::sin(angle)};
	result.Normalize();
	return result;
}

RigidBody::~RigidBody() {
	if (body) world->DestroyBody(body);
}

Component *RigidBody::clone() {
	auto *body = new RigidBody(x, y, rotation, angularDampening, density, bodyType, gravityScale, precise, has_collider,
	                           has_trigger);
	body->width = width;
	body->height = height;
	body->friction = friction;
	body->bounciness = bounciness;
	body->radius = radius;
	body->triggerHeight = triggerHeight;
	body->triggerRadius = triggerRadius;
	body->triggerType = triggerType;
	body->triggerWidth = triggerWidth;
	body->colliderType = colliderType;
	body->key = key;
	enabled = true;
	initialized = false;
	body->first = body;
	return body;
}

float RigidBody::getTorque() const {
	return body->getTorque();
}

const b2Vec2 sentinel = {-999.0f, -999.0f};

void ContactListener::BeginContact(b2Contact *contact) {
	b2Fixture *A = contact->GetFixtureA();
	b2Fixture *B = contact->GetFixtureB();
	Collision col;
	b2WorldManifold manifold;
	contact->GetWorldManifold(&manifold);
	bool trigger = (A->GetFilterData().categoryBits & 4) != 0;
	col.relativeVelocity = A->GetBody()->GetLinearVelocity() - B->GetBody()->GetLinearVelocity();
	// if trigger use sentinel
	if (trigger) {
		col.point = sentinel;
		col.normal = sentinel;
	} else {
		col.point = manifold.points[0];
		col.normal = manifold.normal;
	}
	col.other = reinterpret_cast<Actor *>(B->GetUserData().pointer);
	auto *actor = reinterpret_cast<Actor *>(A->GetUserData().pointer);

	for (std::pair<const std::string, Component *> &c: actor->components) {
		Component *component = c.second;
		if (trigger && component->onTriggerEnter)
			try {
				component->onTriggerEnter(component->first, col);
			}
		catch (LuaException& e) {
			ReportError(actor->name, e);
		}
		else if (!trigger && component->onCollisionEnter)
			try {
				component->onCollisionEnter(component->first, col);
			}
		catch (LuaException& e) {
			ReportError(actor->name, e);
		}
	}
	std::swap(col.other, actor);
	for (std::pair<const std::string, Component *> &c: actor->components) {
		Component *component = c.second;
		if (trigger && component->onTriggerEnter)
			try {
				component->onTriggerEnter(component->first, col);
			}
			catch (LuaException& e) {
				ReportError(actor->name, e);
			}
		else if (!trigger && component->onCollisionEnter)
			try {
				component->onCollisionEnter(component->first, col);
			}
			catch (LuaException& e) {
				ReportError(actor->name, e);
			}
	}
}

void ContactListener::EndContact(b2Contact *contact) {
	b2Fixture *A = contact->GetFixtureA();
	b2Fixture *B = contact->GetFixtureB();
	Collision col;
	col.point = sentinel;
	col.relativeVelocity = A->GetBody()->GetLinearVelocity() - B->GetBody()->GetLinearVelocity();
	col.normal = sentinel;
	col.other = reinterpret_cast<Actor *>(B->GetUserData().pointer);
	auto *actor = reinterpret_cast<Actor *>(A->GetUserData().pointer);
	bool trigger = (A->GetFilterData().categoryBits & 4) != 0;
	for (std::pair<const std::string, Component *> &c: actor->components) {
		Component *component = c.second;
		if (trigger && component->onTriggerExit)
			try {
				component->onTriggerExit(component->first, col);
			}
		catch (LuaException& e) {
			ReportError(actor->name, e);
		}
		else if (!trigger && component->onCollisionExit)
			try {
				component->onCollisionExit(component->first, col);
			}
		catch (LuaException& e) {
			ReportError(actor->name, e);
		}
	}
	std::swap(col.other, actor);
	for (std::pair<const std::string, Component *> &c: actor->components) {
		Component *component = c.second;
		if (trigger && component->onTriggerExit)
			try {
				component->onTriggerExit(component->first, col);
			}
		catch (LuaException& e) {
			ReportError(actor->name, e);
		}
		else if (!trigger && component->onCollisionExit)
			try {
				component->onCollisionExit(component->first, col);
			}
		catch (LuaException& e) {
			ReportError(actor->name, e);
		}
	}
}
