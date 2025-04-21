#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <fstream>
#include <iosfwd>
#include <sstream>
#include <utility>

#include "luafuncs.h"
#include "scene.hpp"
#include "ParticleSystem.h"
#include "RigidBody.h"

inline bool is_big_endian() {
    union {
        uint32_t i;
        char c[4];
    } b = {0x01020304};
    return b.c[0] == 1;
}

template<typename T>
T swap_endian(T u) {
    union {
        T u;
        unsigned char u8[sizeof(T)];
    } source{}, dest{};

    source.u = u;

    for (size_t k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];

    return dest.u;
}

struct Reference {
    luabridge::LuaRef ref;
    std::string component;
    std::string key;
    size_t actor;
    char type;
    Reference(const luabridge::LuaRef& ref, std::string component, const size_t actor, const char type, std::string key)
    : ref(ref), component(std::move(component)), key(std::move(key)), actor(actor), type(type) {}
};

class SerialError final : public std::exception {
public:
    std::string message = "Serial Error";

    [[nodiscard]] const char* what() const noexcept override {
        return message.c_str();
    }

    SerialError() = default;

    explicit SerialError(std::string str) : message(std::move(str)) {}
};

class Serializer {
    size_t current_pos;
    std::ofstream file;
    inline static std::unordered_set<std::string> excludeSet = {"warn",
"Event",
"type",
"pcall",
"rawlen",
"rawget",
"os",
"ParticleSystem",
"io",
"pairs",
"rawset",
"package",
"coroutine",
"_G",
"print",
"tostring",
"collectgarbage",
"Text",
"Audio",
"Saving",
"Scene",
"Actor",
"string",
"vec2",
"select",
"Application",
"debug",
"Collision",
"Image",
"load",
"HitResult",
"Physics",
"Input",
"Rigidbody",
"require",
"Camera",
"Vector2",
"Debug",
"getmetatable",
"utf8",
"math",
"table",
"xpcall",
"assert",
"ipairs",
"dofile",
"rawequal",
"next",
"tonumber",
"_VERSION",
"setmetatable",
"error",
"loadfile"};

public:
    static void addToExcludeSet(const std::string& s) {
        excludeSet.insert(s);
    }

    void writeString(const std::string &str) {
        const size_t size = str.length();
        const char *buf = str.c_str();
        file.write(buf, static_cast<std::streamsize>(size));
        file.write("\0", 1);
        current_pos += size;
        current_pos++;
    }

    void writeBool(const bool b) {
        if (b) {
            char c[1];
            c[0] = 1;
            file.write(c, 1);
            current_pos++;
        } else {
            char c[1];
            c[0] = 0;
            file.write(c, 1);
            current_pos++;
        }
    }

    void writeChar(const char c) {
        char buf[1];
        buf[0] = c;
        file.write(buf, 1);
        current_pos++;
    }

    void writeShort(short s) {
        if (is_big_endian()) {
            s = swap_endian<short>(s);
        }
        char buf[2];
        memcpy(buf, &s, 2);
        file.write(buf, 2);
        current_pos += 2;
    }

    void writeInt(int i) {
        if (is_big_endian()) {
            i = swap_endian<int>(i);
        }
        char buf[4];
        memcpy(buf, &i, 4);
        file.write(buf, 4);
        current_pos += 4;
    }

    void writeFloat(float f) {
        if (is_big_endian()) {
            f = swap_endian<float>(f);
        }
        char buf[4];
        memcpy(buf, &f, 4);
        file.write(buf, 4);
        current_pos += 4;
    }

    void writeLong(long long i) {
        if (is_big_endian()) {
            i = swap_endian<long long>(i);
        }
        char buf[8];
        memcpy(buf, &i, 8);
        file.write(buf, 8);
        current_pos += 8;
    }

    void writeSizeT(size_t i) {
        if (is_big_endian()) {
            i = swap_endian<size_t>(i);
        }
        char buf[8];
        memcpy(buf, &i, 8);
        file.write(buf, 8);
        current_pos += 8;
    }

