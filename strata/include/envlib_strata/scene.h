// Scene: an EnvironmentView turned into Strata draw calls.
//
// The grid is one static floor mesh (one quad per location, rebuilt only when
// the grid size or a floor colour changes). Each entity is one instanced
// primitive per frame, tweened from its previous cell to its current one and
// fanned out inside the cell by a fixed offset from a hash of its id, so it
// keeps its place in a shared cell from frame to frame.
//
// Axes: Strata is Y-up. env-lib x -> +X, env-lib y -> +Z. env-lib's y grows
// downward (README, "Grid Layout and Movement"), so row 0 is the far edge
// seen from the default camera. The grid is centred on the origin.
//
// Copyright (c) 2026 Daniel McCoy Stephenson. MIT licence; see LICENSE.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "strata/strata.h"

#include "envlib_strata/environment_view.h"

namespace strata {
class Engine;
}

namespace envlib_strata {

struct OrbitCamera {
    float targetX = 0, targetZ = 0;  // the floor point looked at
    float distance = 20;
    float yawDegrees = 0;            // 0: looking from +Z towards -Z
    float pitchDegrees = 55;         // above the horizon
    float fovYDegrees = 40;
    StrataCamera toStrata(float aspect) const;
    void pan(float dx, float dz);    // along the camera's own right / forward axes
    void zoom(float factor);
};

// A camera that frames a size x size grid.
OrbitCamera fitCamera(int gridSize, float aspect);

// The world-space centre of cell (x, y) on a size x size grid.
std::array<float, 3> cellCenter(int gridSize, int x, int y);
// The fan-out offset (x, z) of an entity inside its cell, each in
// [-maxOffset, maxOffset], from a hash of its id.
std::array<float, 2> entityOffset(int id, float maxOffset = 0.2f);
// A world point to drawable pixels; false when it is behind the camera.
bool projectToScreen(const StrataCamera& camera, int width, int height, const float world[3], float& px,
                     float& py);

struct SceneOptions {
    float floorColorA[3] = {0.20f, 0.24f, 0.20f};  // checkerboard, linear RGB
    float floorColorB[3] = {0.16f, 0.19f, 0.16f};
    float floorThickness = 0.0f;                   // the floor's top is at y = 0
    float maxOffset = 0.2f;                        // fan-out inside a cell
    bool gridLines = true;
    float gridLineColor[4] = {0.05f, 0.06f, 0.05f, 1.0f};
};

class Scene {
public:
    explicit Scene(SceneOptions options = {});

    // (Re)builds the floor mesh when the view's grid size or floor colours
    // changed since the last call. Call once per sync, outside a frame.
    void updateFloor(strata::Engine& engine, const EnvironmentView& view);
    // Removes the floor mesh (before the engine goes away, or to reset).
    void releaseFloor(strata::Engine& engine);

    // One item per visible entity, positions tweened by t in [0, 1] from the
    // previous sync's cell to the current one. A move of more than one cell
    // (a teleport or a random placement) snaps instead of sliding.
    const std::vector<StrataRenderItem>& buildItems(const EnvironmentView& view, float t);
    // Thin lines between cells, on top of the floor.
    const std::vector<StrataLineVertex>& buildGridLines(const EnvironmentView& view);
    // A highlight around one cell (a picked one).
    const std::vector<StrataLineVertex>& buildCellOutline(const EnvironmentView& view, int x, int y,
                                                          const float color[4]);

    // The cell under drawable pixel (px, py), by Strata's mesh raycast (ABI
    // 1.8) against the floor. Strata does not test primitives, so this picks a
    // location, not an entity; list its entities with view.entitiesAt().
    bool pick(const strata::Engine& engine, const StrataCamera& camera, int width, int height, float px, float py,
              int& x, int& y) const;

    uint32_t floorMeshId() const { return floorMesh_; }

private:
    SceneOptions options_;
    uint32_t floorMesh_ = 0;
    int floorSize_ = -1;
    uint64_t floorRevision_ = UINT64_MAX;
    std::vector<StrataRenderItem> items_;
    std::vector<StrataLineVertex> lines_;
    std::vector<StrataLineVertex> outline_;
};

}  // namespace envlib_strata
