#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <string>
#include <iostream>
#include <assert.h>

#include <catch2/catch_template_test_macros.hpp>

#include "header/entity.h"
#include "header/environment.h"
#include "header/location.h"
#include "header/grid.h"

using namespace envlibcpp;

TEST_CASE("name", "[tag]") {
    REQUIRE(true);
}

TEST_CASE("placing entity in location", "[entity]") {
    Entity entity(0, "Daniel");
    Location location("my location id", 0, 0);
    location.addEntity(&entity);
    REQUIRE(location.isEntityPresent(&entity) == true);
    REQUIRE(entity.getLocationId() == location.getId());
}

TEST_CASE("removing entity from location", "[entity]") {
    Entity entity(0, "Daniel");
    Location location("location-0-0", 0, 0);
    location.addEntity(&entity);
    location.removeEntity(&entity);
    REQUIRE(!location.isEntityPresent(&entity));
    REQUIRE(entity.getLocationId() == "N/S");
}

TEST_CASE("generating grid", "[grid]") {
    int size = 4;
    Environment environment(0, "test environment", 4);
    int numLocations = environment.getGrid()->getLocations().size();
    int expectedNumLocations = size*size;
    REQUIRE(numLocations == expectedNumLocations);
}

TEST_CASE("retrieving location from grid", "[grid]") {
    Grid grid(0, 5);
    Location firstLocation = grid.getFirstLocation();
    REQUIRE(firstLocation.getX() == 0 && firstLocation.getY() == 0);
}

TEST_CASE("retrieving location from grid after modification", "[grid]") {
    Grid grid(0, 5);
    Location firstLocation = grid.getFirstLocation();
    Entity entity = Entity(0, "test");
    firstLocation = grid.getFirstLocation();
    firstLocation.addEntity(&entity);
    REQUIRE(firstLocation.getX() == 0 && firstLocation.getY() == 0);
    REQUIRE(firstLocation.isEntityPresent(&entity));
}

TEST_CASE("placing entity in grid", "[grid]") {
    Entity entity(0, "Daniel");
    Grid grid(0, 4);
    grid.addEntity(entity);
    REQUIRE(grid.isEntityPresent(entity));
    REQUIRE(entity.getGridId() == grid.getId());
}

TEST_CASE("removing entity from grid", "[grid]") {
    Entity entity(0, "Daniel");
    Grid grid(0, 4);
    grid.addEntity(entity);
    grid.removeEntity(entity);
    REQUIRE(!grid.isEntityPresent(entity));
    REQUIRE(entity.getGridId() == -1);
}

TEST_CASE("placing entity in environment", "[environment]") {
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 2);
    environment.addEntity(entity);
    REQUIRE(environment.isEntityPresent(entity));
    REQUIRE(entity.getEnvironmentId() == environment.getId());
}

TEST_CASE("removing entity from environment", "[environment]") {
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 2);
    environment.addEntity(entity);
    environment.removeEntity(entity);
    REQUIRE(!environment.isEntityPresent(entity));
    REQUIRE(entity.getEnvironmentId() == -1);
}

TEST_CASE("placing multiple entities in environment", "[environment]") {
    Entity entity1(0, "Bob");
    Entity entity2(1, "Phil");
    Entity entity3(2, "Clarisse");
    Environment environment(0, "Earth", 6);
    environment.addEntity(entity1);
    environment.addEntity(entity2);
    environment.addEntity(entity3);
    REQUIRE(environment.getNumEntities() == 3);
}

TEST_CASE("resetting entity location id", "[entity]") {
    Entity entity(0, "Mark");
    entity.setLocationId("0-0-0");
    REQUIRE(entity.getLocationId() == "0-0-0");
    entity.resetLocationId();
    REQUIRE(entity.getLocationId() == "N/S");
}

TEST_CASE("retrieving first entity in an environment", "[environment]") {
    Entity entity(34, "Daniel");
    Environment environment(0, "Earth", 3);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getFirstEntity();
    REQUIRE(retrievedEntity.getId() == entity.getId());
}

