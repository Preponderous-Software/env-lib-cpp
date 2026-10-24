#include "envlib_strata/viewer_app.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "strata/sim/fixed_step.hpp"
#include "strata/strata.hpp"

namespace envlib_strata {

namespace {

// SDL3 scancodes (USB HID usage ids), as StrataEvent.scancode carries them.
enum : int32_t {
    kA = 4, kD = 7, kE = 8, kN = 17, kQ = 20, kS = 22, kW = 26,
    kEscape = 41, kTab = 43, kSpace = 44, kMinus = 45, kEquals = 46, kComma = 54, kPeriod = 55,
    kRight = 79, kLeft = 80, kDown = 81, kUp = 82, kKpMinus = 86, kKpPlus = 87
};

const float kWhite[4] = {0.95f, 0.95f, 0.92f, 1.0f};
const float kDim[4] = {0.55f, 0.58f, 0.62f, 1.0f};
const float kPick[4] = {1.0f, 0.78f, 0.15f, 1.0f};

// Strata's 8x8 font draws printable ASCII; anything else shows as '?'.
std::string printable(const std::string& s) {
    std::string out;
    for (unsigned char c : s) out += (c >= 32 && c < 127) ? static_cast<char>(c) : '?';
    return out;
}

}  // namespace

struct ViewerApp::Runner {
    envlibcpp::Environment* env;
    TickFn& tick;
    ViewerOptions& o;
    EnvironmentView& view;
    std::unique_ptr<strata::Engine> engine;
    Scene scene;
    OrbitCamera camera;
    StrataLighting light{};
    int w = 0, h = 0;
    long ticks = 0;
    bool paused = false, hud = true, running = true;
    int pickX = -1, pickY = -1;
    static constexpr double kSpeeds[6] = {0.25, 0.5, 1.0, 2.0, 4.0, 8.0};
    int speed = 2;

    // The window loop's state between frames.
    std::unique_ptr<strata::sim::FixedStep> clock;
    std::chrono::steady_clock::time_point last;
    bool dragging = false;
    float downX = 0, downY = 0, lastX = 0, lastY = 0;
    float pointerScaleX = 1, pointerScaleY = 1;

    // Texts must outlive submitText's call only, but keep them together.
    std::vector<std::string> strings;
    std::vector<StrataText> texts;

    Runner(envlibcpp::Environment& e, TickFn& t, ViewerOptions& opts, EnvironmentView& v)
        : env(&e), tick(t), o(opts), view(v), scene(opts.scene) {}

    float aspect() const { return static_cast<float>(w) / static_cast<float>(std::max(1, h)); }

    void step() {
        tick();
        ++ticks;
        view.sync(*env);
        for (const auto& warning : view.warnings()) std::fprintf(stderr, "envlib-strata: %s\n", warning.c_str());
        scene.updateFloor(*engine, view);
    }

    void text(float x, float y, float size, const float color[4], std::string s) {
        StrataText t{};
        t.x = x;
        t.y = y;
        t.pixel_size = size;
        t.flags = STRATA_TEXT_SHADOW;
        std::copy(color, color + 4, t.color);
        strings.push_back(std::move(s));
        texts.push_back(t);
    }

