// The demo worlds, shared by envlib_strata_demo and the tests. Each is plain
// env-lib-cpp code: an Environment, entities kept at stable addresses (a
// std::list) and a tick that moves them through the public API. Nothing here
// knows about Strata except the style and floor-colour callbacks, which the
// frontend calls during sync() and which only read.
//
// env-lib-cpp draws its randomness from rand() (Grid::getRandomLocation,
// Environment::moveEntityToRandomAdjacentLocation), so the worlds do too:
// one srand(seed) makes a run repeatable.
//
// Copyright (c) 2026 Daniel McCoy Stephenson. MIT licence; see LICENSE.
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "environment.h"

#include "envlib_strata/style.h"

namespace demo {

struct World {
    virtual ~World() = default;
    virtual envlibcpp::Environment& environment() = 0;
    virtual void tick() = 0;
    virtual envlib_strata::StyleFn style() = 0;
    virtual envlib_strata::LocationColorFn locationColor() { return nullptr; }

    // A hash of every entity's id and location id, in id order: equal hashes
    // mean equal worlds.
    uint64_t stateHash() {
        std::vector<std::pair<int, std::string>> placed;
        for (auto& loc : environment().getGrid()->getLocations())
            for (envlibcpp::Entity* e : loc.getEntities()) placed.emplace_back(e->getId(), loc.getId());
        std::sort(placed.begin(), placed.end());
        uint64_t h = 1469598103934665603ull;
        auto mix = [&h](unsigned char c) {
            h ^= c;
            h *= 1099511628211ull;
        };
        for (const auto& p : placed) {
            for (int i = 0; i < 4; ++i) mix(static_cast<unsigned char>((p.first >> (8 * i)) & 0xFF));
            for (char c : p.second) mix(static_cast<unsigned char>(c));
            mix(0);
        }
        return h;
    }
};

// docs/demo.cpp in 3D: ten walkers on an 8x8 grid, each moved to a random
// adjacent location every tick. Seed 7 is the README GIF's run.
class Walkers : public World {
public:
    explicit Walkers(unsigned seed) : env_(0, "Earth", 8) {
        srand(seed);
        for (int i = 0; i < 10; i++) entities_.emplace_back(i, "walker");
        for (auto& e : entities_) env_.addEntity(e);
    }
    envlibcpp::Environment& environment() override { return env_; }
    void tick() override {
        for (auto& e : entities_) env_.moveEntityToRandomAdjacentLocation(e.getId());
    }
    envlib_strata::StyleFn style() override {
        return [](envlibcpp::Entity& e) {
            envlib_strata::Style s;
            // One hue per walker, so each can be followed.
            const auto rgb = envlib_strata::srgbToLinear(
                envlib_strata::hsvToRgb(static_cast<float>(e.getId()) / 10.0f, 0.65f, 0.95f));
            s.color[0] = rgb[0];
            s.color[1] = rgb[1];
            s.color[2] = rgb[2];
            s.scale[0] = s.scale[2] = 0.42f;
            s.scale[1] = 0.6f;
            s.primitive = STRATA_PRIMITIVE_CAPSULE;
            return s;
        };
    }

private:
    envlibcpp::Environment env_;
    std::list<envlibcpp::Entity> entities_;
};

// A meadow: sheep graze and wander, wolves stalk the nearest sheep, flowers
// stay put. Grass is the demo's own per-location state (the floor colour);
// a sheep eats the grass under it and the grass grows back. A wolf that
// reaches a sheep's location removes it, and a lamb (a new id) appears at a
// random location: entities come and go through the public API.
class Meadow : public World {
public:
    static constexpr int kSize = 20;

    explicit Meadow(unsigned seed) : env_(0, "Meadow", kSize) {
        srand(seed);
        for (int i = 0; i < 28; ++i) add("flower");
        for (int i = 0; i < 14; ++i) add("sheep");
        for (int i = 0; i < 3; ++i) add("wolf");
        for (auto& loc : env_.getGrid()->getLocations())
            grass_[loc.getId()] = 0.55f + 0.45f * static_cast<float>(rand() % 100) / 100.0f;
    }

    envlibcpp::Environment& environment() override { return env_; }

    void tick() override {
        ++ticks_;
        for (auto& [id, g] : grass_) g = std::min(1.0f, g + 0.012f);
        std::vector<int> sheep, wolves;
        for (auto& e : entities_) {
            if (e.getName() == "sheep") sheep.push_back(e.getId());
            else if (e.getName() == "wolf") wolves.push_back(e.getId());
        }
        for (int id : sheep) {
            // Graze where the grass is good, otherwise wander.
            envlibcpp::Entity& s = env_.getEntity(id);
            float& g = grass_[s.getLocationId()];
            if (g > 0.35f && rand() % 3 != 0) {
                g = std::max(0.0f, g - 0.3f);
            } else {
                env_.moveEntityToRandomAdjacentLocation(id);
            }
        }
        if (ticks_ % 2 == 0) {
            for (int id : wolves) stalk(id);
        }
    }

