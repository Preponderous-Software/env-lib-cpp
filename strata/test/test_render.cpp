// Headless render tests through Strata's EGL surfaceless surface. They need a
// GL 4.5 driver at run time; CI uses Mesa llvmpipe
// (LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe).
//
// The golden image is the walkers world (docs/demo.cpp, seed 7) after 5
// ticks at 320x200, averaged down to 16x10 blocks and stored as text, so the
// comparison tolerates small rasteriser differences between Mesa versions
// but not a moved grid, a missing entity or a changed palette. Regenerate it
// with ENVLIB_STRATA_WRITE_GOLDEN=1 after a deliberate change to the look.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <vector>

#include "check.h"
#include "envlib_strata/environment_view.h"
#include "envlib_strata/scene.h"
#include "envlib_strata/viewer_app.h"
#include "strata/strata.hpp"
#include "worlds.h"

using namespace envlib_strata;

namespace {

constexpr int kW = 320, kH = 200, kBlocksX = 16, kBlocksY = 10;

std::unique_ptr<strata::Engine> headlessEngine() {
    strata::EngineDesc desc;
    desc.surface = STRATA_SURFACE_HEADLESS;
    desc.width = kW;
    desc.height = kH;
    return strata::Engine::create(desc);
}

void render(strata::Engine& engine, Scene& scene, const EnvironmentView& view, const StrataCamera& cam) {
    StrataLighting light = strata::lightingDefaults(STRATA_LIGHTING_SUN);
    light.time_of_day = 0.38f;
    light.latitude_degrees = 35.0f;
    light.fog = 0;
    light.cascade_count = 2;
    engine.setLighting(light);
    engine.setCamera(cam);
    scene.updateFloor(engine, view);
    StrataFrameDesc fd{};
    fd.clear_flags = STRATA_CLEAR_COLOR | STRATA_CLEAR_DEPTH;
    fd.clear_color[3] = 1.0f;
    engine.beginFrame(fd);
    const auto& items = scene.buildItems(view, 1.0f);
    engine.submitPrimitives(items.data(), items.size());
    const auto& lines = scene.buildGridLines(view);
    engine.submitLines(lines.data(), lines.size());
    engine.endFrame();
}

std::vector<int> blocks(const std::vector<uint8_t>& rgba, int w, int h) {
    std::vector<int> out(static_cast<size_t>(kBlocksX * kBlocksY * 3), 0);
    const int bw = w / kBlocksX, bh = h / kBlocksY;
    for (int by = 0; by < kBlocksY; ++by)
        for (int bx = 0; bx < kBlocksX; ++bx)
            for (int c = 0; c < 3; ++c) {
                long sum = 0;
                for (int y = by * bh; y < (by + 1) * bh; ++y)
                    for (int x = bx * bw; x < (bx + 1) * bw; ++x) sum += rgba[(static_cast<size_t>(y) * w + x) * 4 + c];
                out[static_cast<size_t>((by * kBlocksX + bx) * 3 + c)] = static_cast<int>(sum / (bw * bh));
            }
    return out;
}

}  // namespace

TEST("the walkers world renders headlessly and matches the golden image", "[render]") {
    auto world = demo::makeWorld("walkers", 7);
    EnvironmentView view(world->style());
    for (int t = 0; t < 5; ++t) world->tick();
    view.sync(world->environment());

    auto engine = headlessEngine();
    Scene scene;
    const float aspect = static_cast<float>(kW) / kH;
    const StrataCamera cam = fitCamera(view.gridSize(), aspect).toStrata(aspect);
    render(*engine, scene, view, cam);
    const StrataFrameStats& stats = engine->frameStats();
    CHECK(stats.items_rejected == 0);
    CHECK(stats.instances == static_cast<int32_t>(view.entities().size() + 1));  // + the board
    CHECK(view.entities().size() == 10);

    std::vector<uint8_t> rgba;
    int32_t w = 0, h = 0;
    engine->readPixels(rgba, w, h);
    REQUIRE(w == kW && h == kH);
    size_t lit = 0;
    for (size_t i = 0; i < rgba.size(); i += 4)
        if (rgba[i] + rgba[i + 1] + rgba[i + 2] > 30) ++lit;
    CHECK(lit > rgba.size() / 4 / 4);  // the grid fills a good part of the frame

    const std::vector<int> got = blocks(rgba, w, h);
    if (std::getenv("ENVLIB_STRATA_WRITE_GOLDEN")) {
        std::ofstream out(ENVLIB_STRATA_GOLDEN);
        out << "# 16x10 block means (RGB) of walkers seed 7 after 5 ticks, 320x200; see test_render.cpp\n";
        for (size_t i = 0; i < got.size(); ++i) out << got[i] << ((i + 1) % (kBlocksX * 3) == 0 ? "\n" : " ");
        std::printf("wrote %s\n", ENVLIB_STRATA_GOLDEN);
        engine->saveScreenshotPng("golden-walkers-seed7.png");
        return;
    }
    std::ifstream in(ENVLIB_STRATA_GOLDEN);
    REQUIRE(in.good());
    std::string header;
    std::getline(in, header);
    std::vector<int> want;
    int v = 0;
    while (in >> v) want.push_back(v);
    REQUIRE(want.size() == got.size());
    int worst = 0;
    double total = 0;
    for (size_t i = 0; i < got.size(); ++i) {
        const int d = std::abs(got[i] - want[i]);
        worst = std::max(worst, d);
        total += d;
    }
    const double mean = total / static_cast<double>(got.size());
    std::printf("  golden: mean block difference %.2f, worst %d (limits 3.0, 40)\n", mean, worst);
    if (mean > 3.0 || worst > 40) engine->saveScreenshotPng("golden-walkers-seed7-actual.png");
    CHECK(mean <= 3.0);
    CHECK(worst <= 40);
}

