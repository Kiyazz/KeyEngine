//
// Created by kiyazz on 3/24/25.
//

#include "ParticleSystem.h"

#include "Rendering.h"
#include "serializer.h"

using namespace luabridge;

void ParticleSystem::burst() {
    for (int j = 0; j < burstQuantity; j++) {
        float angle = glm::radians(emitAngle.Sample());
        float dist = emitRadius.Sample();
        glm::vec3 particle = {startPos.x + dist * glm::cos(angle), startPos.y + dist*glm::sin(angle), 0.0f};
        float rotation = rotationDist.Sample();
        float speed = speedDist.Sample();
        glm::vec2 vel = {glm::cos(angle)*speed, glm::sin(angle)*speed};
        float angularVel = rotSpeedDist.Sample();
        float scale = scaleDist.Sample();
        if (freeIndices.empty()) {
            particles.push_back(particle); // grade again
            vels.push_back(vel);
            rotations.emplace_back(angularVel, rotation);
            scales.push_back(scale);
        }
        else {
            int i = freeIndices.front();
            freeIndices.pop();
            particles[i] = particle;
            vels[i] = vel;
            rotations[i] = { angularVel, rotation };
            scales[i] = scale;
        }
        generated++;
    }
}

void ParticleSystem::stop() {
    playing = false;
}

void ParticleSystem::play() {
    playing = true;
}

void ParticleSystem::setX(float x) {
    startPos.x = x;
}

float ParticleSystem::getX() const {
    return startPos.x;
}

void ParticleSystem::setY(float y) {
    startPos.y = y;
}

float ParticleSystem::getY() const {
    return startPos.y;
}

void ParticleSystem::setScaleMin(float scale)
{
    startScale.x = scale;
}

float ParticleSystem::getScaleMin() const
{
    return startScale.x;
}

void ParticleSystem::setScaleMax(float scale)
{
    startScale.y = scale;
}

float ParticleSystem::getScaleMax() const
{
    return startScale.y;
}

uint8_t ParticleSystem::getR() const
{
    return startColor.r;
}

void ParticleSystem::setR(uint8_t r)
{
    startColor.r = r;
}

uint8_t ParticleSystem::getG() const
{
    return startColor.g;
}

void ParticleSystem::setG(uint8_t g)
{
    startColor.g = g;
}

uint8_t ParticleSystem::getB() const
{
    return startColor.b;
}

void ParticleSystem::setB(uint8_t b)
{
    startColor.b = b;
}

uint8_t ParticleSystem::getA() const
{
    return startColor.a;
}

void ParticleSystem::setA(uint8_t a)
{
    startColor.a = a;
}

uint8_t ParticleSystem::getEndR() const
{
    return endColor.r;
}

void ParticleSystem::setEndR(uint8_t r)
{
    endColor.r = r;
    endR = true;
}

uint8_t ParticleSystem::getEndG() const
{
    return endColor.g;
}

void ParticleSystem::setEndG(uint8_t g)
{
    endColor.g = g;
    endG = true;
}

uint8_t ParticleSystem::getEndB() const
{
    return endColor.b;
}

void ParticleSystem::setEndB(uint8_t b)
{
    endColor.b = b;
    endB = true;
}

uint8_t ParticleSystem::getEndA() const
{
    return endColor.a;
}

void ParticleSystem::setEndA(uint8_t a)
{
    endColor.a = a;
    endA = true;
}

