// EnvironmentView: a read-only, flat copy of an env-lib-cpp Environment.
//
// sync() reads the environment through its public, non-mutating calls
// (getGrid, getLocations, Location::getEntities, Entity::getId/getName) and
// copies what drawing needs: each entity's id, cell and Style, and the
// optional floor colours. It keeps no Entity*, Location& or vector reference
// past the call, because env-lib-cpp does not own its entities and
// getLocations() may move (README, "Ownership and Lifetime").
//
// sync() must run between ticks, on the thread that runs the simulation:
// env-lib-cpp has no locking, so the consumer decides when the world is in a
// consistent state. It never calls rand() and never changes the environment,
// so a run gives the same world with and without the frontend.
//
// Copyright (c) 2026 Daniel McCoy Stephenson. MIT licence; see LICENSE.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "environment.h"

#include "envlib_strata/style.h"

namespace envlib_strata {

struct EntitySnapshot {
    int id = -1;
    std::string name;
    int x = 0, y = 0;          // this sync's cell
    int prevX = 0, prevY = 0;  // the cell at the previous sync (== x, y for a new entity)
    bool isNew = true;         // first seen at this sync
    Style style;
};

class EnvironmentView {
public:
    explicit EnvironmentView(StyleFn style = defaultStyle, LocationColorFn locationColor = nullptr);

    // Copies the environment's current state. Read-only.
    void sync(envlibcpp::Environment& env);

    // The number of sync() calls so far.
    uint64_t syncs() const { return syncs_; }
    // The grid's side length at the last sync (env-lib-cpp grids are square).
    int gridSize() const { return gridSize_; }
    const std::string& environmentName() const { return environmentName_; }
    // In the order the grid's locations were read (row by row).
    const std::vector<EntitySnapshot>& entities() const { return entities_; }
    // Per-location floor colours, row-major (y * size + x), linear RGB; empty
    // without a LocationColorFn.
    const std::vector<std::array<float, 3>>& locationColors() const { return locationColors_; }
    // Bumped whenever the grid size or a floor colour changed in a sync.
    uint64_t floorRevision() const { return floorRevision_; }
    // The entities in one cell (indices into entities()).
    std::vector<size_t> entitiesAt(int x, int y) const;
    // Problems seen in the last sync (an id in two places, a location outside
    // the grid). The view draws what it finds and does not guess.
    const std::vector<std::string>& warnings() const { return warnings_; }

private:
    StyleFn style_;
    LocationColorFn locationColor_;
    uint64_t syncs_ = 0;
    uint64_t floorRevision_ = 0;
    int gridSize_ = 0;
    std::string environmentName_;
    std::vector<EntitySnapshot> entities_;
    std::vector<std::array<float, 3>> locationColors_;
    std::unordered_map<int, std::array<int, 2>> lastCell_;
    std::vector<std::string> warnings_;
};

}  // namespace envlib_strata
