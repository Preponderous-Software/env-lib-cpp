#include "header/location.h"

#include <algorithm>
#include <iostream>

namespace envlibcpp {
    Location::Location(std::string identifier, int xpos, int ypos) {
        id = identifier;
        x = xpos;
        y = ypos;
    }

    std::string Location::getId() {
        return id;
    }

    int Location::getX() {
        return x;
    }
    
    int Location::getY() {
        return y;
    }
    
    size_t Location::getNumEntities() {
        return entities.size();
    }
    
    void Location::addEntity(Entity* entity) {
        entity->setLocationId(getId());
        entities.push_back(entity);
    }
    
    void Location::removeEntity(Entity* entity) {
        // every match goes, since an entity added twice is stored twice and must not stay present
        auto newEnd = std::remove_if(entities.begin(), entities.end(), [entity](Entity* e) {
            return e->getId() == entity->getId();
        });
        // an entity that is not in this location keeps its location id
        if (newEnd != entities.end()) {
            entities.erase(newEnd, entities.end());
            entity->resetLocationId();
        }
    }
    
    bool Location::isEntityPresent(Entity* entity) {
        for (Entity* e : entities) {
            if (e->getId() == entity->getId()) {
                return true;
            }
        }
        return false;
    }
    
    std::vector<Entity*>& Location::getEntities() {
        return entities;
    }
    
}