    void buildHud(const StrataCamera& cam) {
        strings.clear();
        texts.clear();
        if (!hud) return;
        const float s = h >= 1000 ? 24.0f : (h >= 600 ? 16.0f : 8.0f);
        const float lh = std::round(s * 1.35f);
        const float m = s;
        char line[256];
        std::snprintf(line, sizeof line, "%s  tick %ld  entities %zu  grid %dx%d", view.environmentName().c_str(),
                      ticks, view.entities().size(), view.gridSize(), view.gridSize());
        text(m, m, s, kWhite, printable(line));
        if (!o.headless) {
            std::snprintf(line, sizeof line, "%s  %.2gx", paused ? "paused" : "running", kSpeeds[speed]);
            text(m, m + lh, s, kDim, line);
            if (o.keyHints)
                text(m, static_cast<float>(h) - m - s, std::max(8.0f, s - 8.0f), kDim,
                     "drag pan  Q/E rotate  wheel zoom  click pick  space pause  N step  Tab HUD");
        }
        if (pickX >= 0) {
            const auto c = cellCenter(view.gridSize(), pickX, pickY);
            const float above[3] = {c[0], 0.9f, c[2]};
            float px = 0, py = 0;
            const auto here = view.entitiesAt(pickX, pickY);
            std::snprintf(line, sizeof line, "(%d, %d)  %zu here", pickX, pickY, here.size());
            if (projectToScreen(cam, w, h, above, px, py)) {
                const std::string label = line;
                text(std::round(px - static_cast<float>(label.size()) * s * 0.5f), std::round(py - s), s, kPick,
                     label);
            }
            float y = m + lh * 3;
            text(m, y, s, kPick, std::string("location ") + line);
            for (size_t i = 0; i < here.size() && i < 12; ++i) {
                const auto& e = view.entities()[here[i]];
                y += lh;
                std::snprintf(line, sizeof line, "  #%d %s", e.id, e.name.c_str());
                text(m, y, s, kWhite, printable(line));
            }
        }
        for (size_t i = 0; i < texts.size(); ++i) texts[i].utf8 = strings[i].c_str();
    }

    void draw(float t) {
        const StrataCamera cam = camera.toStrata(aspect());
        engine->setCamera(cam);
        light.shadow_distance = camera.distance * 3.0f;
        engine->setLighting(light);
        StrataFrameDesc fd{};
        fd.clear_flags = STRATA_CLEAR_COLOR | STRATA_CLEAR_DEPTH;
        fd.clear_color[0] = o.clearColor[0];
        fd.clear_color[1] = o.clearColor[1];
        fd.clear_color[2] = o.clearColor[2];
        fd.clear_color[3] = 1.0f;
        engine->beginFrame(fd);
        const auto& items = scene.buildItems(view, t);
        engine->submitPrimitives(items.data(), items.size());
        const auto& lines = scene.buildGridLines(view);
        if (!lines.empty()) engine->submitLines(lines.data(), lines.size());
        if (pickX >= 0) {
            const auto& outline = scene.buildCellOutline(view, pickX, pickY, kPick);
            if (!outline.empty()) engine->submitLines(outline.data(), outline.size());
        }
        buildHud(cam);
        if (!texts.empty()) engine->submitText(texts.data(), texts.size());
        engine->endFrame();
    }

    void key(const StrataEvent& e) {
        const float stepLen = camera.distance * 0.04f;
        switch (e.scancode) {
            case kEscape: if (o.escapeQuits) running = false; break;
            case kW: case kUp: camera.pan(0, stepLen); break;
            case kS: case kDown: camera.pan(0, -stepLen); break;
            case kA: case kLeft: camera.pan(-stepLen, 0); break;
            case kD: case kRight: camera.pan(stepLen, 0); break;
            case kQ: camera.yawDegrees -= 5.0f; break;
            case kE: camera.yawDegrees += 5.0f; break;
            case kEquals: case kKpPlus: camera.zoom(0.9f); break;
            case kMinus: case kKpMinus: camera.zoom(1.0f / 0.9f); break;
            case kSpace: if (!e.repeat) paused = !paused; break;
            case kN: if (paused) step(); break;
            case kComma: speed = std::max(speed - 1, 0); break;
            case kPeriod: speed = std::min(speed + 1, 5); break;
            case kTab: if (!e.repeat) hud = !hud; break;
            default: break;
        }
    }

    int headless() {
        int frame = 0;
        auto shoot = [&](float t) {
            draw(t);
            char name[64];
            std::snprintf(name, sizeof name, "/frame-%05d.png", frame++);
            engine->saveScreenshotPng(o.framesDir + name);
        };
        const int per = std::max(1, o.framesPerTick);
        if (!o.framesDir.empty()) shoot(1.0f);
        for (long i = 0; i < o.ticks; ++i) {
            step();
            if (!o.framesDir.empty())
                for (int f = 1; f <= per; ++f) shoot(static_cast<float>(f) / static_cast<float>(per));
        }
        draw(1.0f);
        if (!o.screenshot.empty()) engine->saveScreenshotPng(o.screenshot);
        if (!o.framesDir.empty()) std::printf("frames: %d to %s\n", frame, o.framesDir.c_str());
        return 0;
    }