    // Function will recursively call writeTable if component is storing tables inside of it
    void writeTable(const luabridge::LuaRef &ref) {
        assert(ref.type() == LUA_TTABLE);
        luabridge::Iterator it(ref);
        size_t count = 0;
        while (!it.isNil()) {
            if (!it.value().isFunction() && !it.value().isNil()) count++;
            ++it;
        }
        writeSizeT(count);
        it = luabridge::Iterator(ref);
        while (!it.isNil()) {
            if (it.value().isFunction()) {
                ++it;
                continue;
            }
            std::string s = it.key();
            writeString(s);
            if (it.value().isTable()) {
                // first test if table is a component
                if (!it.value()["key"].isNil() && !it.value()["actor"].isNil() && !it.value()["enabled"].isNil()) {
                    // it's a component, so write a component reference
                    Actor* act = it.value()["actor"];
                    Component* c = act->getCompPointerByKey(it.value()["key"]);
                    writeChar(4);
                    writeString(c->type);
                    writeSizeT(act->uuid);
                    writeString(it.value()["key"]);
                    ++it;
                    continue;
                }
                writeChar(5);
                writeTable(it.value());
            } else if (it.value().isString()) {
                writeChar(3);
                writeString(it.value());
            } else if (it.value().isNumber()) {
                // test type of number
                it.value().push();
                if (lua_isinteger(luaState, -1)) {
                    // integral type, act as if it's a long
                    writeChar(0);
                    writeLong(it.value());
                } else {
                    // floating point number, act as if it's a float
                    writeChar(1);
                    writeFloat(it.value());
                }
                lua_pop(luaState, 1);
            } else if (it.value().isBool()) {
                writeChar(2);
                writeBool(it.value());
            } else if (it.value().isUserdata()) {
                // attempt to find what the type is
                if (it.value().isInstance<b2Vec2>() || it.value().isInstance<glm::vec2>()) {
                    auto vec = it.value();
                    // is a b2vec2, so print as 2 floats
                    writeChar(6);
                    writeFloat(vec["x"]);
                    writeFloat(vec["y"]);
                } else if (it.value().isInstance<RigidBody>()) {
                    writeChar(4);
                    writeString("RigidBody");
                    writeSizeT(it.value()["actor"]["GetID"](it.value()["actor"]));
                    writeString(it.value()["key"]);
                } else if (it.value().isInstance<ParticleSystem>()) {
                    writeChar(4);
                    writeString("ParticleSystem");
                    writeSizeT(it.value()["actor"]["GetID"]());
                    writeString(it.value()["key"]);
                } else if (it.value().isInstance<Actor>()) {
                    writeChar(4);
                    writeString("Actor");
                    writeSizeT(it.value().cast<Actor*>()->uuid);
                }
            }
            ++it;
        }
    }

    void writeVec2(b2Vec2 vec) {
        writeFloat(vec.x);
        writeFloat(vec.y);
    }

    void writeComponent(Component* component) {
        writeString(component->type);
        writeBool(component->initialized);
        component->serialize(*this);
    }

    void writeActor(const Actor* act) {
        writeSizeT(act->uuid);
        writeString(act->name);
        writeBool(act->dontDestroy);
        writeBool(act->serialize);
        writeSizeT(act->components.size());
        for (const auto&[fst, snd] : act->components) {
            writeString(fst);
            writeComponent(snd);
        }
    }

    void writeScene(const Scene& scene) {
        writeString(scene.name);
        writeFloat(scene.cameraPos.x);
        writeFloat(scene.cameraPos.y);
        writeSizeT(scene.actors.size());
        for (const Actor* act : scene.actors) {
            writeActor(act);
        }
    }

    void writeVec2(glm::vec2 vec) {
        writeFloat(vec.x);
        writeFloat(vec.y);
    }

    void writeVec3(glm::vec3 vec) {
        writeFloat(vec.x);
        writeFloat(vec.y);
        writeFloat(vec.z);
    }