void onUpdateFunc(LuaRef ref) {
    auto* ps = ref.cast<ParticleSystem*>();
    if (ps->frameCount % ps->framesBetweenBursts == 0 && ps->playing) {
        ps->burst();
    }

    // render + updates particles
    auto& vec = ps->particles;
    for (int i = 0; i < vec.size(); ++i) {
        // is particle in lifetime
        if (vec[i].z <= ps->lifetime1) {
            // do updates
            if (vec[i].z >= 0.999999f) {
                // particle just became out of lifetime
                ps->freeIndices.push(i);
                // make sure particle can't hit this condition again
                vec[i].z += 1;
                continue;
            }
            ps->vels[i].x += ps->accel.x;
            ps->vels[i].y += ps->accel.y;
            ps->vels[i] *= ps->dragFactor;
            ps->rotations[i].x *= ps->angularDragFactor;
            ps->rotations[i].y += ps->rotations[i].x;
            vec[i].x += ps->vels[i].x;
            vec[i].y += ps->vels[i].y;

            float scale;
            if (ps->endScale != -9999.0f)
                scale = glm::mix(ps->scales[i], ps->endScale, vec[i].z);
            else scale = ps->scales[i];
            uint8_t r, g, b, a;
            if (ps->endR) r = glm::mix(ps->startColor.r, ps->endColor.r, vec[i].z);
            else r = ps->startColor.r;
            if (ps->endG) g = glm::mix(ps->startColor.g, ps->endColor.g, vec[i].z);
            else g = ps->startColor.g;
            if (ps->endB) b = glm::mix(ps->startColor.b, ps->endColor.b, vec[i].z);
            else b = ps->startColor.b;
            if (ps->endA) a = glm::mix(ps->startColor.a, ps->endColor.a, vec[i].z);
            else a = ps->startColor.a;
            drawParticle(ps->texture, vec[i].x, vec[i].y, ps->rotations[i].y, scale,
                r, g, b, a, ps->sortingOrder);
            vec[i].z += ps->lifetimePerFrame;
        }
    }
    ps->frameCount++;
}

void onUpdateFuncNoColor(LuaRef ref) {
    auto* ps = ref.cast<ParticleSystem*>();
    if (ps->frameCount % ps->framesBetweenBursts == 0 && ps->playing) {
        ps->burst();
    }

    // render + updates particles
    auto& vec = ps->particles;
    for (int i = 0; i < vec.size(); ++i) {
        // is particle in lifetime
        if (vec[i].z <= ps->lifetime1) {
            // do updates
            if (vec[i].z >= 0.999999f) {
                // particle just became out of lifetime
                ps->freeIndices.push(i);
                // make sure particle can't hit this condition again
                vec[i].z += 1;
                continue;
            }
            ps->vels[i].x += ps->accel.x;
            ps->vels[i].y += ps->accel.y;
            ps->vels[i] *= ps->dragFactor;
            ps->rotations[i].x *= ps->angularDragFactor;
            ps->rotations[i].y += ps->rotations[i].x;
            vec[i].x += ps->vels[i].x;
            vec[i].y += ps->vels[i].y;

            float scale;
            if (ps->endScale != -9999.0f)
                scale = glm::mix(ps->scales[i], ps->endScale, vec[i].z);
            else scale = ps->scales[i];
            drawParticle(ps->texture, vec[i].x, vec[i].y, ps->rotations[i].y, scale,
                ps->startColor.r, ps->startColor.g, ps->startColor.b, ps->startColor.a, ps->sortingOrder);
            vec[i].z += ps->lifetimePerFrame;
        }
    }
    ps->frameCount++;
}

void onUpdateFuncNoScale(LuaRef ref) {
    auto* ps = ref.cast<ParticleSystem*>();
    if (ps->frameCount % ps->framesBetweenBursts == 0 && ps->playing) {
        ps->burst();
    }

    // render + updates particles
    auto& vec = ps->particles;
    for (int i = 0; i < vec.size(); ++i) {
        // is particle in lifetime
        if (vec[i].z <= ps->lifetime1) {
            // do updates
            if (vec[i].z >= 0.999999f) {
                // particle just became out of lifetime
                ps->freeIndices.push(i);
                // make sure particle can't hit this condition again
                vec[i].z += 1;
                continue;
            }
            ps->vels[i].x += ps->accel.x;
            ps->vels[i].y += ps->accel.y;
            ps->vels[i] *= ps->dragFactor;
            ps->rotations[i].x *= ps->angularDragFactor;
            ps->rotations[i].y += ps->rotations[i].x;
            vec[i].x += ps->vels[i].x;
            vec[i].y += ps->vels[i].y;

            uint8_t r, g, b, a;
            if (ps->endR) r = glm::mix(ps->startColor.r, ps->endColor.r, vec[i].z);
            else r = ps->startColor.r;
            if (ps->endG) g = glm::mix(ps->startColor.g, ps->endColor.g, vec[i].z);
            else g = ps->startColor.g;
            if (ps->endB) b = glm::mix(ps->startColor.b, ps->endColor.b, vec[i].z);
            else b = ps->startColor.b;
            if (ps->endA) a = glm::mix(ps->startColor.a, ps->endColor.a, vec[i].z);
            else a = ps->startColor.a;
            drawParticle(ps->texture, vec[i].x, vec[i].y, ps->rotations[i].y, ps->scales[i],
                r, g, b, a, ps->sortingOrder);
            vec[i].z += ps->lifetimePerFrame;
        }
    }
    ps->frameCount++;
}