    // Fits the camera to the grid with the options' angles and zoom.
    void fit() {
        camera = fitCamera(view.gridSize(), aspect());
        camera.yawDegrees = o.yawDegrees;
        camera.pitchDegrees = o.pitchDegrees;
        camera.distance /= std::max(0.05f, o.zoom);
    }

    void pickAtPixel(float px, float py) {
        int x = -1, y = -1;
        if (scene.pick(*engine, camera.toStrata(aspect()), w, h, px, py, x, y)) {
            pickX = x;
            pickY = y;
        } else {
            pickX = pickY = -1;
        }
    }

    void startWindow() {
        clock = std::make_unique<strata::sim::FixedStep>(1.0 / std::max(0.1, o.ticksPerSecond), 4);
        last = std::chrono::steady_clock::now();
        dragging = false;
    }

    void frame() {
        StrataEvent e;
        while (engine->pollEvent(e)) {
            if (e.type == STRATA_EVENT_QUIT) running = false;
            else if (e.type == STRATA_EVENT_RESIZED) engine->drawableSize(w, h);
            else if (e.type == STRATA_EVENT_MOUSE_WHEEL) camera.zoom(e.y > 0 ? 0.88f : 1.0f / 0.88f);
            else if (e.type == STRATA_EVENT_MOUSE_BUTTON_DOWN && e.button == 1) {
                dragging = true;
                downX = lastX = e.x * pointerScaleX;
                downY = lastY = e.y * pointerScaleY;
            } else if (e.type == STRATA_EVENT_MOUSE_BUTTON_UP && e.button == 1) {
                dragging = false;
                const float x = e.x * pointerScaleX, y = e.y * pointerScaleY;
                if (std::fabs(x - downX) + std::fabs(y - downY) < 5.0f * pointerScaleX) pickAtPixel(x, y);
            } else if (e.type == STRATA_EVENT_MOUSE_MOTION && dragging) {
                const float x = e.x * pointerScaleX, y = e.y * pointerScaleY;
                const float perPixel = camera.distance / static_cast<float>(std::max(1, h)) * 0.75f;
                camera.pan(-(x - lastX) * perPixel, (y - lastY) * perPixel);
                lastX = x;
                lastY = y;
            } else if (e.type == STRATA_EVENT_KEY_DOWN) key(e);
        }
        const auto now = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(now - last).count();
        last = now;
        clock->setTimeScale(paused ? 0.0 : kSpeeds[speed]);
        const int n = clock->advance(elapsed);
        for (int i = 0; i < n; ++i) step();
        draw(paused ? 1.0f : static_cast<float>(clock->alpha()));
        engine->present();
    }

    int window() {
        std::printf(
            "controls: drag or WASD/arrows pan, Q/E rotate, wheel or +/- zoom, click pick a location,\n"
            "          space pause, N step while paused, ,/. slower/faster, Tab HUD, Esc quit\n");
        startWindow();
        while (running) frame();
        return 0;
    }
};

ViewerApp::ViewerApp(envlibcpp::Environment& env, TickFn tick, ViewerOptions options, StyleFn style,
                     LocationColorFn locationColor)
    : env_(&env), tick_(std::move(tick)), options_(std::move(options)), view_(std::move(style), std::move(locationColor)) {}

ViewerApp::~ViewerApp() { stop(); }

bool ViewerApp::start() {
    stop();
    runner_ = std::make_unique<Runner>(*env_, tick_, options_, view_);
    Runner& r = *runner_;
    strata::EngineDesc desc;
    desc.surface = options_.headless ? STRATA_SURFACE_HEADLESS : STRATA_SURFACE_WINDOW;
    desc.width = options_.width;
    desc.height = options_.height;
    desc.title = options_.title;
    desc.flags = options_.headless ? 0 : STRATA_FLAG_VSYNC;
    try {
        r.engine = strata::Engine::create(desc);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "envlib-strata: cannot start the renderer: %s\n", e.what());
        runner_.reset();
        return false;
    }
    r.light = strata::lightingDefaults(STRATA_LIGHTING_SUN);
    r.light.time_of_day = 0.38f;
    r.light.latitude_degrees = 35.0f;
    r.light.fog = 0;
    r.light.cascade_count = 2;
    r.w = desc.width;
    r.h = desc.height;
    r.engine->drawableSize(r.w, r.h);
    r.hud = options_.hud;
    r.pickX = options_.pickX;
    r.pickY = options_.pickY;

