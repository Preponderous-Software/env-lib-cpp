// EnvironmentView and the scene maths: no GL needed.
#include <cmath>
#include <list>
#include <memory>

#include "check.h"
#include "envlib_strata/environment_view.h"
#include "envlib_strata/scene.h"
#include "worlds.h"

using envlibcpp::Entity;
using envlibcpp::Environment;
using namespace envlib_strata;

namespace {

bool near(float a, float b, float eps = 1e-5f) { return std::fabs(a - b) <= eps; }

}  // namespace

TEST("sync copies each entity's id, name and cell", "[view]") {
    Environment env(3, "Box", 4);
    Entity a(11, "alpha");
    env.addEntityToLocation(a, env.getGrid()->getLocationByCoordinates(1, 2));
    EnvironmentView view;
    view.sync(env);
    REQUIRE(view.entities().size() == 1);
    const EntitySnapshot& s = view.entities()[0];
    CHECK(s.id == 11);
    CHECK(s.name == "alpha");
    CHECK(s.x == 1);
    CHECK(s.y == 2);
    CHECK(s.isNew);
    CHECK(s.prevX == 1 && s.prevY == 2);
    CHECK(view.gridSize() == 4);
    CHECK(view.environmentName() == "Box");
    CHECK(view.syncs() == 1);
    CHECK(view.warnings().empty());

    CHECK(env.moveEntityRight(11));
    view.sync(env);
    REQUIRE(view.entities().size() == 1);
    CHECK(!view.entities()[0].isNew);
    CHECK(view.entities()[0].prevX == 1 && view.entities()[0].prevY == 2);
    CHECK(view.entities()[0].x == 2 && view.entities()[0].y == 2);
    CHECK(view.entitiesAt(2, 2).size() == 1);
    CHECK(view.entitiesAt(1, 2).empty());
}

TEST("the view keeps no pointer: an entity freed after sync is still drawn from the copy", "[view]") {
    Environment env(0, "Box", 3);
    auto e = std::make_unique<Entity>(5, "temp");
    env.addEntityToLocation(*e, env.getGrid()->getLocationByCoordinates(0, 0));
    EnvironmentView view;
    view.sync(env);
    env.removeEntity(*e);
    e.reset();  // freed: a kept Entity* would now dangle
    REQUIRE(view.entities().size() == 1);
    CHECK(view.entities()[0].name == "temp");
    view.sync(env);
    CHECK(view.entities().empty());
}

TEST("an id in two places is drawn twice and reported", "[view]") {
    Environment env(0, "Box", 3);
    Entity a(1, "a"), b(1, "b");
    env.addEntityToLocation(a, env.getGrid()->getLocationByCoordinates(0, 0));
    env.addEntityToLocation(b, env.getGrid()->getLocationByCoordinates(2, 2));
    EnvironmentView view;
    view.sync(env);
    CHECK(view.entities().size() == 2);
    CHECK(view.warnings().size() == 1);
}

TEST("drawing never changes the simulation: same seed, same world, with and without the view", "[view]") {
    for (const char* name : {"walkers", "meadow"}) {
        auto plain = demo::makeWorld(name, 3);
        for (int t = 0; t < 60; ++t) plain->tick();
        const uint64_t expected = plain->stateHash();

        auto viewed = demo::makeWorld(name, 3);
        EnvironmentView view(viewed->style(), viewed->locationColor());
        Scene scene;
        view.sync(viewed->environment());
        for (int t = 0; t < 60; ++t) {
            viewed->tick();
            view.sync(viewed->environment());
            scene.buildItems(view, 0.5f);
            scene.buildGridLines(view);
        }
        CHECK(viewed->stateHash() == expected);
    }
}

TEST("styles: the default colour follows the name; the StyleFn is used", "[view]") {
    Entity a(1, "sheep"), b(2, "sheep"), c(3, "wolf");
    const Style sa = defaultStyle(a), sb = defaultStyle(b), sc = defaultStyle(c);
    CHECK(sa.color[0] == sb.color[0] && sa.color[1] == sb.color[1] && sa.color[2] == sb.color[2]);
    CHECK(!(sa.color[0] == sc.color[0] && sa.color[1] == sc.color[1] && sa.color[2] == sc.color[2]));

    Environment env(0, "Box", 2);
    env.addEntity(a);
    EnvironmentView view([](Entity&) {
        Style s;
        s.primitive = STRATA_PRIMITIVE_SPHERE;
        return s;
    });
    view.sync(env);
    REQUIRE(view.entities().size() == 1);
    CHECK(view.entities()[0].style.primitive == STRATA_PRIMITIVE_SPHERE);
    CHECK(near(srgbToLinear(1.0f), 1.0f));
    CHECK(near(srgbToLinear(0.0f), 0.0f));
    CHECK(srgbToLinear(0.5f) < 0.25f);
}

