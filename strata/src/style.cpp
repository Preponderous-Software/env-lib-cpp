#include "envlib_strata/style.h"

#include <cmath>

namespace envlib_strata {

float hueOf(const std::string& text) {
    uint32_t h = 2166136261u;  // FNV-1a
    for (unsigned char c : text) {
        h ^= c;
        h *= 16777619u;
    }
    return static_cast<float>(h % 360u) / 360.0f;
}

std::array<float, 3> hsvToRgb(float h, float s, float v) {
    h = h - std::floor(h);
    const float i = std::floor(h * 6.0f);
    const float f = h * 6.0f - i;
    const float p = v * (1.0f - s);
    const float q = v * (1.0f - s * f);
    const float t = v * (1.0f - s * (1.0f - f));
    switch (static_cast<int>(i) % 6) {
        case 0: return {v, t, p};
        case 1: return {q, v, p};
        case 2: return {p, v, t};
        case 3: return {p, q, v};
        case 4: return {t, p, v};
        default: return {v, p, q};
    }
}

float srgbToLinear(float c) {
    return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

std::array<float, 3> srgbToLinear(const std::array<float, 3>& rgb) {
    return {srgbToLinear(rgb[0]), srgbToLinear(rgb[1]), srgbToLinear(rgb[2])};
}

Style defaultStyle(envlibcpp::Entity& entity) {
    Style s;
    const auto rgb = srgbToLinear(hsvToRgb(hueOf(entity.getName()), 0.55f, 0.9f));
    s.color[0] = rgb[0];
    s.color[1] = rgb[1];
    s.color[2] = rgb[2];
    s.color[3] = 1.0f;
    return s;
}

}  // namespace envlib_strata