    envlib_strata::StyleFn style() override {
        return [](envlibcpp::Entity& e) {
            envlib_strata::Style s;
            const std::string name = e.getName();
            auto set = [&s](std::array<float, 3> srgb) {
                const auto rgb = envlib_strata::srgbToLinear(srgb);
                s.color[0] = rgb[0];
                s.color[1] = rgb[1];
                s.color[2] = rgb[2];
            };
            if (name == "sheep") {
                set({0.93f, 0.92f, 0.86f});
                s.scale[0] = 0.52f;
                s.scale[1] = 0.42f;
                s.scale[2] = 0.7f;
            } else if (name == "wolf") {
                set({0.22f, 0.22f, 0.27f});
                s.primitive = STRATA_PRIMITIVE_CAPSULE;
                s.scale[0] = s.scale[2] = 0.4f;
                s.scale[1] = 0.9f;
            } else {  // flower
                static const std::array<float, 3> petals[4] = {
                    {0.95f, 0.35f, 0.45f}, {0.98f, 0.82f, 0.25f}, {0.62f, 0.45f, 0.95f}, {0.98f, 0.98f, 0.98f}};
                set(petals[static_cast<unsigned>(e.getId()) % 4]);
                s.primitive = STRATA_PRIMITIVE_SPHERE;
                s.scale[0] = s.scale[1] = s.scale[2] = 0.28f;
                s.lift = 0.06f;
            }
            return s;
        };
    }

    envlib_strata::LocationColorFn locationColor() override {
        return [this](envlibcpp::Location& loc) {
            const auto it = grass_.find(loc.getId());
            const float g = it == grass_.end() ? 1.0f : it->second;
            const std::array<float, 3> dirt = {0.52f, 0.40f, 0.24f};
            const std::array<float, 3> grass = {0.36f, 0.66f, 0.26f};
            std::array<float, 3> srgb{};
            for (int i = 0; i < 3; ++i) srgb[i] = dirt[i] + (grass[i] - dirt[i]) * g;
            return envlib_strata::srgbToLinear(srgb);
        };
    }

private:
    void add(const std::string& name) {
        entities_.emplace_back(nextId_++, name);
        env_.addEntity(entities_.back());
    }

    static void cellOf(const std::string& locationId, int& x, int& y) {
        // "<gridId>-<x>-<y>" (README, "Grid Layout and Movement").
        const size_t a = locationId.find('-');
        const size_t b = locationId.find('-', a + 1);
        x = std::atoi(locationId.substr(a + 1, b - a - 1).c_str());
        y = std::atoi(locationId.substr(b + 1).c_str());
    }

    void stalk(int wolfId) {
        envlibcpp::Entity& wolf = env_.getEntity(wolfId);
        int wx = 0, wy = 0;
        cellOf(wolf.getLocationId(), wx, wy);
        envlibcpp::Entity* prey = nullptr;
        int best = 1 << 30, px = 0, py = 0;
        for (auto& e : entities_) {
            if (e.getName() != "sheep" || e.getLocationId() == "N/S") continue;
            int x = 0, y = 0;
            cellOf(e.getLocationId(), x, y);
            const int d = std::abs(x - wx) + std::abs(y - wy);
            if (d < best) {
                best = d;
                prey = &e;
                px = x;
                py = y;
            }
        }
        if (!prey) return;
        if (best == 0) {
            // Already there.
        } else if (std::abs(px - wx) >= std::abs(py - wy)) {
            if (px > wx) env_.moveEntityRight(wolfId);
            else if (px < wx) env_.moveEntityLeft(wolfId);
        } else {
            if (py > wy) env_.moveEntityDown(wolfId);
            else env_.moveEntityUp(wolfId);
        }
        if (wolf.getLocationId() == prey->getLocationId()) {
            const int preyId = prey->getId();
            env_.removeEntity(*prey);
            entities_.remove_if([preyId](envlibcpp::Entity& e) { return e.getId() == preyId; });
            add("sheep");  // a lamb, somewhere else
        }
    }

    envlibcpp::Environment env_;
    std::list<envlibcpp::Entity> entities_;
    std::unordered_map<std::string, float> grass_;
    int nextId_ = 0;
    long ticks_ = 0;
};

inline std::unique_ptr<World> makeWorld(const std::string& name, unsigned seed) {
    if (name == "walkers") return std::make_unique<Walkers>(seed);
    if (name == "meadow") return std::make_unique<Meadow>(seed);
    return nullptr;
}

}  // namespace demo