TEST("floor colours are row-major and bump the floor revision only when they change", "[view]") {
    Environment env(0, "Box", 3);
    float level = 0.5f;
    EnvironmentView view(defaultStyle, [&level](envlibcpp::Location& l) {
        return std::array<float, 3>{static_cast<float>(l.getX()), static_cast<float>(l.getY()), level};
    });
    view.sync(env);
    REQUIRE(view.locationColors().size() == 9);
    CHECK(view.locationColors()[1 * 3 + 2][0] == 2.0f);  // x = 2, y = 1
    CHECK(view.locationColors()[1 * 3 + 2][1] == 1.0f);
    const uint64_t r1 = view.floorRevision();
    view.sync(env);
    CHECK(view.floorRevision() == r1);
    level = 0.75f;
    view.sync(env);
    CHECK(view.floorRevision() == r1 + 1);
}

TEST("axes: env-lib x -> +X, y -> +Z, grid centred, row 0 at the far (-Z) edge", "[view]") {
    const auto c00 = cellCenter(8, 0, 0);
    const auto c71 = cellCenter(8, 7, 1);
    CHECK(near(c00[0], -3.5f) && near(c00[2], -3.5f) && near(c00[1], 0.0f));
    CHECK(near(c71[0], 3.5f) && near(c71[2], -2.5f));
}

TEST("fan-out offsets are stable per id and stay inside the cell", "[view]") {
    for (int id = -5; id < 500; ++id) {
        const auto o = entityOffset(id, 0.2f);
        CHECK(std::fabs(o[0]) <= 0.2f && std::fabs(o[1]) <= 0.2f);
        const auto again = entityOffset(id, 0.2f);
        CHECK(o[0] == again[0] && o[1] == again[1]);
    }
    CHECK(entityOffset(1)[0] != entityOffset(2)[0]);
}

TEST("tweening slides one-cell moves and snaps longer jumps", "[view]") {
    Environment env(0, "Box", 6);
    Entity a(1, "a"), b(2, "b");
    env.addEntityToLocation(a, env.getGrid()->getLocationByCoordinates(1, 1));
    env.addEntityToLocation(b, env.getGrid()->getLocationByCoordinates(0, 0));
    EnvironmentView view;
    view.sync(env);
    env.moveEntityRight(1);                    // one cell: slides
    env.moveEntityToNewLocation(2, "0-4-4");   // a jump: snaps
    view.sync(env);
    Scene scene;
    const auto at = [&](float t, int id) {
        const auto& items = scene.buildItems(view, t);
        // Item 0 is the board under the floor; then one per entity, in view order.
        for (size_t i = 0; i < view.entities().size(); ++i)
            if (view.entities()[i].id == id) return std::array<float, 2>{items[i + 1].position[0], items[i + 1].position[2]};
        return std::array<float, 2>{NAN, NAN};
    };
    const auto oa = entityOffset(1), ob = entityOffset(2);
    const auto a0 = at(0.0f, 1), aHalf = at(0.5f, 1), a1 = at(1.0f, 1);
    CHECK(near(a0[0], cellCenter(6, 1, 1)[0] + oa[0]));
    CHECK(near(aHalf[0], cellCenter(6, 1, 1)[0] + 0.5f + oa[0]));
    CHECK(near(a1[0], cellCenter(6, 2, 1)[0] + oa[0]));
    const auto b0 = at(0.0f, 2);
    CHECK(near(b0[0], cellCenter(6, 4, 4)[0] + ob[0]));
    CHECK(near(b0[1], cellCenter(6, 4, 4)[2] + ob[1]));
}

TEST("the camera's target projects to the middle of the screen", "[view]") {
    const OrbitCamera cam = fitCamera(8, 16.0f / 9.0f);
    const StrataCamera c = cam.toStrata(16.0f / 9.0f);
    const float target[3] = {cam.targetX, 0.0f, cam.targetZ};
    float px = 0, py = 0;
    REQUIRE(projectToScreen(c, 1600, 900, target, px, py));
    CHECK(std::fabs(px - 800.0f) < 0.5f);
    CHECK(std::fabs(py - 450.0f) < 0.5f);
}
