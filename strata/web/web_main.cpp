// env-lib-cpp's demo worlds in a browser: the same ViewerApp on Strata's
// WebGL 2 backend (Emscripten), one frame per browser callback.
//
// The page (strata/web/shell.html) picks a world and has buttons for what
// keys do on the desktop. Mouse and touch reach the canvas through SDL as on
// the desktop: drag to pan, wheel to zoom, click or tap to pick a location.
//
// Copyright (c) 2026 Daniel McCoy Stephenson. MIT licence; see LICENSE.
#include <cstdio>
#include <memory>
#include <string>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include "envlib_strata/viewer_app.h"
#include "worlds.h"

namespace {

struct WorldDef {
    const char* id;
    unsigned seed;  // the desktop demo's defaults
};

// The order the page's buttons use.
const WorldDef kWorlds[] = {{"walkers", 7}, {"meadow", 1}};
constexpr int kWorldCount = static_cast<int>(sizeof kWorlds / sizeof kWorlds[0]);

struct App {
    std::unique_ptr<envlib_strata::ViewerApp> viewer;
    std::unique_ptr<demo::World> world;
    int index = -1;
    double cssW = 0, cssH = 0;  // the canvas in CSS pixels (the page's coordinates)
    double winW = 0, winH = 0;  // SDL's window size (its pointer coordinates)
    std::string status;
};

App* app = nullptr;

// SDL's pointer events are in its window's units (the size the page asked
// for, which is less than the CSS size when the page caps a high device
// pixel ratio); the drawable is that times the device pixel ratio.
void fitPointer() {
    if (!app->viewer || app->winW < 1 || app->winH < 1) return;
    int dw = 0, dh = 0;
    emscripten_get_canvas_element_size("#canvas", &dw, &dh);
    app->viewer->setPointerScale(static_cast<float>(dw / app->winW), static_cast<float>(dh / app->winH));
}

void frame(void*) {
    if (app->viewer) app->viewer->frame();
}

void jsonString(std::string& out, const std::string& s) {
    out += '"';
    for (char ch : s) {
        if (ch == '"' || ch == '\\') {
            out += '\\';
            out += ch;
        } else if (static_cast<unsigned char>(ch) < 0x20) {
            out += ' ';
        } else {
            out += ch;
        }
    }
    out += '"';
}

}  // namespace