    view_.sync(*env_);
    r.scene.updateFloor(*r.engine, view_);
    r.fit();
    if (!options_.headless) r.startWindow();
    return true;
}

void ViewerApp::frame() {
    if (!runner_ || !runner_->running || !runner_->clock) return;  // not started, closed, or headless
    try {
        runner_->frame();
    } catch (const std::exception& e) {
        // Said once, not every frame.
        static std::string lastError;
        if (lastError != e.what()) std::fprintf(stderr, "envlib-strata: %s\n", e.what());
        lastError = e.what();
    }
}

bool ViewerApp::running() const { return runner_ && runner_->running; }

void ViewerApp::stop() {
    if (!runner_) return;
    if (runner_->engine) runner_->scene.releaseFloor(*runner_->engine);
    runner_->engine.reset();
    runner_.reset();
}

int ViewerApp::run() {
    if (!start()) return 1;
    Runner& r = *runner_;
    int code = 0;
    try {
        code = options_.headless ? r.headless() : r.window();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "envlib-strata: %s\n", e.what());
        code = 1;
    }
    stop();
    return code;
}

void ViewerApp::setWorld(envlibcpp::Environment& env, TickFn tick, StyleFn style, LocationColorFn locationColor) {
    env_ = &env;
    tick_ = std::move(tick);
    view_ = EnvironmentView(std::move(style), std::move(locationColor));
    if (!runner_) return;
    Runner& r = *runner_;
    r.env = &env;
    r.scene.releaseFloor(*r.engine);
    r.ticks = 0;
    r.pickX = r.pickY = -1;
    view_.sync(env);
    r.scene.updateFloor(*r.engine, view_);
    r.fit();
    if (!options_.headless) r.startWindow();
}

void ViewerApp::key(int scancode) {
    if (!runner_) return;
    StrataEvent e{};
    e.type = STRATA_EVENT_KEY_DOWN;
    e.scancode = scancode;
    runner_->key(e);
}

void ViewerApp::zoom(float factor) {
    if (runner_ && factor > 0) runner_->camera.zoom(factor);
}

void ViewerApp::rotate(float degrees) {
    if (runner_) runner_->camera.yawDegrees += degrees;
}

void ViewerApp::home() {
    if (runner_) runner_->fit();
}

bool ViewerApp::pickAt(float px, float py) {
    if (!runner_) return false;
    runner_->pickAtPixel(px, py);
    return runner_->pickX >= 0;
}

void ViewerApp::resize(int width, int height) {
    if (!runner_ || width < 1 || height < 1) return;
    runner_->engine->resize(width, height);
    runner_->engine->drawableSize(runner_->w, runner_->h);
}

void ViewerApp::setPointerScale(float sx, float sy) {
    if (!runner_) return;
    runner_->pointerScaleX = sx > 0 ? sx : 1;
    runner_->pointerScaleY = sy > 0 ? sy : 1;
}

ViewerStatus ViewerApp::status() const {
    ViewerStatus s;
    s.environment = view_.environmentName();
    s.gridSize = view_.gridSize();
    s.entities = view_.entities().size();
    if (!runner_) return s;
    const Runner& r = *runner_;
    s.ticks = r.ticks;
    s.paused = r.paused;
    s.speed = Runner::kSpeeds[r.speed];
    s.hud = r.hud;
    s.pickX = r.pickX;
    s.pickY = r.pickY;
    if (r.pickX >= 0) s.pickedEntities = view_.entitiesAt(r.pickX, r.pickY).size();
    return s;
}

}  // namespace envlib_strata
