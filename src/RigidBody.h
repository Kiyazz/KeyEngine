//
// Created by kiyazz on 3/24/25.
//



#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include "scene.hpp"
#include "Box2D/Box2D.hpp"

class ContactListener : public b2::ContactListener {

    void BeginContact(b2::Contact *contact) override;
    void EndContact(b2::Contact *contact) override;

};

class RigidBody : public Component {
public:
    std::string bodyType;
    std::string key;
    b2::Body* body = nullptr;
    Actor* actor = nullptr;
    float x, y;
    float rotation;
    float angularDampening, density, gravityScale;

    bool precise;
    bool enabled = true;
    bool has_collider;
    bool has_trigger;
    std::string colliderType = "box";
    float width = 1.0f;
    float height = 1.0f;
    float radius = 0.5f;
    float friction = 0.3f;
    float bounciness = 0.3f;
    std::string triggerType = "box";
    float triggerWidth = 1.0f;
    float triggerHeight = 1.0f;
    float triggerRadius = 0.5f;

    [[nodiscard]] b2::Vec2 getPosition() const;
    [[nodiscard]] float getRotation() const;
    explicit RigidBody(float x = 0.0f, float y= 0.0f, float angle=0.0f, float angularFriction= 0.3f, float density= 1.0f, std::string bodyType = "dynamic", float gravityScale = 1.0f, bool precise = true, bool collider = true, bool trigger = true);
    explicit RigidBody(Deserializer& serial, Actor* act);
    void AddForce(b2::Vec2 vec) const;
    void setVelocity(b2::Vec2 vec) const;
    void setPosition(b2::Vec2 vec);
    void setRotation(float degrees);
    void setAngularVelocity(float degrees) const;
    void setGravityScale(float scale);
    void setUpDirection(b2::Vec2 vec);
    void setRightDirection(b2::Vec2 vec);

    [[nodiscard]] b2::Vec2 getForce() const;
    [[nodiscard]] b2::Vec2 getVelocity() const;
    [[nodiscard]] float getAngularVelocity() const;
    [[nodiscard]] float getGravityScale() const;
    [[nodiscard]] b2::Vec2 getUpDirection() const;
    [[nodiscard]] b2::Vec2 getRightDirection() const;
    static inline b2::World* world = nullptr;
    Component* clone() override;

    [[nodiscard]] float getTorque() const;

    void serialize(Serializer &serial) override;

    ~RigidBody() override;
};



#endif //RIGIDBODY_H