    void writeGlobalTable() {
        lua_pushglobaltable(luaState);
        luabridge::LuaRef global = luabridge::LuaRef::fromStack(luaState);
        size_t count = 0;
        for (luabridge::Iterator it(global); !it.isNil(); ++it) {
            std::string key = it.key();
            if (excludeSet.find(it.key().tostring()) == excludeSet.end()) {
                count++;
            }
        }
        writeSizeT(count);
        for (luabridge::Iterator it(global); !it.isNil(); ++it) {
            if (it.value().isFunction()) {
                continue;
            }
            std::string s = it.key();
            if (excludeSet.find(s) != excludeSet.end()) continue;
            writeString(s);
            if (it.value().isTable()) {
                // first test if table is a component
                if (!it.value()["key"].isNil() && !it.value()["actor"].isNil() && !it.value()["enabled"].isNil()) {
                    // it's a component, so write a component reference
                    Actor* act = it.value()["actor"];
                    Component* c = act->getCompPointerByKey(it.value()["key"]);
                    writeChar(4);
                    writeString(c->type);
                    writeSizeT(act->uuid);
                    writeString(it.value()["key"]);
                    continue;
                }
                writeChar(5);
                writeTable(it.value());
            } else if (it.value().isString()) {
                writeChar(3);
                writeString(it.value());
            } else if (it.value().isNumber()) {
                // test type of number
                it.value().push();
                if (lua_isinteger(luaState, -1)) {
                    // integral type, act as if it's a long
                    writeChar(0);
                    writeLong(it.value());
                } else {
                    // floating point number, act as if it's a float
                    writeChar(1);
                    writeFloat(it.value());
                }
                lua_pop(luaState, 1);
            } else if (it.value().isBool()) {
                writeChar(2);
                writeBool(it.value());
            } else if (it.value().isUserdata()) {
                // attempt to find what the type is
                if (it.value().isInstance<b2Vec2>() || it.value().isInstance<glm::vec2>()) {
                    auto vec = it.value();
                    // is a b2vec2, so print as 2 floats
                    writeChar(6);
                    writeFloat(vec["x"]);
                    writeFloat(vec["y"]);
                } else if (it.value().isInstance<RigidBody>()) {
                    writeChar(4);
                    writeString("RigidBody");
                    writeSizeT(it.value()["actor"]["GetID"](it.value()["actor"]));
                    writeString(it.value()["key"]);
                } else if (it.value().isInstance<ParticleSystem>()) {
                    writeChar(4);
                    writeString("ParticleSystem");
                    writeSizeT(it.value()["actor"]["GetID"]());
                    writeString(it.value()["key"]);
                } else if (it.value().isInstance<Actor>()) {
                    writeChar(4);
                    writeString("Actor");
                    writeSizeT(it.value().cast<Actor*>()->uuid);
                }
            }
        }
    }

    explicit Serializer(const std::string& filename) : current_pos(0) {
        file.open(filename, std::ios_base::binary);
        if (!file.is_open()) {
            throw SerialError("Failed to open file" + filename);
        }
    }

    ~Serializer() {
        file.close();
    }
};

class Deserializer {
    std::ifstream file;


public:

    std::string readString() {
        std::ostringstream os;
        char buf[1];
        buf[0] = 1;
        while (true) {
            file.read(buf, 1);
            if (buf[0] == '\0') break;
            os << buf[0];
        }
        return os.str();
    }

    int readInt() {
        char buf[4];
        file.read(buf, 4);
        int out;
        memcpy(&out, buf, 4);
        return out;
    }

    bool readBool() {
        char buf[1];
        file.read(buf, 1);
        return buf[0] != 0;
    }

    char readChar() {
        char buf[1];
        file.read(buf, 1);
        return buf[0];
    }

    size_t readSizeT() {
        char buf[8];
        file.read(buf, 8);
        size_t l;
        memcpy(&l, buf, 8);
        return l;
    }

    long long readLong() {
        char buf[8];
        file.read(buf, 8);
        long long l;
        memcpy(&l, buf, 8);
        return l;
    }

    float readFloat() {
        char buf[4];
        file.read(buf, 4);
        float f;
        memcpy(&f, buf, 4);
        return f;
    }

    glm::vec2 readVec2() {
        float x = readFloat();
        float y = readFloat();
        return {x, y};
    }

    glm::vec3 readVec3() {
        float x = readFloat();
        float y = readFloat();
        float z = readFloat();
        return {x, y, z};
    }

    luabridge::LuaRef readTable(std::vector<Reference>& relocTable) {
        luabridge::LuaRef table = luabridge::newTable(luaState);
        size_t size = readSizeT();
        for (size_t i = 0; i < size; i++) {
            std::string key = readString();
            unsigned char byte = readChar();
            switch (byte) {
                case 0:
                    table[key] = readLong();
                    break;
                case 1:
                    table[key] = readFloat();
                    break;
                case 2:
                    table[key] = readBool();
                    break;
                case 3:
                    table[key] = readString();
                    break;
                case 4: {
                    std::string type = readString();
                    size_t actor_id = readSizeT();
                    if (type == "Actor") {
                        relocTable.emplace_back(table, "", actor_id, 0, key);
                    }
                    else {
                        relocTable.emplace_back(table, readString(), actor_id, 1, key);
                    }
                }
                break;
                case 5:
                    // recursively read table
                    table[key] = readTable(relocTable);
                    break;
                case 6:
                    table[key] = b2Vec2(readFloat(), readFloat());
                    break;
                default:
                    throw SerialError("Corrupted save file");
            }
        }
        return table;
    }

