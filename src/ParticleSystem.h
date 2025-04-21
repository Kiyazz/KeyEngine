//
// Created by kiyazz on 3/24/25.
//

#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include <vector>
#include <queue>

#include "scene.hpp"
#include "Helper.h"

#include "SDL.h"

class ParticleSystem final : public Component {
public:
    std::queue<int> freeIndices;
    std::vector<glm::vec3> particles;
    std::vector<glm::vec2> vels;
    std::vector<glm::vec2> rotations;
    std::vector<float> scales;

    RandomEngine emitAngle;
    RandomEngine emitRadius;
    RandomEngine rotationDist;
    RandomEngine scaleDist;
    RandomEngine speedDist;
    RandomEngine rotSpeedDist;
    std::string key;
    std::string image;
    SDL_Texture* texture = nullptr;
    Actor* actor;
    glm::vec2 startPos = {0.0f, 0.0f};
    glm::vec2 startSpeed = {0.0f, 0.0f};
    glm::vec2 rotationSpeed = {0.0f, 0.0f};
    glm::vec2 accel = {0.0f, 0.0f};
    glm::vec2 startScale = {1.0f, 1.0f};
    glm::vec2 rotationRange = {0.0f, 0.0f};
    glm::vec2 emitRadiusRange = {0.0f, 0.5f};
    glm::vec2 emitAngleRange = {0.0f, 360.0f};
    glm::u8vec4 startColor = {255, 255, 255, 255};
    glm::u8vec4 endColor = {255, 255, 255, 255};
    float dragFactor = 1.0f;
    float angularDragFactor = 1.0f;
    float lifetimePerFrame{};
    float lifetime1{};
    float endScale = -9999.0f;
    size_t generated = 0;
    int framesBetweenBursts = 1;
    int burstQuantity = 1;
    int durationFrames = 300;
    int sortingOrder = 9999;
    int frameCount = 0;
    bool playing = true;
    bool enabled = true;
    bool endR = false, endG = false, endB = false, endA = false;

    void burst();
    void stop();
    void play();
    void setX(float x);
    float getX() const;
    void setY(float y);
    float getY() const;
    void setScaleMin(float scale);
    float getScaleMin() const;
    void setScaleMax(float scale);
    float getScaleMax() const;
    uint8_t getR() const;
    void setR(uint8_t);
    uint8_t getG() const;
    void setG(uint8_t);
    uint8_t getB() const;
    void setB(uint8_t);
    uint8_t getA() const;
    void setA(uint8_t);
    uint8_t getEndR() const;
    void setEndR(uint8_t);
    uint8_t getEndG() const;
    void setEndG(uint8_t);
    uint8_t getEndB() const;
    void setEndB(uint8_t);
    uint8_t getEndA() const;
    void setEndA(uint8_t);
    ParticleSystem();
    explicit ParticleSystem(rapidjson::Value& json);
    ParticleSystem(const ParticleSystem& other) = default;

    explicit ParticleSystem(Deserializer & serial);

    void serialize(Serializer &serial) override;

    Component* clone() override;
};



#endif //PARTICLESYSTEM_H
