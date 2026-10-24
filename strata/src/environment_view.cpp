#include "envlib_strata/environment_view.h"

#include <cstdlib>
#include <unordered_set>
#include <utility>

namespace envlib_strata {

EnvironmentView::EnvironmentView(StyleFn style, LocationColorFn locationColor)
    : style_(style ? std::move(style) : StyleFn(defaultStyle)), locationColor_(std::move(locationColor)) {}

void EnvironmentView::sync(envlibcpp::Environment& env) {
    ++syncs_;
    warnings_.clear();
    environmentName_ = env.getName();
    envlibcpp::Grid* grid = env.getGrid();
    const int size = grid ? grid->getSize() : 0;
    bool floorChanged = size != gridSize_;
    gridSize_ = size;

    std::vector<EntitySnapshot> next;
    std::vector<std::array<float, 3>> colors;
    if (locationColor_) colors.assign(static_cast<size_t>(size) * static_cast<size_t>(size), {0.0f, 0.0f, 0.0f});
    std::unordered_set<int> seen;
    std::unordered_map<int, std::array<int, 2>> cells;

    if (grid) {
        // The only read of getLocations(); the reference is not kept.
        for (envlibcpp::Location& location : grid->getLocations()) {
            const int x = location.getX();
            const int y = location.getY();
            const bool inside = x >= 0 && y >= 0 && x < size && y < size;
            if (!inside) {
                warnings_.push_back("location " + location.getId() + " lies outside the " + std::to_string(size) +
                                    "x" + std::to_string(size) + " grid; not drawn");
                continue;
            }
            if (locationColor_) colors[static_cast<size_t>(y) * size + x] = locationColor_(location);
            for (envlibcpp::Entity* entity : location.getEntities()) {
                if (!entity) continue;
                EntitySnapshot s;
                s.id = entity->getId();
                s.name = entity->getName();
                s.x = x;
                s.y = y;
                s.style = style_(*entity);
                if (!seen.insert(s.id).second) {
                    warnings_.push_back("entity id " + std::to_string(s.id) +
                                        " appears in more than one place; each is drawn");
                }
                const auto last = lastCell_.find(s.id);
                if (last != lastCell_.end()) {
                    s.isNew = false;
                    s.prevX = last->second[0];
                    s.prevY = last->second[1];
                } else {
                    s.prevX = x;
                    s.prevY = y;
                }
                cells[s.id] = {x, y};
                next.push_back(std::move(s));
            }
        }
    }
    if (colors != locationColors_) floorChanged = true;
    locationColors_ = std::move(colors);
    if (floorChanged) ++floorRevision_;
    entities_ = std::move(next);
    lastCell_ = std::move(cells);
}

std::vector<size_t> EnvironmentView::entitiesAt(int x, int y) const {
    std::vector<size_t> out;
    for (size_t i = 0; i < entities_.size(); ++i)
        if (entities_[i].x == x && entities_[i].y == y) out.push_back(i);
    return out;
}

}  // namespace envlib_strata
