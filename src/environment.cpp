#include "header/environment.h"

#include <iostream>

namespace envlibcpp {
    Environment::Environment(std::string envName, int size) {
        id = rand() % 100 + 1;
        name = envName;
        grid = Grid(size);
    }

    int Environment::getId() {
        return id;
    }

    std::string Environment::getName() {
        return name;
    }

    Grid& Environment::getGrid() {
        return grid;
    }

    void Environment::setName(std::string newName) {
        name = newName;
    }

    void Environment::addEntity(Entity &entity) {
        entity.setEnvironmentId(id);
        grid.addEntity(entity);
    }

    void Environment::addEntityToLocation(Entity &entity, Location &location) {
        entity.setEnvironmentId(id);
        grid.addEntityToLocation(entity, location);
    }

    void Environment::removeEntity(Entity &entity) {
        entity.setEnvironmentId(-1);
        grid.removeEntity(entity);
    }

    bool Environment::isEntityPresent(Entity &entity) {
        return grid.isEntityPresent(entity);
    }

    int Environment::getNumEntities() {
        return grid.getNumEntities();
    }
    
    void Environment::printInfo() {
        std::cout << "=== " << getName() << " ===" << std::endl;
        std::cout << "Num entities: " << getNumEntities() << std::endl;
    }

    envlibcpp::Entity& Environment::getFirstEntity() {
        for (envlibcpp::Location &location : getGrid().getLocations()) {
            if (location.getNumEntities() > 0) {
                return location.getEntities()[0];
            }
        }
        throw new std::exception();
    }

    envlibcpp::Entity& Environment::getEntity(int entityId) {
        for (envlibcpp::Location &location : getGrid().getLocations()) {
            for (envlibcpp::Entity &entity : location.getEntities()) {
                if (entity.getId() == entityId) {
                    return entity;
                }
            }
        }
        throw new std::exception();
    }
}