extern "C" {

// Opens world `index` (see kWorlds) from its first tick; 1 on success.
EMSCRIPTEN_KEEPALIVE int envlib_open(int index) {
    if (!app || !app->viewer || index < 0 || index >= kWorldCount) return 0;
    auto w = demo::makeWorld(kWorlds[index].id, kWorlds[index].seed);
    if (!w) return 0;
    demo::World* p = w.get();
    app->viewer->setWorld(p->environment(), [p] { p->tick(); }, p->style(), p->locationColor());
    app->world = std::move(w);  // the old world goes only after the viewer let go of it
    app->index = index;
    return 1;
}
// The canvas changed size: it is cssW x cssH CSS pixels, drawn at w x h
// (the page caps the device pixel ratio).
EMSCRIPTEN_KEEPALIVE void envlib_resize(double w, double h, double cssW, double cssH) {
    if (!app || !app->viewer || w < 1 || h < 1 || cssW < 1 || cssH < 1) return;
    app->cssW = cssW;
    app->cssH = cssH;
    app->winW = w;
    app->winH = h;
    app->viewer->resize(static_cast<int>(w), static_cast<int>(h));
    fitPointer();
    app->viewer->home();  // frame the grid for the new shape (a phone turned)
}
// A key press from a page button (an SDL scancode, as the keyboard sends).
EMSCRIPTEN_KEEPALIVE void envlib_key(int scancode) {
    if (app && app->viewer) app->viewer->key(scancode);
}
EMSCRIPTEN_KEEPALIVE void envlib_zoom(double factor) {
    if (app && app->viewer) app->viewer->zoom(static_cast<float>(factor));
}
EMSCRIPTEN_KEEPALIVE void envlib_rotate(double degrees) {
    if (app && app->viewer) app->viewer->rotate(static_cast<float>(degrees));
}
EMSCRIPTEN_KEEPALIVE void envlib_home() {
    if (app && app->viewer) app->viewer->home();
}
// Picks the location under a point of the canvas, in CSS pixels; 1 if one was.
EMSCRIPTEN_KEEPALIVE int envlib_pick(double x, double y) {
    if (!app || !app->viewer || app->cssW < 1 || app->cssH < 1) return 0;
    int dw = 0, dh = 0;
    emscripten_get_canvas_element_size("#canvas", &dw, &dh);
    return app->viewer->pickAt(static_cast<float>(x * dw / app->cssW), static_cast<float>(y * dh / app->cssH)) ? 1
                                                                                                             : 0;
}
// The viewer's state as JSON, for the page's status line and the smoke test.
EMSCRIPTEN_KEEPALIVE const char* envlib_status() {
    if (!app || !app->viewer) return "{}";
    const envlib_strata::ViewerStatus s = app->viewer->status();
    std::string o = "{\"world\":";
    jsonString(o, app->index >= 0 ? kWorlds[app->index].id : "");
    o += ",\"environment\":";
    jsonString(o, s.environment);
    char buf[256];
    std::snprintf(buf, sizeof buf,
                  ",\"grid\":%d,\"tick\":%ld,\"entities\":%zu,\"paused\":%s,\"speed\":%g,\"hud\":%s,"
                  "\"pickX\":%d,\"pickY\":%d,\"picked\":%zu",
                  s.gridSize, s.ticks, s.entities, s.paused ? "true" : "false", s.speed, s.hud ? "true" : "false",
                  s.pickX, s.pickY, s.pickedEntities);
    o += buf;
    o += ",\"here\":[";
    if (s.pickX >= 0) {
        const auto& view = app->viewer->view();
        bool first = true;
        for (size_t i : view.entitiesAt(s.pickX, s.pickY)) {
            const auto& e = view.entities()[i];
            if (!first) o += ',';
            first = false;
            std::snprintf(buf, sizeof buf, "{\"id\":%d,\"name\":", e.id);
            o += buf;
            jsonString(o, e.name);
            o += '}';
        }
    }
    o += "]}";
    app->status = std::move(o);
    return app->status.c_str();
}

}  // extern "C"

int main() {
    static App a;
    app = &a;
    double cw = 0, ch = 0;
    emscripten_get_element_css_size("#canvas", &cw, &ch);
    if (cw < 1 || ch < 1) {
        cw = 1280;
        ch = 720;
    }
    app->cssW = app->winW = cw;
    app->cssH = app->winH = ch;

    // A placeholder world until the page opens one; ViewerApp needs one to start.
    app->world = demo::makeWorld(kWorlds[0].id, kWorlds[0].seed);
    demo::World* p = app->world.get();
    envlib_strata::ViewerOptions o;
    o.title = "env-lib-cpp on Strata";
    o.width = static_cast<int>(cw);
    o.height = static_cast<int>(ch);
    o.keyHints = false;      // the page has buttons
    o.escapeQuits = false;   // and nothing to quit
    app->viewer = std::make_unique<envlib_strata::ViewerApp>(p->environment(), [p] { p->tick(); }, o, p->style(),
                                                             p->locationColor());
    if (!app->viewer->start()) {
        app->viewer.reset();
        emscripten_run_script("if (Module.onRendererFailed) Module.onRendererFailed('see the console')");
        return 1;
    }
    app->index = 0;
    fitPointer();
    emscripten_run_script("if (Module.onViewerReady) Module.onViewerReady()");
    emscripten_set_main_loop_arg(frame, nullptr, 0, false);
    return 0;
}
