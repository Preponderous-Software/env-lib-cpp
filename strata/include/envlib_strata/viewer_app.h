// ViewerApp: a ready main loop for a program that wants no Strata code of
// its own. It opens a window (or a headless EGL surface), ticks the
// simulation at a fixed rate with strata::sim::FixedStep, syncs the view
// after each tick and draws every frame with tweening between ticks.
//
// Window controls: drag or WASD/arrows pan, Q/E rotate, wheel or +/- zoom,
// click picks a location (its entities are listed), space pauses, N steps one
// tick while paused, ,/. slower/faster, Tab toggles the HUD, Esc quits.
//
// Headless (--headless in the demo): runs `ticks` ticks, can write
// `framesPerTick` tweened frames per tick to a directory (frame-00000.png,
// ...) and a final screenshot. That is how docs/media/ was recorded.
//
// Copyright (c) 2026 Daniel McCoy Stephenson. MIT licence; see LICENSE.
#pragma once

#include <functional>
#include <string>

#include "environment.h"

#include "envlib_strata/environment_view.h"
#include "envlib_strata/scene.h"

namespace envlib_strata {

struct ViewerOptions {
    std::string title = "env-lib-cpp on Strata";
    int width = 1280;
    int height = 720;
    bool headless = false;
    double ticksPerSecond = 2.0;  // window: simulation ticks per second
    long ticks = 20;              // headless: ticks to run
    std::string framesDir;        // headless: write frames here (empty: none)
    int framesPerTick = 1;        // headless: tweened frames written per tick
    std::string screenshot;       // headless: final screenshot (empty: none)
    bool hud = true;
    float yawDegrees = 0.0f;
    float pitchDegrees = 55.0f;
    float zoom = 1.0f;            // > 1 moves the camera closer
    int pickX = -1, pickY = -1;   // a location to show as picked at start
    float clearColor[3] = {0.035f, 0.045f, 0.06f};
    SceneOptions scene;
};

// One simulation tick; the frontend never calls it except through here.
using TickFn = std::function<void()>;

class ViewerApp {
public:
    ViewerApp(envlibcpp::Environment& env, TickFn tick, ViewerOptions options = {}, StyleFn style = defaultStyle,
              LocationColorFn locationColor = nullptr);
    // Runs until the window closes, or the headless ticks are done. Returns a
    // process exit code; prints errors to stderr.
    int run();

    const EnvironmentView& view() const { return view_; }

private:
    envlibcpp::Environment& env_;
    TickFn tick_;
    ViewerOptions options_;
    EnvironmentView view_;
};

}  // namespace envlib_strata
