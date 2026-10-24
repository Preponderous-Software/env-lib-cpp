#include "envlib_strata/scene.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "strata/strata.hpp"

namespace envlib_strata {

namespace {

constexpr float kPi = 3.14159265358979f;

uint64_t splitmix64(uint64_t x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

}  // namespace

StrataCamera OrbitCamera::toStrata(float aspect) const {
    const float yaw = yawDegrees * kPi / 180.0f;
    const float pitch = pitchDegrees * kPi / 180.0f;
    const float eye[3] = {targetX + distance * std::cos(pitch) * std::sin(yaw), distance * std::sin(pitch),
                          targetZ + distance * std::cos(pitch) * std::cos(yaw)};
    const float target[3] = {targetX, 0.0f, targetZ};
    const float up[3] = {0.0f, 1.0f, 0.0f};
    return strata::cameraLookAt(eye, target, up, fovYDegrees, aspect, std::max(0.05f, distance * 0.02f),
                                distance * 6.0f);
}

void OrbitCamera::pan(float dx, float dz) {
    const float yaw = yawDegrees * kPi / 180.0f;
    targetX += dx * std::cos(yaw) - dz * std::sin(yaw);
    targetZ += -dx * std::sin(yaw) - dz * std::cos(yaw);
}

void OrbitCamera::zoom(float factor) { distance = std::min(std::max(distance * factor, 1.5f), 4000.0f); }

OrbitCamera fitCamera(int gridSize, float aspect) {
    OrbitCamera cam;
    const float extent = static_cast<float>(std::max(gridSize, 1));
    // Fit the grid's width (scaled by the aspect) and depth into the view.
    const float fit = std::max(extent / std::max(aspect, 0.1f), extent);
    cam.distance = fit * 1.55f + 2.0f;
    cam.targetZ = extent * 0.04f;
    return cam;
}

std::array<float, 3> cellCenter(int gridSize, int x, int y) {
    const float half = static_cast<float>(gridSize) * 0.5f;
    return {static_cast<float>(x) + 0.5f - half, 0.0f, static_cast<float>(y) + 0.5f - half};
}

std::array<float, 2> entityOffset(int id, float maxOffset) {
    const uint64_t h = splitmix64(static_cast<uint64_t>(static_cast<uint32_t>(id)));
    const float a = static_cast<float>(h & 0xFFFFFFFFull) / 4294967295.0f;
    const float b = static_cast<float>(h >> 32) / 4294967295.0f;
    return {(a * 2.0f - 1.0f) * maxOffset, (b * 2.0f - 1.0f) * maxOffset};
}

bool projectToScreen(const StrataCamera& c, int width, int height, const float p[3], float& px, float& py) {
    float v[4];
    for (int r = 0; r < 4; ++r)
        v[r] = c.view[0 * 4 + r] * p[0] + c.view[1 * 4 + r] * p[1] + c.view[2 * 4 + r] * p[2] + c.view[3 * 4 + r];
    float clip[4];
    for (int r = 0; r < 4; ++r)
        clip[r] = c.projection[0 * 4 + r] * v[0] + c.projection[1 * 4 + r] * v[1] + c.projection[2 * 4 + r] * v[2] +
                  c.projection[3 * 4 + r] * v[3];
    if (clip[3] <= 1e-6f) return false;
    const float nx = clip[0] / clip[3];
    const float ny = clip[1] / clip[3];
    px = (nx * 0.5f + 0.5f) * static_cast<float>(width);
    py = (0.5f - ny * 0.5f) * static_cast<float>(height);
    return true;
}

Scene::Scene(SceneOptions options) : options_(options) {}

void Scene::updateFloor(strata::Engine& engine, const EnvironmentView& view) {
    if (view.floorRevision() == floorRevision_ && floorMesh_ != 0) return;
    floorRevision_ = view.floorRevision();
    releaseFloor(engine);
    const int size = view.gridSize();
    floorSize_ = size;
    if (size <= 0) return;

    const size_t cells = static_cast<size_t>(size) * static_cast<size_t>(size);
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> colors;
    std::vector<uint32_t> indices;
    positions.reserve(cells * 12);
    normals.reserve(cells * 12);
    colors.reserve(cells * 12);
    indices.reserve(cells * 6);
    const auto& custom = view.locationColors();
    const float y = options_.floorThickness;
    for (int cy = 0; cy < size; ++cy) {
        for (int cx = 0; cx < size; ++cx) {
            const auto c = cellCenter(size, cx, cy);
            const float x0 = c[0] - 0.5f, x1 = c[0] + 0.5f, z0 = c[2] - 0.5f, z1 = c[2] + 0.5f;
            const float corners[4][3] = {{x0, y, z0}, {x0, y, z1}, {x1, y, z1}, {x1, y, z0}};
            const float* rgb = ((cx + cy) % 2 == 0) ? options_.floorColorA : options_.floorColorB;
            float cell[3] = {rgb[0], rgb[1], rgb[2]};
            if (!custom.empty()) {
                const auto& lc = custom[static_cast<size_t>(cy) * size + cx];
                // Keep a faint checkerboard over custom colours so cells stay legible.
                const float k = ((cx + cy) % 2 == 0) ? 1.0f : 0.88f;
                cell[0] = lc[0] * k;
                cell[1] = lc[1] * k;
                cell[2] = lc[2] * k;
            }
            const uint32_t base = static_cast<uint32_t>(positions.size() / 3);
            for (const auto& corner : corners) {
                positions.insert(positions.end(), corner, corner + 3);
                normals.insert(normals.end(), {0.0f, 1.0f, 0.0f});
                colors.insert(colors.end(), cell, cell + 3);
            }
            // Counter-clockwise seen from +Y. Triangles 2i and 2i+1 are cell i
            // (row-major), which pick() relies on.
            indices.insert(indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
        }
    }
    StrataMesh mesh{};
    mesh.positions = positions.data();
    mesh.normals = normals.data();
    mesh.colors = colors.data();
    mesh.indices = indices.data();
    mesh.vertex_count = static_cast<int32_t>(positions.size() / 3);
    mesh.index_count = static_cast<int32_t>(indices.size());
    mesh.color[0] = mesh.color[1] = mesh.color[2] = mesh.color[3] = 1.0f;
    floorMesh_ = engine.addMesh(mesh);
}

void Scene::releaseFloor(strata::Engine& engine) {
    if (floorMesh_ != 0) engine.removeMesh(floorMesh_);
    floorMesh_ = 0;
}

const std::vector<StrataRenderItem>& Scene::buildItems(const EnvironmentView& view, float t) {
    items_.clear();
    const int size = view.gridSize();
    t = std::min(std::max(t, 0.0f), 1.0f);
    {
        // The board under the floor: a slab, so the grid reads as an object.
        StrataRenderItem slab{};
        slab.kind = STRATA_PRIMITIVE_CUBE;
        const float side = static_cast<float>(size) + 0.3f;
        slab.position[1] = -0.26f;
        slab.scale[0] = side;
        slab.scale[1] = 0.5f;
        slab.scale[2] = side;
        slab.color[0] = 0.09f;
        slab.color[1] = 0.075f;
        slab.color[2] = 0.06f;
        slab.color[3] = 1.0f;
        if (size > 0) items_.push_back(slab);
    }
    for (const EntitySnapshot& e : view.entities()) {
        if (!e.style.visible) continue;
        const bool slide = std::abs(e.x - e.prevX) + std::abs(e.y - e.prevY) <= 1;
        const float k = slide ? t : 1.0f;
        const auto from = cellCenter(size, e.prevX, e.prevY);
        const auto to = cellCenter(size, e.x, e.y);
        const auto off = entityOffset(e.id, options_.maxOffset);
        StrataRenderItem it{};
        it.kind = e.style.primitive;
        it.position[0] = from[0] + (to[0] - from[0]) * k + off[0];
        it.position[2] = from[2] + (to[2] - from[2]) * k + off[1];
        it.position[1] = options_.floorThickness + e.style.scale[1] * 0.5f + e.style.lift;
        for (int i = 0; i < 3; ++i) it.scale[i] = e.style.scale[i];
        for (int i = 0; i < 4; ++i) it.color[i] = e.style.color[i];
        items_.push_back(it);
    }
    return items_;
}

const std::vector<StrataLineVertex>& Scene::buildGridLines(const EnvironmentView& view) {
    lines_.clear();
    const int size = view.gridSize();
    if (!options_.gridLines || size <= 0) return lines_;
    const float half = static_cast<float>(size) * 0.5f;
    const float y = options_.floorThickness + 0.004f;
    for (int i = 0; i <= size; ++i) {
        const float u = static_cast<float>(i) - half;
        StrataLineVertex a{}, b{};
        std::copy(options_.gridLineColor, options_.gridLineColor + 4, a.color);
        std::copy(options_.gridLineColor, options_.gridLineColor + 4, b.color);
        a.position[0] = u; a.position[1] = y; a.position[2] = -half;
        b.position[0] = u; b.position[1] = y; b.position[2] = half;
        lines_.push_back(a);
        lines_.push_back(b);
        a.position[0] = -half; a.position[2] = u;
        b.position[0] = half;  b.position[2] = u;
        lines_.push_back(a);
        lines_.push_back(b);
    }
    return lines_;
}

const std::vector<StrataLineVertex>& Scene::buildCellOutline(const EnvironmentView& view, int x, int y,
                                                             const float color[4]) {
    outline_.clear();
    const int size = view.gridSize();
    if (x < 0 || y < 0 || x >= size || y >= size) return outline_;
    const auto c = cellCenter(size, x, y);
    const float h = options_.floorThickness + 0.01f;
    for (float inset : {0.47f, 0.44f, 0.41f}) {
        const float pts[4][3] = {{c[0] - inset, h, c[2] - inset},
                                 {c[0] + inset, h, c[2] - inset},
                                 {c[0] + inset, h, c[2] + inset},
                                 {c[0] - inset, h, c[2] + inset}};
        for (int i = 0; i < 4; ++i) {
            StrataLineVertex a{}, b{};
            std::copy(pts[i], pts[i] + 3, a.position);
            std::copy(pts[(i + 1) % 4], pts[(i + 1) % 4] + 3, b.position);
            std::copy(color, color + 4, a.color);
            std::copy(color, color + 4, b.color);
            outline_.push_back(a);
            outline_.push_back(b);
        }
    }
    return outline_;
}

bool Scene::pick(const strata::Engine& engine, const StrataCamera& c, int width, int height, float px, float py,
                 int& x, int& y) const {
    if (floorMesh_ == 0 || floorSize_ <= 0) return false;
    // The view matrix's rotation rows are the camera's right, up and backward axes.
    float right[3], up[3], forward[3];
    for (int i = 0; i < 3; ++i) {
        right[i] = c.view[i * 4 + 0];
        up[i] = c.view[i * 4 + 1];
        forward[i] = -c.view[i * 4 + 2];
    }
    const float nx = 2.0f * px / static_cast<float>(std::max(1, width)) - 1.0f;
    const float ny = 1.0f - 2.0f * py / static_cast<float>(std::max(1, height));
    const float sx = nx / std::max(std::fabs(c.projection[0]), 1e-6f);
    const float sy = ny / std::max(std::fabs(c.projection[5]), 1e-6f);
    float dir[3];
    for (int i = 0; i < 3; ++i) dir[i] = forward[i] + right[i] * sx + up[i] * sy;
    const StrataRayHit hit = engine.raycastMeshes(c.eye, dir, c.far_plane * 2.0f);
    if (hit.mesh_id != floorMesh_ || hit.triangle < 0) return false;
    const int cell = hit.triangle / 2;
    x = cell % floorSize_;
    y = cell / floorSize_;
    return true;
}

}  // namespace envlib_strata
