// How an entity looks: the one thing a consumer of the frontend writes.
//
// An env-lib-cpp Entity has an id and a name and nothing else; a consumer
// gives it meaning by subclassing. The frontend cannot know that meaning, so
// the consumer passes a StyleFn that turns an entity into a Style. It is
// called once per entity per sync(), never per frame.
//
// Colours are linear RGB, as Strata expects. srgbToLinear() converts values
// picked in sRGB (most 2D palettes, colour pickers and CSS hex codes).
//
// Copyright (c) 2026 Daniel McCoy Stephenson. MIT licence; see LICENSE.
#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>

#include "strata/strata.h"

#include "entity.h"
#include "location.h"

namespace envlib_strata {

struct Style {
    int32_t primitive = STRATA_PRIMITIVE_CUBE;  // one of Strata's four kinds
    float color[4] = {0.8f, 0.8f, 0.8f, 1.0f};  // linear RGB(A)
    float scale[3] = {0.5f, 0.5f, 0.5f};        // world units; a cell is 1.0
    float lift = 0.0f;                          // extra height above the floor
    bool visible = true;
};

// Called during sync() only. The entity is never changed by the frontend;
// the reference is non-const only because env-lib-cpp's getters are.
using StyleFn = std::function<Style(envlibcpp::Entity&)>;

// Optional per-location floor colour (linear RGB), also called during sync().
using LocationColorFn = std::function<std::array<float, 3>(envlibcpp::Location&)>;

// The default: a cube whose hue is a hash of the entity's name, so entities
// with the same name (the same kind, in most consumers) share a colour.
Style defaultStyle(envlibcpp::Entity& entity);

// A stable hue in [0, 1) for a string (FNV-1a).
float hueOf(const std::string& text);

// HSV (each 0..1) to RGB (0..1), no colour-space conversion.
std::array<float, 3> hsvToRgb(float h, float s, float v);

// One sRGB channel (0..1) to linear.
float srgbToLinear(float c);
std::array<float, 3> srgbToLinear(const std::array<float, 3>& rgb);

}  // namespace envlib_strata
