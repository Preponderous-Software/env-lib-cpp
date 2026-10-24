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
// A host that owns the loop (a browser page, where the loop is the
// browser's) calls start() once and frame() once per callback instead of
// run(); setWorld() swaps the environment on the same engine, and the
// control calls (key, zoom, rotate, home, pickAt) are what page buttons use.
//
// Copyright (c) 2026 Daniel McCoy Stephenson. MIT licence; see LICENSE.
#pragma once

#include <cstddef>
#include <functional>
#include <memory>
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
    bool keyHints = true;         // window: the HUD's line of key controls
    bool escapeQuits = true;      // window: Esc ends run() (a page has nothing to quit)
    float yawDegrees = 0.0f;
    float pitchDegrees = 55.0f;
    float zoom = 1.0f;            // > 1 moves the camera closer
    int pickX = -1, pickY = -1;   // a location to show as picked at start
    float clearColor[3] = {0.035f, 0.045f, 0.06f};
    SceneOptions scene;
};

// One simulation tick; the frontend never calls it except through here.
using TickFn = std::function<void()>;

// What the viewer shows, for a host's own UI.
struct ViewerStatus {
    std::string environment;
    int gridSize = 0;
    long ticks = 0;
    size_t entities = 0;
    bool paused = false;
    double speed = 1.0;           // the time scale (1: ticksPerSecond)
    bool hud = true;
    int pickX = -1, pickY = -1;   // -1: nothing picked
    size_t pickedEntities = 0;
};

class ViewerApp {
public:
    ViewerApp(envlibcpp::Environment& env, TickFn tick, ViewerOptions options = {}, StyleFn style = defaultStyle,
              LocationColorFn locationColor = nullptr);
    ~ViewerApp();
    ViewerApp(const ViewerApp&) = delete;
    ViewerApp& operator=(const ViewerApp&) = delete;

    // Runs until the window closes, or the headless ticks are done. Returns a
    // process exit code; prints errors to stderr.
    int run();

    // The same window loop, one iteration at a time. start() creates the
    // engine (false, with the error on stderr, if it cannot); frame() handles
    // the pending events, runs the ticks that are due, draws and presents;
    // running() is false once the window was closed (or Esc pressed, when it
    // quits); stop() releases the engine. frame() reports exceptions to
    // stderr and keeps going.
    bool start();
    void frame();
    bool running() const;
    void stop();

    // Another environment on the same engine: the view starts over (tick 0,
    // nothing picked, the camera fitted to the new grid). `env` and `tick`
    // must outlive this ViewerApp or the next setWorld().
    void setWorld(envlibcpp::Environment& env, TickFn tick, StyleFn style = defaultStyle,
                  LocationColorFn locationColor = nullptr);

    // Controls, after start(). key() takes an SDL scancode, as the keyboard
    // sends; zoom() multiplies the camera distance (< 1 is closer); rotate()
    // turns the camera about the grid; home() frames the grid again.
    void key(int scancode);
    void zoom(float factor);
    void rotate(float degrees);
    void home();
    // Picks the location under a drawable pixel; false (and nothing picked)
    // when there is none.
    bool pickAt(float px, float py);
    // The drawable changed size (a browser page resized its canvas).
    void resize(int width, int height);
    // Pointer events arrive in window units; a denser drawable (a browser's
    // device pixel ratio) multiplies them by this before they are used.
    void setPointerScale(float sx, float sy);

    ViewerStatus status() const;
    const EnvironmentView& view() const { return view_; }

    struct Runner;

private:
    envlibcpp::Environment* env_;
    TickFn tick_;
    ViewerOptions options_;
    EnvironmentView view_;
    std::unique_ptr<Runner> runner_;
};

}  // namespace envlib_strata