void onUpdateFuncNoNothing(LuaRef ref) {
    auto* ps = ref.cast<ParticleSystem*>();
    if (ps->frameCount % ps->framesBetweenBursts == 0 && ps->playing) {
        ps->burst();
    }

    // render + updates particles
    auto& vec = ps->particles;
    for (int i = 0; i < vec.size(); ++i) {
        // is particle in lifetime
        if (vec[i].z <= ps->lifetime1) {
            // do updates
            if (vec[i].z >= 0.999999f) {
                // particle just became out of lifetime
                ps->freeIndices.push(i);
                // make sure particle can't hit this condition again
                vec[i].z += 1;
                continue;
            }
            ps->vels[i].x += ps->accel.x;
            ps->vels[i].y += ps->accel.y;
            ps->vels[i] *= ps->dragFactor;
            ps->rotations[i].x *= ps->angularDragFactor;
            ps->rotations[i].y += ps->rotations[i].x;
            vec[i].x += ps->vels[i].x;
            vec[i].y += ps->vels[i].y;
            drawParticle(ps->texture, vec[i].x, vec[i].y, ps->rotations[i].y, ps->scales[i],
                ps->startColor.r, ps->startColor.g, ps->startColor.b, ps->startColor.a, ps->sortingOrder);
            vec[i].z += ps->lifetimePerFrame;
        }
    }
    ps->frameCount++;
}

void onStartFunc(LuaRef ref) {
    auto* ps = ref.cast<ParticleSystem*>();
    ps->emitAngle = RandomEngine(ps->emitAngleRange.x, ps->emitAngleRange.y, 298);
    ps->emitRadius = RandomEngine(ps->emitRadiusRange.x, ps->emitRadiusRange.y, 404);
    ps->rotationDist = RandomEngine(ps->rotationRange.x, ps->rotationRange.y, 440);
    ps->speedDist = RandomEngine(ps->startSpeed.x, ps->startSpeed.y, 498);
    ps->scaleDist = RandomEngine(ps->startScale.x, ps->startScale.y, 494);
    ps->rotSpeedDist = RandomEngine(ps->rotationSpeed.x, ps->rotationSpeed.y, 305);
    if (ps->durationFrames < 1) ps->durationFrames = 1;
    if (ps->burstQuantity < 1) ps->burstQuantity = 1;
    if (ps->framesBetweenBursts < 1) ps->framesBetweenBursts = 1;
    ps->lifetimePerFrame = 1.0f / static_cast<float>(ps->durationFrames);
    if (ps->durationFrames <= 60) {
        int significand = static_cast<int>(ps->lifetimePerFrame * static_cast<float>(100000000));
        ps->lifetimePerFrame = (static_cast<float>(significand) / static_cast<float>(100000000));
        ps->lifetime1 = 0.999999f + ps->lifetimePerFrame;
    }
    else {
        int significand = static_cast<int>(ps->lifetimePerFrame * static_cast<float>(1000000000));
        ps->lifetimePerFrame = static_cast<float>(significand) / static_cast<float>(1000000000);
        ps->lifetime1 = 0.999999f + ps->lifetimePerFrame;
    }
    ps->texture = getImage(renderer, ps->image);
    if (ps->endScale == -9999.0f) {
        ps->onUpdate = onUpdateFuncNoScale;
    }
    if (!ps->endA && !ps->endB && !ps->endG && !ps->endR) {
        ps->onUpdate = onUpdateFuncNoColor;
    }
    if (ps->endScale == -9999.0f && !ps->endA && !ps->endB && !ps->endG && !ps->endR) {
        ps->onUpdate = onUpdateFuncNoNothing;
    }
}

ParticleSystem::ParticleSystem(): lifetimePerFrame(0), lifetime1(0) {
    actor = nullptr;
    onStart = &onStartFunc;
    onUpdate = &onUpdateFunc;
    this->type = "ParticleSystem";
}