TEST("clicking a cell's centre picks that cell (mesh raycast against the floor)", "[render]") {
    demo::Walkers world(7);
    EnvironmentView view(world.style());
    view.sync(world.environment());
    auto engine = headlessEngine();
    Scene scene;
    const float aspect = static_cast<float>(kW) / kH;
    const StrataCamera cam = fitCamera(view.gridSize(), aspect).toStrata(aspect);
    render(*engine, scene, view, cam);
    for (const auto& cell : std::vector<std::array<int, 2>>{{0, 0}, {3, 5}, {7, 7}, {6, 1}}) {
        const auto c = cellCenter(view.gridSize(), cell[0], cell[1]);
        float px = 0, py = 0;
        REQUIRE(projectToScreen(cam, kW, kH, c.data(), px, py));
        int x = -1, y = -1;
        CHECK(scene.pick(*engine, cam, kW, kH, px, py, x, y));
        CHECK(x == cell[0]);
        CHECK(y == cell[1]);
    }
    int x = -1, y = -1;
    CHECK(!scene.pick(*engine, cam, kW, kH, 1.0f, 1.0f, x, y));  // the sky corner: no cell
}

TEST("a changed floor colour replaces the floor mesh; an unchanged one keeps it", "[render]") {
    demo::Meadow world(1);
    EnvironmentView view(world.style(), world.locationColor());
    view.sync(world.environment());
    auto engine = headlessEngine();
    Scene scene;
    scene.updateFloor(*engine, view);
    const uint32_t first = scene.floorMeshId();
    CHECK(first != 0);
    scene.updateFloor(*engine, view);
    CHECK(scene.floorMeshId() == first);
    world.tick();  // grass grows and is grazed
    view.sync(world.environment());
    scene.updateFloor(*engine, view);
    CHECK(scene.floorMeshId() != first);
    bool threw = false;
    try {
        engine->removeMesh(first);  // already removed by the rebuild
    } catch (const strata::Error&) {
        threw = true;
    }
    CHECK(threw);
    scene.releaseFloor(*engine);
}

TEST("ViewerApp a frame at a time: start, setWorld to another grid, pickAt, status", "[render]") {
    // What the browser page does, on a headless surface: one engine, two worlds.
    demo::Walkers walkers(7);
    ViewerOptions o;
    o.headless = true;
    o.width = kW;
    o.height = kH;
    ViewerApp app(walkers.environment(), [&walkers] { walkers.tick(); }, o, walkers.style());
    CHECK(app.status().ticks == 0);
    REQUIRE(app.start());
    ViewerStatus s = app.status();
    CHECK(s.environment == "Earth");
    CHECK(s.gridSize == 8);
    CHECK(s.entities == 10);
    CHECK(s.pickX == -1);
    app.frame();  // headless has no window loop: a no-op, not a crash

    // The middle of the drawable, with the camera fitted to an 8x8 grid, is
    // a cell near the middle.
    CHECK(app.pickAt(kW * 0.5f, kH * 0.5f));
    s = app.status();
    CHECK(s.pickX >= 3 && s.pickX <= 4);
    CHECK(s.pickY >= 3 && s.pickY <= 4);
    CHECK(!app.pickAt(1.0f, 1.0f));  // the sky corner
    CHECK(app.status().pickX == -1);

    demo::Meadow meadow(1);
    app.setWorld(meadow.environment(), [&meadow] { meadow.tick(); }, meadow.style(), meadow.locationColor());
    s = app.status();
    CHECK(s.environment == "Meadow");
    CHECK(s.gridSize == 20);
    CHECK(s.ticks == 0);
    CHECK(app.view().locationColors().size() == 400u);
    CHECK(app.pickAt(kW * 0.5f, kH * 0.5f));
    s = app.status();
    CHECK(s.pickX >= 9 && s.pickX <= 10);

    app.key(44);  // space pauses
    CHECK(app.status().paused);
    app.rotate(30.0f);
    app.zoom(0.5f);
    app.home();
    app.stop();
    CHECK(!app.running());
}
