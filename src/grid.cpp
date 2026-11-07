#include "header/grid.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace envlibcpp {
    Grid::Grid(int identifier, int gridSize) {
        id = identifier;
        size = gridSize;
        generateLocations();
    }

    int Grid::getId() {
        return id;
    }

    std::vector<Location>& Grid::getLocations() {
        return locations;
    }

    Location& Grid::getFirstLocation() {
        return locations.front();
    }

    Location& Grid::getLocationByCoordinates(int x, int y) {
        for (Location& location : locations) {
            if (location.getX() == x && location.getY() == y) {
                return location;
            }
        }
        throw std::runtime_error("Location at (" + std::to_string(x) + ", " + std::to_string(y) + ") not found in grid '" + std::to_string(id) + "'");
    }

    int Grid::getSize() {
        return size;
    }

    int Grid::getNumEntities() {
        int count = 0;
        for (Location& location : locations) {
            count = count + location.getNumEntities();
        }
        return count;
    }

    void Grid::setId(int newId) {
        id = newId;
    }

    void Grid::setSize(int newSize) {
        size = newSize;
    }

    void Grid::addLocation(Location& location) {
        locations.push_back(location);
    }

    void Grid::removeLocation(Location& location) {
        // the id is copied up front because `location` may itself refer to an element of
        // `locations`, which erase() would leave dangling
        std::string targetId = location.getId();
        for (auto i = locations.begin(); i != locations.end();) {
            if (i->getId() == targetId) {
                std::vector<Entity*> detached = i->getEntities();
                i = locations.erase(i);
                for (Entity* entity : detached) {
                    updateIdsOfDetachedEntity(entity, targetId);
                }
            } else {
                i++;
            }
        }
    }

    void Grid::updateIdsOfDetachedEntity(Entity* entity, std::string removedLocationId) {
        // compared by address, since a same-id twin elsewhere does not keep this entity placed
        for (Location& location : locations) {
            std::vector<Entity*>& entities = location.getEntities();
            if (std::find(entities.begin(), entities.end(), entity) != entities.end()) {
                if (entity->getLocationId() == removedLocationId) {
                    entity->setLocationId(location.getId());
                }
                return;
            }
        }
        // an entity left in no location is no longer part of the grid, so its ids are
        // cleared the same way removeEntity() clears them
        entity->resetLocationId();
        entity->setGridId(-1);
    }

    void Grid::addEntity(Entity& entity) {
        entity.setGridId(id);
        Location& firstLocation = getRandomLocation();
        firstLocation.addEntity(&entity);
    }

    bool Grid::addEntityToLocation(Entity& entity, Location& location) {
        // the entity goes into the grid's own element, not the argument, which may be a
        // separately constructed Location that merely shares the id
        for (Location& l : locations) {
            if (l.getId() == location.getId()) {
                entity.setGridId(id);
                l.addEntity(&entity);
                return true;
            }
        }
        return false;
    }

    void Grid::removeEntity(Entity& entity) {
        // an entity that is not in this grid keeps its ids
        if (!isEntityPresent(entity)) {
            return;
        }
        for (Location& location : locations) {
            if (location.isEntityPresent(&entity)) {
                location.removeEntity(&entity);
                entity.resetLocationId();
            }
        }
        entity.setGridId(-1);
    }

    bool Grid::isEntityPresent(Entity& entity) {
        for (Location& location : locations) {
            if (location.isEntityPresent(&entity)) {
                return true;
            }
        }
        return false;
    }

    Location& Grid::getLocation(std::string locationId) {
        for (Location& location : locations) {
            if (location.getId() == locationId) {
                return location;
            }
        }
        throw std::runtime_error("Location with id '" + locationId + "' not found in grid '" + std::to_string(id) + "'");
    }

    Location& Grid::getRandomLocation() {
        int index = rand() % getLocations().size();
        return getLocations()[index];
    }
    
    void Grid::generateLocations() {
        for (int y = 0; y < getSize(); y++) {
            for (int x = 0; x < getSize(); x++) {
                std::string identifier = "" + std::to_string(getId()) + "-" + std::to_string(x) + "-" + std::to_string(y);
                Location location(identifier, x, y);
                locations.push_back(location);
            }
        }
    }
}