ParticleSystem::ParticleSystem(rapidjson::Value &json) : ParticleSystem() {
    const auto end = json.MemberEnd();
    if (auto it = json.FindMember("x"); it != end) {
        startPos.x = it->value.GetFloat();
    }
    if (auto it = json.FindMember("y"); it != end) {
        startPos.y = it->value.GetFloat();
    }
    if (auto it = json.FindMember("frames_between_bursts"); it != end) {
        framesBetweenBursts = it->value.GetInt();
    }
    if (auto it = json.FindMember("burst_quantity"); it != end) {
        burstQuantity = it->value.GetInt();
    }
    if (auto it = json.FindMember("rotation_min"); it != end) {
        rotationRange.x = it->value.GetFloat();
    }
    if (auto it = json.FindMember("rotation_max"); it != end) {
        rotationRange.y = it->value.GetFloat();
    }
    if (auto it = json.FindMember("start_scale_min"); it != end) {
        startScale.x = it->value.GetFloat();
    }
    if (auto it = json.FindMember("start_scale_max"); it != end) {
        startScale.y = it->value.GetFloat();
    }
    if (auto it = json.FindMember("start_color_r"); it != end) {
        startColor.r = it->value.GetInt();
    }
    if (auto it = json.FindMember("start_color_g"); it != end) {
        startColor.g = it->value.GetInt();
    }
    if (auto it = json.FindMember("start_color_b"); it != end) {
        startColor.b = it->value.GetInt();
    }
    if (auto it = json.FindMember("start_color_a"); it != end) {
        startColor.a = it->value.GetInt();
    }
    if (auto it = json.FindMember("end_color_r"); it != end) {
        endColor.r = it->value.GetInt();
        endR = true;
    }
    if (auto it = json.FindMember("end_color_g"); it != end) {
        endColor.g = it->value.GetInt();
        endG = true;
    }
    if (auto it = json.FindMember("end_color_b"); it != end) {
        endColor.b = it->value.GetInt();
        endB = true;
    }
    if (auto it = json.FindMember("end_color_a"); it != end) {
        endColor.a = it->value.GetInt();
        endA = true;
    }
    if (auto it = json.FindMember("emit_radius_min"); it != end) {
        emitRadiusRange.x = it->value.GetFloat();
    }
    if (auto it = json.FindMember("emit_radius_max"); it != end) {
        emitRadiusRange.y = it->value.GetFloat();
    }
    if (auto it = json.FindMember("emit_angle_min"); it != end) {
        emitAngleRange.x = it->value.GetFloat();
    }
    if (auto it = json.FindMember("emit_angle_max"); it != end) {
        emitAngleRange.y = it->value.GetFloat();
    }
    if (auto it = json.FindMember("image"); it != end) {
        image = it->value.GetString();
    }
    if (auto it = json.FindMember("sorting_order"); it != end) {
        sortingOrder = it->value.GetInt();
    }
    if (auto it = json.FindMember("duration_frames"); it != end) {
        durationFrames = it->value.GetInt();
    }
    if (auto it = json.FindMember("start_speed_min"); it != end) {
        startSpeed.x = it->value.GetFloat();
    }
    if (auto it = json.FindMember("start_speed_max"); it != end) {
        startSpeed.y = it->value.GetFloat();
    }
    if (auto it = json.FindMember("rotation_speed_min"); it != end) {
        rotationSpeed.x = it->value.GetFloat();
    }
    if (auto it = json.FindMember("rotation_speed_max"); it != end) {
        rotationSpeed.y = it->value.GetFloat();
    }
    if (auto it = json.FindMember("gravity_scale_x"); it != end) {
        accel.x = it->value.GetFloat();
    }
    if (auto it = json.FindMember("gravity_scale_y"); it != end) {
        accel.y = it->value.GetFloat();
    }
    if (auto it = json.FindMember("drag_factor"); it != end) {
        dragFactor = it->value.GetFloat();
    }
    if (auto it = json.FindMember("angular_drag_factor"); it != end) {
        angularDragFactor = it->value.GetFloat();
    }
    if (auto it = json.FindMember("end_scale"); it != end) {
        endScale = it->value.GetFloat();
    }
}