TEST_CASE("retrieving an entity from an environment by id", "[environment]") {
    Entity entity(57, "Daniel");
    Environment environment(0, "Earth", 5);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    REQUIRE(retrievedEntity.getId() == entity.getId());
}

TEST_CASE("retrieving location from a grid by id", "[grid]") {
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 7);
    environment.addEntity(entity);
    std::string locationId = entity.getLocationId();
    Location& location = environment.getGrid()->getLocation(locationId);
    REQUIRE(location.getId() == locationId);
}

TEST_CASE("moving entity to new location", "[entity]") {
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    std::string initialLocationId = entity.getLocationId();
    REQUIRE(initialLocationId != "N/S");

    Location* newLocation;
    do {
        newLocation = &environment.getGrid()->getRandomLocation();
    } while(newLocation->getId() == initialLocationId);
    
    environment.moveEntityToNewLocation(entity.getId(), newLocation->getId());
    std::string currentLocationId = entity.getLocationId();
    REQUIRE(currentLocationId == newLocation->getId());
}

TEST_CASE("moving entity to new location repeatedly", "[entity]") {
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    for (int i = 0; i < 10; i++) {
        std::string initialLocationId = entity.getLocationId();
        REQUIRE(initialLocationId != "N/S");

        Location* newLocation;
        do {
            newLocation = &environment.getGrid()->getRandomLocation();
        } while(newLocation->getId() == initialLocationId);

        environment.moveEntityToNewLocation(entity.getId(), newLocation->getId());
        std::string currentLocationId = entity.getLocationId();
        REQUIRE(currentLocationId == newLocation->getId());
    }
}

TEST_CASE("moving multiple entities to new locations", "[entity]") {
    std::vector<Entity> entities;
    Entity entity0(0, "Daniel");
    entities.push_back(entity0);
    Entity entity1(1, "Albert");
    entities.push_back(entity1);
    Entity entity2(2, "Charlie");
    entities.push_back(entity2);

    Environment environment(0, "Earth", 8);
    
    for (Entity& e : entities) {
        environment.addEntity(e);
    }

    for (int i = 0; i < 10; i++) {
        for (Entity& e : entities) {
            std::string initialLocationId = e.getLocationId();
            REQUIRE(initialLocationId != "N/S");

            Location* newLocation;
            do {
                newLocation = &environment.getGrid()->getRandomLocation();
            } while(newLocation->getId() == initialLocationId);
            
            environment.moveEntityToNewLocation(e.getId(), newLocation->getId());
            std::string currentLocationId = e.getLocationId();
            REQUIRE(currentLocationId == newLocation->getId());
        }
    }
}

TEST_CASE("moving multiple entities to new locations repeatedly", "[entity]") {
    std::vector<Entity> entities;
    Entity entity0(0, "Daniel");
    entities.push_back(entity0);
    Entity entity1(1, "Albert");
    entities.push_back(entity1);
    Entity entity2(2, "Charlie");
    entities.push_back(entity2);

    Environment environment(0, "Earth", 8);
    
    for (Entity& e : entities) {
        environment.addEntity(e);
    }

    for (Entity& e : entities) {
        std::string initialLocationId = e.getLocationId();
        REQUIRE(initialLocationId != "N/S");

        Location* newLocation;
        do {
            newLocation = &environment.getGrid()->getRandomLocation();
        } while(newLocation->getId() == initialLocationId);
        
        environment.moveEntityToNewLocation(e.getId(), newLocation->getId());
        std::string currentLocationId = e.getLocationId();
        REQUIRE(currentLocationId == newLocation->getId());
    }
}

