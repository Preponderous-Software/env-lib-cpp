// envlib_strata_demo: env-lib-cpp worlds drawn in 3D with Strata.
//
//   envlib_strata_demo [--world walkers|meadow] [--seed N] [--tps N]
//                      [--yaw DEG] [--pitch DEG] [--zoom F] [--pick X,Y] [--no-hud]
//                      [--width W --height H]
//                      [--headless --ticks N [--screenshot out.png] [--frames DIR --frames-per-tick N]]
//                      [--state --ticks N]
//
// `walkers` is docs/demo.cpp (ten walkers on an 8x8 grid, seed 7 by
// default). `meadow` is a 20x20 meadow with grazing sheep, stalking wolves
// and flowers, and the grass as per-location floor colour.
//
// --state runs the world without any rendering and prints its state hash;
// the headless run prints the same hash after drawing, which is how CI checks
// that the frontend never changes the simulation.
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "envlib_strata/viewer_app.h"
#include "worlds.h"

namespace {

void usage(const char* argv0) {
    std::fprintf(stderr,
                 "usage: %s [--world walkers|meadow] [--seed N] [--tps N]\n"
                 "          [--yaw DEG] [--pitch DEG] [--zoom F] [--pick X,Y] [--no-hud] [--width W --height H]\n"
                 "          [--headless --ticks N [--screenshot out.png] [--frames DIR --frames-per-tick N]]\n"
                 "          [--state --ticks N]\n",
                 argv0);
}

}  // namespace

int main(int argc, char** argv) {
    std::string world = "walkers";
    long seed = -1;
    bool stateOnly = false;
    envlib_strata::ViewerOptions o;
    try {
        for (int i = 1; i < argc; ++i) {
            const std::string k = argv[i];
            auto val = [&]() -> std::string {
                if (i + 1 >= argc) throw std::invalid_argument(k + " needs a value");
                return argv[++i];
            };
            if (k == "--world") world = val();
            else if (k == "--seed") seed = std::atol(val().c_str());
            else if (k == "--tps") o.ticksPerSecond = std::atof(val().c_str());
            else if (k == "--yaw") o.yawDegrees = static_cast<float>(std::atof(val().c_str()));
            else if (k == "--pitch") o.pitchDegrees = static_cast<float>(std::atof(val().c_str()));
            else if (k == "--zoom") o.zoom = static_cast<float>(std::atof(val().c_str()));
            else if (k == "--pick") {
                const std::string v = val();
                const size_t comma = v.find(',');
                if (comma == std::string::npos) throw std::invalid_argument("--pick takes X,Y");
                o.pickX = std::atoi(v.substr(0, comma).c_str());
                o.pickY = std::atoi(v.substr(comma + 1).c_str());
            } else if (k == "--no-hud") o.hud = false;
            else if (k == "--width") o.width = std::atoi(val().c_str());
            else if (k == "--height") o.height = std::atoi(val().c_str());
            else if (k == "--headless") o.headless = true;
            else if (k == "--ticks") o.ticks = std::atol(val().c_str());
            else if (k == "--screenshot") o.screenshot = val();
            else if (k == "--frames") o.framesDir = val();
            else if (k == "--frames-per-tick") o.framesPerTick = std::atoi(val().c_str());
            else if (k == "--state") stateOnly = true;
            else {
                usage(argv[0]);
                return 2;
            }
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        usage(argv[0]);
        return 2;
    }
    const unsigned s = static_cast<unsigned>(seed >= 0 ? seed : (world == "walkers" ? 7 : 1));
    auto w = demo::makeWorld(world, s);
    if (!w) {
        usage(argv[0]);
        return 2;
    }
    if (stateOnly) {
        for (long t = 0; t < o.ticks; ++t) w->tick();
        std::printf("state %016llx\n", static_cast<unsigned long long>(w->stateHash()));
        return 0;
    }
    o.title = "env-lib-cpp on Strata: " + world;
    envlib_strata::ViewerApp app(w->environment(), [&w] { w->tick(); }, o, w->style(), w->locationColor());
    const int code = app.run();
    if (o.headless) std::printf("state %016llx\n", static_cast<unsigned long long>(w->stateHash()));
    return code;
}