ParticleSystem::ParticleSystem(Deserializer &serial) : ParticleSystem() {
    playing = serial.readBool();
    enabled = serial.readBool();
    startPos = serial.readVec2();
    startScale = serial.readVec2();
    startSpeed = serial.readVec2();
    rotationRange = serial.readVec2();
    rotationSpeed = serial.readVec2();
    accel = serial.readVec2();
    emitAngleRange = serial.readVec2();
    emitRadiusRange = serial.readVec2();
    startColor.r = serial.readChar();
    startColor.g = serial.readChar();
    startColor.b = serial.readChar();
    startColor.a = serial.readChar();
    endColor.r = serial.readChar();
    endColor.g = serial.readChar();
    endColor.b = serial.readChar();
    endColor.a = serial.readChar();
    dragFactor = serial.readFloat();
    angularDragFactor = serial.readFloat();
    lifetime1 = serial.readFloat();
    lifetimePerFrame = serial.readFloat();
    endScale = serial.readFloat();
    sortingOrder = serial.readInt();
    frameCount = serial.readInt();
    framesBetweenBursts = serial.readInt();
    durationFrames = serial.readInt();
    burstQuantity = serial.readInt();
    image = serial.readString();
    texture = getImage(renderer, image);
    endR = serial.readBool();
    endG = serial.readBool();
    endB = serial.readBool();
    endA = serial.readBool();
    generated = serial.readSizeT();
    emitAngle = RandomEngine(emitAngleRange.x, emitAngleRange.y, 298);
    emitRadius = RandomEngine(emitRadiusRange.x, emitRadiusRange.y, 404);
    rotationDist = RandomEngine(rotationRange.x, rotationRange.y, 440);
    speedDist = RandomEngine(startSpeed.x, startSpeed.y, 498);
    scaleDist = RandomEngine(startScale.x, startScale.y, 494);
    rotSpeedDist = RandomEngine(rotationSpeed.x, rotationSpeed.y, 305);
    emitAngle.discard(generated);
    emitRadius.discard(generated);
    rotationDist.discard(generated);
    speedDist.discard(generated);
    scaleDist.discard(generated);
    rotSpeedDist.discard(generated);
    if (endScale == -9999.0f) {
        onUpdate = onUpdateFuncNoScale;
    }
    if (!endA && !endB && !endG && !endR) {
        onUpdate = onUpdateFuncNoColor;
    }
    if (endScale == -9999.0f && !endA && !endB && !endG && !endR) {
        onUpdate = onUpdateFuncNoNothing;
    }
    size_t size = serial.readSizeT();
    particles.reserve(size);
    vels.reserve(size);
    rotations.reserve(size);
    scales.reserve(size);
    for (size_t i = 0; i < size; i++) {
        particles.push_back(serial.readVec3());
        rotations.push_back(serial.readVec2());
        vels.push_back(serial.readVec2());
        scales.push_back(serial.readFloat());
    }
}

void ParticleSystem::serialize(Serializer &serial) {
    serial.writeBool(playing);
    serial.writeBool(enabled);
    serial.writeVec2(startPos);
    serial.writeVec2(startScale);
    serial.writeVec2(startSpeed);
    serial.writeVec2(rotationRange);
    serial.writeVec2(rotationSpeed);
    serial.writeVec2(accel);
    serial.writeVec2(emitAngleRange);
    serial.writeVec2(emitRadiusRange);
    serial.writeChar(startColor.r);
    serial.writeChar(startColor.g);
    serial.writeChar(startColor.b);
    serial.writeChar(startColor.a);
    serial.writeChar(endColor.r);
    serial.writeChar(endColor.g);
    serial.writeChar(endColor.b);
    serial.writeChar(endColor.a);
    serial.writeFloat(dragFactor);
    serial.writeFloat(angularDragFactor);
    serial.writeFloat(lifetime1);
    serial.writeFloat(lifetimePerFrame);
    serial.writeFloat(endScale);
    serial.writeInt(sortingOrder);
    serial.writeInt(frameCount);
    serial.writeInt(framesBetweenBursts);
    serial.writeInt(durationFrames);
    serial.writeInt(burstQuantity);
    serial.writeString(image);
    serial.writeBool(endR);
    serial.writeBool(endG);
    serial.writeBool(endB);
    serial.writeBool(endA);
    serial.writeSizeT(generated);
    serial.writeSizeT(particles.size());
    for (int i = 0; i < particles.size(); ++i) {
        serial.writeVec3(particles[i]);
        serial.writeVec2(rotations[i]);
        serial.writeVec2(vels[i]);
        serial.writeFloat(scales[i]);
    }
}

Component* ParticleSystem::clone() {
    auto* comp = new ParticleSystem(*this);
    comp->first = comp;
    return comp;
}