TEST_CASE("moving entity up", "[entity]") {
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    std::string initialLocationId = retrievedEntity.getLocationId();
    REQUIRE(initialLocationId != "N/S");
    
    bool success = environment.moveEntityUp(retrievedEntity.getId());
    std::string currentLocationId = retrievedEntity.getLocationId();

    if (success) {
        // assert that the entity's location is upwards adjacent to the previous location
        Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
        Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
        REQUIRE(initialLocation.getX() == currentLocation.getX());
        REQUIRE(initialLocation.getY() - 1 == currentLocation.getY());
    }
    else {
        // assert that the entity's location has not changed
        REQUIRE(currentLocationId == initialLocationId);
    }
}

TEST_CASE("moving entity right", "[entity]") {
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    std::string initialLocationId = retrievedEntity.getLocationId();
    REQUIRE(initialLocationId != "N/S");
    
    bool success = environment.moveEntityRight(retrievedEntity.getId());
    std::string currentLocationId = retrievedEntity.getLocationId();

    if (success) {
        // assert that the entity's location is rightwards adjacent to the previous location
        Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
        Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
        REQUIRE(initialLocation.getX() + 1 == currentLocation.getX());
        REQUIRE(initialLocation.getY() == currentLocation.getY());
    }
    else {
        // assert that the entity's location has not changed
        REQUIRE(currentLocationId == initialLocationId);
    }
}

TEST_CASE("moving entity down", "[entity]") {
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    std::string initialLocationId = retrievedEntity.getLocationId();
    REQUIRE(initialLocationId != "N/S");
    
    bool success = environment.moveEntityDown(retrievedEntity.getId());
    std::string currentLocationId = retrievedEntity.getLocationId();

    if (success) {
        // assert that the entity's location is downwards adjacent to the previous location
        Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
        Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
        REQUIRE(initialLocation.getX() == currentLocation.getX());
        REQUIRE(initialLocation.getY() + 1 == currentLocation.getY());
    }
    else {
        // assert that the entity's location has not changed
        REQUIRE(currentLocationId == initialLocationId);
    }
}

TEST_CASE("moving entity left", "[entity]") {
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    std::string initialLocationId = retrievedEntity.getLocationId();
    REQUIRE(initialLocationId != "N/S");
    
    bool success = environment.moveEntityLeft(retrievedEntity.getId());
    std::string currentLocationId = retrievedEntity.getLocationId();

    if (success) {
        // assert that the entity's location is leftwards adjacent to the previous location
        Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
        Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
        REQUIRE(initialLocation.getX() - 1 == currentLocation.getX());
        REQUIRE(initialLocation.getY() == currentLocation.getY());
    }
    else {
        // assert that the entity's location has not changed
        REQUIRE(currentLocationId == initialLocationId);
    }
}

TEST_CASE("moving entity to random adjacent location", "[entity]") {
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    std::string initialLocationId = retrievedEntity.getLocationId();
    REQUIRE(initialLocationId != "N/S");
    
    bool success = environment.moveEntityToRandomAdjacentLocation(retrievedEntity.getId());
    std::string currentLocationId = retrievedEntity.getLocationId();

    if (success) {
        // assert that the entity's location is adjacent to the previous location
        Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
        Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
        REQUIRE((initialLocation.getX() == currentLocation.getX() && initialLocation.getY() - 1 == currentLocation.getY()) ||
                (initialLocation.getX() == currentLocation.getX() + 1 && initialLocation.getY() == currentLocation.getY()) ||
                (initialLocation.getX() == currentLocation.getX() && initialLocation.getY() + 1 == currentLocation.getY()) ||
                (initialLocation.getX() == currentLocation.getX() - 1 && initialLocation.getY() == currentLocation.getY()));
    }
    else {
        // assert that the entity's location has not changed
        REQUIRE(currentLocationId == initialLocationId);
    }
}