    Component* readComponent(std::vector<Reference>& relocTable, Actor* act) {
        std::string type = readString();
        bool initialized = readBool();
        Component* component;
        if (type == "Rigidbody") {
            component = new RigidBody(*this, act);
            component->first = dynamic_cast<RigidBody *>(component);
            component->type = type;
            component->initialized = initialized;
        }
        else if (type == "ParticleSystem") {
            component = new ParticleSystem(*this);
            component->first = dynamic_cast<ParticleSystem *>(component);
            component->type = type;
            component->initialized = initialized;
        }
        else {
            component = new Component();
            component->type = type;
            component->initialized = initialized;
            component->first = readTable(relocTable);
            luabridge::LuaRef tableBase = getBaseComponent(type);
            // copy functions from component
            luabridge::Iterator it(tableBase);
            while (!it.isNil()) {
                if (it.value().isFunction()) component->first[it.key()] = it.value();
                ++it;
            }
        }
        using luabridge::LuaRef;
        LuaRef start = component->first["OnStart"];
        if (start.isFunction()) component->onStart = [](LuaRef ref) {
            ref["OnStart"](ref);
        };
        LuaRef update = component->first["OnUpdate"];
        if (update.isFunction()) component->onUpdate = [](LuaRef ref) {
            ref["OnUpdate"](ref);
        };
        LuaRef lateupdate = component->first["OnLateUpdate"];
        if (lateupdate.isFunction()) component->onLateUpdate = [](LuaRef ref) {
            ref["OnLateUpdate"](ref);
        };
        LuaRef onDestroy = component->first["OnDestroy"];
        if (onDestroy.isFunction()) component->onDestroyed = [](LuaRef ref) {
            ref["OnDestroy"](ref);
        };
        LuaRef collisionEnter = component->first["OnCollisionEnter"];
        if (collisionEnter.isFunction()) component->onCollisionEnter = [](LuaRef ref, Collision& col) {
            ref["OnCollisionEnter"](ref, col);
        };
        LuaRef collisionExit = component->first["OnCollisionExit"];
        if (collisionExit.isFunction()) component->onCollisionExit = [](LuaRef ref, Collision& col) {
            ref["OnCollisionExit"](ref, col);
        };
        LuaRef triggerEnter = component->first["OnTriggerEnter"];
        if (triggerEnter.isFunction()) component->onTriggerEnter = [](LuaRef ref, Collision& col) {
            ref["OnTriggerEnter"](ref, col);
        };
        LuaRef triggerExit = component->first["OnTriggerExit"];
        if (triggerExit.isFunction()) component->onTriggerExit = [](LuaRef ref, Collision& col) {
            ref["OnTriggerExit"](ref, col);
        };
        return component;
    }

    Actor* readActor(std::vector<Reference>& relocTable) {
        auto* act = new Actor();
        act->uuid = readSizeT();
        act->name = readString();
        act->dontDestroy = readBool();
        act->serialize = readBool();
        size_t comps = readSizeT();
        for (int i = 0; i < comps; i++) {
            std::string key = readString();
            Component* comp = readComponent(relocTable, act);
            comp->first["key"] = key;
            comp->first["actor"] = act;
            act->components[key] = comp;
            act->componentsByKey[key] = comp;
            act->componentsByType[comp->type].push_back(comp);
        }
        return act;
    }

    Scene readScene(std::vector<Reference>& relocTable, std::unordered_map<std::string, Actor>&& templates) {
        std::string name = readString();
        float x = readFloat();
        float y = readFloat();
        Scene scene{};
        scene.cameraPos = {x, y};
        size_t size = readSizeT();
        scene.actors.reserve(size);
        scene.templates = templates;
        scene.name = name;
        for (int i = 0; i < size; ++i) {
            scene.actors.push_back(readActor(relocTable));
            scene.actorsByName[scene.actors.back()->name].push_back(scene.actors.back());
        }
        return scene;
    }

    b2Vec2 readb2Vec2() {
        float x = readFloat();
        float y = readFloat();
        return {x, y};
    }

    explicit Deserializer(const std::string &filename) {
        file.open(filename, std::ios_base::binary);
        if (!file.is_open()) {
            throw SerialError("Failed to open file");
        }
    }

    ~Deserializer() {
        file.close();
    }
};


#endif