TEST_CASE("moving entity to random adjacent location repeatedly", "[entity]") {
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    for (int i = 0; i < 10; i++) {
        std::string initialLocationId = retrievedEntity.getLocationId();
        REQUIRE(initialLocationId != "N/S");
        
        bool success = environment.moveEntityToRandomAdjacentLocation(retrievedEntity.getId());
        std::string currentLocationId = retrievedEntity.getLocationId();

        if (success) {
            // assert that the entity's location is adjacent to the previous location
            Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
            Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
            REQUIRE((initialLocation.getX() == currentLocation.getX() && initialLocation.getY() - 1 == currentLocation.getY()) ||
                    (initialLocation.getX() == currentLocation.getX() + 1 && initialLocation.getY() == currentLocation.getY()) ||
                    (initialLocation.getX() == currentLocation.getX() && initialLocation.getY() + 1 == currentLocation.getY()) ||
                    (initialLocation.getX() == currentLocation.getX() - 1 && initialLocation.getY() == currentLocation.getY()));
        }
        else {
            // assert that the entity's location has not changed
            REQUIRE(currentLocationId == initialLocationId);
        }
    }
}

TEST_CASE("moving multiple entities to random adjacent locations", "[entity]") {
    std::vector<Entity> entities;
    Entity entity0(0, "Daniel");
    entities.push_back(entity0);
    Entity entity1(1, "Jack");
    entities.push_back(entity1);
    Entity entity2(2, "Kyle");
    entities.push_back(entity2);
    Entity entity3(3, "Bob");
    entities.push_back(entity3);
    Environment environment(0, "Earth", 8);
    for (Entity& entity : entities) {
        environment.addEntity(entity);
    }
    for (Entity& entity : entities) {
        Entity& retrievedEntity = environment.getEntity(entity.getId());
        std::string initialLocationId = retrievedEntity.getLocationId();
        REQUIRE(initialLocationId != "N/S");
        
        bool success = environment.moveEntityToRandomAdjacentLocation(retrievedEntity.getId());
        std::string currentLocationId = retrievedEntity.getLocationId();

        if (success) {
            // assert that the entity's location is adjacent to the previous location
            Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
            Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
            REQUIRE((initialLocation.getX() == currentLocation.getX() && initialLocation.getY() - 1 == currentLocation.getY()) ||
                    (initialLocation.getX() == currentLocation.getX() + 1 && initialLocation.getY() == currentLocation.getY()) ||
                    (initialLocation.getX() == currentLocation.getX() && initialLocation.getY() + 1 == currentLocation.getY()) ||
                    (initialLocation.getX() == currentLocation.getX() - 1 && initialLocation.getY() == currentLocation.getY()));
        }
        else {
            // assert that the entity's location has not changed
            REQUIRE(currentLocationId == initialLocationId);
        }
    }
}

TEST_CASE("moving multiple entities to random adjacent locations repeatedly", "[entity]") {
    bool debug = false;
    std::vector<Entity> entities;
    for (int i = 0; i < 10; i++) {
        Entity entity(i, "test");
        entities.push_back(entity);
    }
    Environment environment(0, "Earth", 8);
    for (Entity& entity : entities) {
        environment.addEntity(entity);
    }
    for (int i = 0; i < 25; i++) {
        if (debug) {
            environment.printConsoleRepresentation();
            sleep(1);
        }
        for (Entity& entity : entities) {
            Entity& retrievedEntity = environment.getEntity(entity.getId());
            std::string initialLocationId = retrievedEntity.getLocationId();
            REQUIRE(initialLocationId != "N/S");
            
            bool success = environment.moveEntityToRandomAdjacentLocation(retrievedEntity.getId());
            std::string currentLocationId = retrievedEntity.getLocationId();

            if (success) {
                // assert that the entity's location is adjacent to the previous location
                Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
                Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
                REQUIRE((initialLocation.getX() == currentLocation.getX() && initialLocation.getY() - 1 == currentLocation.getY()) ||
                        (initialLocation.getX() == currentLocation.getX() + 1 && initialLocation.getY() == currentLocation.getY()) ||
                        (initialLocation.getX() == currentLocation.getX() && initialLocation.getY() + 1 == currentLocation.getY()) ||
                        (initialLocation.getX() == currentLocation.getX() - 1 && initialLocation.getY() == currentLocation.getY()));
            }
            else {
                // assert that the entity's location has not changed
                REQUIRE(currentLocationId == initialLocationId);
            }
        }
    }
}