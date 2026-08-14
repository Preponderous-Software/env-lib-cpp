#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <string>
#include <iostream>
#include <sstream>
#include <assert.h>

#include "header/entity.h"
#include "header/environment.h"
#include "header/location.h"
#include "header/grid.h"

using namespace envlibcpp;

void testTemplate() {
    std::cout << "Test 0 - Template";
    assert(true);
    std::cout << " --- " << "Success" << std::endl;
}

void testPlacingEntityInLocation() {
    std::cout << "Test 1 - Placing entity in location";
    Entity entity(0, "Daniel");
    Location location("my location id", 0, 0);
    location.addEntity(&entity);
    assert(location.isEntityPresent(&entity) == true);
    assert(entity.getLocationId() == location.getId());
    std::cout << " --- " << "Success" << std::endl;
}

void testRemovingEntityFromLocation() {
    std::cout << "Test 2 - Removing entity from location";
    Entity entity(0, "Daniel");
    Location location("location-0-0", 0, 0);
    location.addEntity(&entity);
    location.removeEntity(&entity);
    assert(!location.isEntityPresent(&entity));
    assert(entity.getLocationId() == "N/S");
    std::cout << " --- " << "Success" << std::endl;
}

void testGeneratingGrid() {
    std::cout << "Test 3 - Generating grid";
    int size = 4;
    Environment environment(0, "test environment", 4);
    int numLocations = environment.getGrid()->getLocations().size();
    int expectedNumLocations = size*size;
    assert(numLocations == expectedNumLocations);
    std::cout << " --- " << "Success" << std::endl;
}

void testRetrievingFirstLocation() {
    std::cout << "Test 4 - Retrieving location";
    Grid grid(0, 5);
    Location firstLocation = grid.getFirstLocation();
    assert(firstLocation.getX() == 0 && firstLocation.getY() == 0);
    std::cout << " --- " << "Success" << std::endl;
}

void testRetrievingLocationAfterModification() {
    std::cout << "Test 5 - Retrieving location after modification";
    Grid grid(0, 5);
    Location firstLocation = grid.getFirstLocation();
    Entity entity = Entity(0, "test");
    firstLocation = grid.getFirstLocation();
    firstLocation.addEntity(&entity);
    assert(firstLocation.getX() == 0 && firstLocation.getY() == 0);
    assert(firstLocation.isEntityPresent(&entity));
    std::cout << " --- " << "Success" << std::endl;
}

void testPlacingEntityInGrid() {
    std::cout << "Test 6 - Placing entity in grid";
    Entity entity(0, "Daniel");
    Grid grid(0, 4);
    grid.addEntity(entity);
    assert(grid.isEntityPresent(entity));
    assert(entity.getGridId() == grid.getId());
    std::cout << " --- " << "Success" << std::endl;
}

void testRemovingEntityFromGrid() {
    std::cout << "Test 7 - Removing entity from grid";
    Entity entity(0, "Daniel");
    Grid grid(0, 4);
    grid.addEntity(entity);
    grid.removeEntity(entity);
    assert(!grid.isEntityPresent(entity));
    assert(entity.getGridId() == -1);
    std::cout << " --- " << "Success" << std::endl;
}

void testPlacingEntityInEnvironment() {
    std::cout << "Test 8 - Placing entity in environment";
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 2);
    environment.addEntity(entity);
    assert(environment.isEntityPresent(entity));
    assert(entity.getEnvironmentId() == environment.getId());
    std::cout << " --- " << "Success" << std::endl;
}

void testRemovingEntityFromEnvironment() {
    std::cout << "Test 9 - Removing entity from environment";
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 2);
    environment.addEntity(entity);
    environment.removeEntity(entity);
    assert(!environment.isEntityPresent(entity));
    assert(entity.getEnvironmentId() == -1);
    std::cout << " --- " << "Success" << std::endl;
}

void testPlacingMultipleEntities() {
    std::cout << "Test 10 - Placing multiple entities";
    Entity entity1(0, "Bob");
    Entity entity2(1, "Phil");
    Entity entity3(2, "Clarisse");
    Environment environment(0, "Earth", 6);
    environment.addEntity(entity1);
    environment.addEntity(entity2);
    environment.addEntity(entity3);
    assert(environment.getNumEntities() == 3);
    std::cout << " --- " << "Success" << std::endl;
}

void testResettingEntityLocationId() {
    std::cout << "Test 11 - Resetting entity location id";
    Entity entity(0, "Mark");
    entity.setLocationId("0-0-0");
    assert(entity.getLocationId() == "0-0-0");
    entity.resetLocationId();
    assert(entity.getLocationId() == "N/S");
    std::cout << " --- " << "Success" << std::endl;
}

void testRetrievingFirstEntity() {
    std::cout << "Test 12 - Retrieving first entity in an environment";
    Entity entity(34, "Daniel");
    Environment environment(0, "Earth", 3);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getFirstEntity();
    assert(retrievedEntity.getId() == entity.getId());
    std::cout << " --- " << "Success" << std::endl;
}

void testRetrievingEntityById() {
    std::cout << "Test 13 - Retrieving an entity from an environment by id";
    Entity entity(57, "Daniel");
    Environment environment(0, "Earth", 5);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    assert(retrievedEntity.getId() == entity.getId());
    std::cout << " --- " << "Success" << std::endl;
}

void testRetrievingLocationById() {
    std::cout << "Test 14 - Retrieving location by id from an grid.";
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 7);
    environment.addEntity(entity);
    std::string locationId = entity.getLocationId();
    Location& location = environment.getGrid()->getLocation(locationId);
    assert(location.getId() == locationId);
    std::cout << " --- " << "Success" << std::endl;
}

void testMovingEntityToNewLocation() {
    std::cout << "Test 15 - Moving entity to new location";
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    std::string initialLocationId = entity.getLocationId();
    assert(initialLocationId != "N/S");

    Location* newLocation;
    do {
        newLocation = &environment.getGrid()->getRandomLocation();
    } while(newLocation->getId() == initialLocationId);
    
    environment.moveEntityToNewLocation(entity.getId(), newLocation->getId());
    std::string currentLocationId = entity.getLocationId();
    assert(currentLocationId == newLocation->getId());
    std::cout << " --- " << "Success" << std::endl;
}

void testMovingEntityToNewLocationRepeatedly() {
    std::cout << "Test 16 - Moving entity to new location repeatedly.";
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    for (size_t i = 0; i < 10; i++) {
        std::string initialLocationId = entity.getLocationId();
        assert(initialLocationId != "N/S");

        Location* newLocation;
        do {
            newLocation = &environment.getGrid()->getRandomLocation();
        } while(newLocation->getId() == initialLocationId);

        environment.moveEntityToNewLocation(entity.getId(), newLocation->getId());
        std::string currentLocationId = entity.getLocationId();
        assert(currentLocationId == newLocation->getId());
    }
    std::cout << " --- " << "Success" << std::endl;
}

void testMovingMultipleEntitiesToNewLocations() {
    std::cout << "Test 17 - Moving multiple entities to new locations";

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

    for (size_t i = 0; i < 10; i++) {
        for (Entity& e : entities) {
            std::string initialLocationId = e.getLocationId();
            assert(initialLocationId != "N/S");

            Location* newLocation;
            do {
                newLocation = &environment.getGrid()->getRandomLocation();
            } while(newLocation->getId() == initialLocationId);
            
            environment.moveEntityToNewLocation(e.getId(), newLocation->getId());
            std::string currentLocationId = e.getLocationId();
            assert(currentLocationId == newLocation->getId());
        }
    }

    std::cout << " --- " << "Success" << std::endl;
}

void testMovingMultipleEntitiesToNewLocationsRepeatedly() {
    std::cout << "Test 18 - Moving multiple entities to new locations repeatedly";

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
        assert(initialLocationId != "N/S");

        Location* newLocation;
        do {
            newLocation = &environment.getGrid()->getRandomLocation();
        } while(newLocation->getId() == initialLocationId);
        
        environment.moveEntityToNewLocation(e.getId(), newLocation->getId());
        std::string currentLocationId = e.getLocationId();
        assert(currentLocationId == newLocation->getId());
    }

    std::cout << " --- " << "Success" << std::endl;
}

void testMovingEntityUp() {
    std::cout << "Test 19 - Moving entity up";
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    std::string initialLocationId = retrievedEntity.getLocationId();
    assert(initialLocationId != "N/S");
    
    bool success = environment.moveEntityUp(retrievedEntity.getId());
    std::string currentLocationId = retrievedEntity.getLocationId();

    if (success) {
        // assert that the entity's location is upwards adjacent to the previous location
        Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
        Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
        assert(initialLocation.getX() == currentLocation.getX());
        assert(initialLocation.getY() - 1 == currentLocation.getY());
    }
    else {
        // assert that the entity's location has not changed
        assert(currentLocationId == initialLocationId);
    }
    
    std::cout << " --- " << "Success" << std::endl;
}

void testMovingEntityRight() {
    std::cout << "Test 20 - Moving entity right";
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    std::string initialLocationId = retrievedEntity.getLocationId();
    assert(initialLocationId != "N/S");
    
    bool success = environment.moveEntityRight(retrievedEntity.getId());
    std::string currentLocationId = retrievedEntity.getLocationId();

    if (success) {
        // assert that the entity's location is rightwards adjacent to the previous location
        Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
        Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
        assert(initialLocation.getX() + 1 == currentLocation.getX());
        assert(initialLocation.getY() == currentLocation.getY());
    }
    else {
        // assert that the entity's location has not changed
        assert(currentLocationId == initialLocationId);
    }
    
    std::cout << " --- " << "Success" << std::endl;
}

void testMovingEntityDown() {
    std::cout << "Test 21 - Moving entity down";
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    std::string initialLocationId = retrievedEntity.getLocationId();
    assert(initialLocationId != "N/S");
    
    bool success = environment.moveEntityDown(retrievedEntity.getId());
    std::string currentLocationId = retrievedEntity.getLocationId();

    if (success) {
        // assert that the entity's location is downwards adjacent to the previous location
        Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
        Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
        assert(initialLocation.getX() == currentLocation.getX());
        assert(initialLocation.getY() + 1 == currentLocation.getY());
    }
    else {
        // assert that the entity's location has not changed
        assert(currentLocationId == initialLocationId);
    }
    
    std::cout << " --- " << "Success" << std::endl;
}

void testMovingEntityLeft() {
    std::cout << "Test 22 - Moving entity left";
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    std::string initialLocationId = retrievedEntity.getLocationId();
    assert(initialLocationId != "N/S");
    
    bool success = environment.moveEntityLeft(retrievedEntity.getId());
    std::string currentLocationId = retrievedEntity.getLocationId();

    if (success) {
        // assert that the entity's location is leftwards adjacent to the previous location
        Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
        Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
        assert(initialLocation.getX() - 1 == currentLocation.getX());
        assert(initialLocation.getY() == currentLocation.getY());
    }
    else {
        // assert that the entity's location has not changed
        assert(currentLocationId == initialLocationId);
    }
    
    std::cout << " --- " << "Success" << std::endl;
}

void testMovingEntityToRandomAdjacentLocation() {
    std::cout << "Test 23 - Moving entity to random adjacent location";
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    std::string initialLocationId = retrievedEntity.getLocationId();
    assert(initialLocationId != "N/S");
    
    bool success = environment.moveEntityToRandomAdjacentLocation(retrievedEntity.getId());
    std::string currentLocationId = retrievedEntity.getLocationId();

    if (success) {
        // assert that the entity's location is adjacent to the previous location
        Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
        Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
        assert((initialLocation.getX() == currentLocation.getX() && initialLocation.getY() - 1 == currentLocation.getY()) ||
                (initialLocation.getX() == currentLocation.getX() + 1 && initialLocation.getY() == currentLocation.getY()) ||
                (initialLocation.getX() == currentLocation.getX() && initialLocation.getY() + 1 == currentLocation.getY()) ||
                (initialLocation.getX() == currentLocation.getX() - 1 && initialLocation.getY() == currentLocation.getY()));
    }
    else {
        // assert that the entity's location has not changed
        assert(currentLocationId == initialLocationId);
    }
    
    std::cout << " --- " << "Success" << std::endl;
}

void testMovingEntityToRandomAdjacentLocationRepeatedly() {
    std::cout << "Test 24 - Moving entity to random adjacent location repeatedly";
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 8);
    environment.addEntity(entity);
    Entity& retrievedEntity = environment.getEntity(entity.getId());
    for (size_t i = 0; i < 10; i++) {
        std::string initialLocationId = retrievedEntity.getLocationId();
        assert(initialLocationId != "N/S");
        
        bool success = environment.moveEntityToRandomAdjacentLocation(retrievedEntity.getId());
        std::string currentLocationId = retrievedEntity.getLocationId();

        if (success) {
            // assert that the entity's location is adjacent to the previous location
            Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
            Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
            assert((initialLocation.getX() == currentLocation.getX() && initialLocation.getY() - 1 == currentLocation.getY()) ||
                    (initialLocation.getX() == currentLocation.getX() + 1 && initialLocation.getY() == currentLocation.getY()) ||
                    (initialLocation.getX() == currentLocation.getX() && initialLocation.getY() + 1 == currentLocation.getY()) ||
                    (initialLocation.getX() == currentLocation.getX() - 1 && initialLocation.getY() == currentLocation.getY()));
        }
        else {
            // assert that the entity's location has not changed
            assert(currentLocationId == initialLocationId);
        }
    }

    std::cout << " --- " << "Success" << std::endl;
}

void testMovingMultipleEntitiesToRandomAdjacentLocations() {
    std::cout << "Test 25 - Moving multiple entities to random adjacent locations";
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
        assert(initialLocationId != "N/S");
        
        bool success = environment.moveEntityToRandomAdjacentLocation(retrievedEntity.getId());
        std::string currentLocationId = retrievedEntity.getLocationId();

        if (success) {
            // assert that the entity's location is adjacent to the previous location
            Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
            Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
            assert((initialLocation.getX() == currentLocation.getX() && initialLocation.getY() - 1 == currentLocation.getY()) ||
                    (initialLocation.getX() == currentLocation.getX() + 1 && initialLocation.getY() == currentLocation.getY()) ||
                    (initialLocation.getX() == currentLocation.getX() && initialLocation.getY() + 1 == currentLocation.getY()) ||
                    (initialLocation.getX() == currentLocation.getX() - 1 && initialLocation.getY() == currentLocation.getY()));
        }
        else {
            // assert that the entity's location has not changed
            assert(currentLocationId == initialLocationId);
        }
    }

    std::cout << " --- " << "Success" << std::endl;
}

void testMovingMultipleEntitiesToRandomAdjacentLocationsRepeatedly() {
    bool debug = false;
    std::cout << "Test 26 - Moving multiple entities to random adjacent locations repeatedly";
    std::vector<Entity> entities;
    for (size_t i = 0; i < 10; i++) {
        Entity entity(i, "test");
        entities.push_back(entity);
    }
    Environment environment(0, "Earth", 8);
    for (Entity& entity : entities) {
        environment.addEntity(entity);
    }
    for (size_t i = 0; i < 25; i++) {
        if (debug) {
            environment.printConsoleRepresentation();
            sleep(1);
        }
        for (Entity& entity : entities) {
            Entity& retrievedEntity = environment.getEntity(entity.getId());
            std::string initialLocationId = retrievedEntity.getLocationId();
            assert(initialLocationId != "N/S");
            
            bool success = environment.moveEntityToRandomAdjacentLocation(retrievedEntity.getId());
            std::string currentLocationId = retrievedEntity.getLocationId();

            if (success) {
                // assert that the entity's location is adjacent to the previous location
                Location& initialLocation = environment.getGrid()->getLocation(initialLocationId);
                Location& currentLocation = environment.getGrid()->getLocation(currentLocationId);
                assert((initialLocation.getX() == currentLocation.getX() && initialLocation.getY() - 1 == currentLocation.getY()) ||
                        (initialLocation.getX() == currentLocation.getX() + 1 && initialLocation.getY() == currentLocation.getY()) ||
                        (initialLocation.getX() == currentLocation.getX() && initialLocation.getY() + 1 == currentLocation.getY()) ||
                        (initialLocation.getX() == currentLocation.getX() - 1 && initialLocation.getY() == currentLocation.getY()));
            }
            else {
                // assert that the entity's location has not changed
                assert(currentLocationId == initialLocationId);
            }
        }
    }
    std::cout << " --- " << "Success" << std::endl;
}

void testSettingEnvironmentName() {
    std::cout << "Test 27 - Setting environment name";
    Environment environment(0, "Earth", 4);
    environment.setName("Mars");
    assert(environment.getName() == "Mars");
    std::cout << " --- " << "Success" << std::endl;
}

void testPrintingEnvironmentInfo() {
    std::cout << "Test 28 - Printing environment info";
    Environment environment(0, "Test Env", 2);
    std::ostringstream buffer;
    std::streambuf* previousBuffer = std::cout.rdbuf(buffer.rdbuf());
    environment.printInfo();
    std::cout.rdbuf(previousBuffer);
    std::string output = buffer.str();
    assert(output.find("Test Env") != std::string::npos);
    assert(output.find("Num entities: 0") != std::string::npos);
    std::cout << " --- " << "Success" << std::endl;
}

void testAddingEntityToSpecificLocation() {
    std::cout << "Test 29 - Adding entity directly to a specific location";
    Entity entity(0, "Daniel");
    Environment environment(0, "Earth", 4);
    Location& targetLocation = environment.getGrid()->getFirstLocation();
    environment.addEntityToLocation(entity, targetLocation);
    assert(entity.getLocationId() == targetLocation.getId());
    assert(targetLocation.isEntityPresent(&entity));
    std::cout << " --- " << "Success" << std::endl;
}

void testSettingGridSize() {
    std::cout << "Test 30 - Setting grid size";
    Grid grid(0, 4);
    grid.setSize(6);
    assert(grid.getSize() == 6);
    std::cout << " --- " << "Success" << std::endl;
}

void testAddingLocationToGrid() {
    std::cout << "Test 31 - Adding a location to a grid";
    Grid grid(0, 2);
    size_t initialNumLocations = grid.getLocations().size();
    Location newLocation("extra-location", 10, 10);
    grid.addLocation(newLocation);
    assert(grid.getLocations().size() == initialNumLocations + 1);
    assert(grid.getLocations().back().getId() == "extra-location");
    std::cout << " --- " << "Success" << std::endl;
}

void testRetrievingLocationByCoordinates() {
    std::cout << "Test 32 - Retrieving a location by coordinates";
    Grid grid(0, 5);
    Location& location = grid.getLocationByCoordinates(2, 3);
    assert(location.getX() == 2);
    assert(location.getY() == 3);
    std::cout << " --- " << "Success" << std::endl;
}

void testRetrievingEntitiesFromLocation() {
    std::cout << "Test 33 - Retrieving entities from a location";
    Location location("test-location", 0, 0);
    Entity entity1(0, "Bob");
    Entity entity2(1, "Phil");
    location.addEntity(&entity1);
    location.addEntity(&entity2);
    std::vector<Entity*>& entities = location.getEntities();
    assert(entities.size() == 2);
    assert(entities[0]->getId() == entity1.getId());
    assert(entities[1]->getId() == entity2.getId());
    std::cout << " --- " << "Success" << std::endl;
}

void testRetrievingNumEntitiesFromLocation() {
    std::cout << "Test 34 - Retrieving the number of entities in a location";
    Location location("test-location", 0, 0);
    assert(location.getNumEntities() == 0);
    Entity entity(0, "Daniel");
    location.addEntity(&entity);
    assert(location.getNumEntities() == 1);
    std::cout << " --- " << "Success" << std::endl;
}

void testRemovingLocationFromGrid() {
    std::cout << "Test 35 - Removing a location from a grid";
    Grid grid(0, 2);
    size_t initialNumLocations = grid.getLocations().size();
    Location extraLocation("extra-location", 10, 10);
    grid.addLocation(extraLocation);
    assert(grid.getLocations().size() == initialNumLocations + 1);
    grid.removeLocation(extraLocation);
    assert(grid.getLocations().size() == initialNumLocations);
    for (Location& location : grid.getLocations()) {
        assert(location.getId() != "extra-location");
    }
    std::cout << " --- " << "Success" << std::endl;
}

void testRemovingLocationFromMiddleOfGrid() {
    std::cout << "Test 36 - Removing a location from the middle of a grid";
    Grid grid(0, 2);
    size_t initialNumLocations = grid.getLocations().size();
    std::string middleId = grid.getLocations()[1].getId();
    Location target(middleId, 0, 0);
    grid.removeLocation(target);
    assert(grid.getLocations().size() == initialNumLocations - 1);
    for (Location& location : grid.getLocations()) {
        assert(location.getId() != middleId);
    }
    std::cout << " --- " << "Success" << std::endl;
}

void testRemovingLocationFromGridClearsEntityIds() {
    std::cout << "Test 37 - Removing a location from a grid clears the ids of its entities";
    Grid grid(0, 2);
    Entity entity(0, "Daniel");
    Location& location = grid.getFirstLocation();
    std::string locationId = location.getId();
    grid.addEntityToLocation(entity, location);
    assert(entity.getLocationId() == locationId);
    assert(entity.getGridId() == grid.getId());
    grid.removeLocation(location);
    assert(entity.getLocationId() == "N/S");
    assert(entity.getGridId() == -1);
    assert(grid.isEntityPresent(entity) == false);
    assert(grid.getNumEntities() == 0);
    std::cout << " --- " << "Success" << std::endl;
}

void testRemovingLocationFromGridLeavesOtherEntitiesAlone() {
    std::cout << "Test 38 - Removing a location from a grid leaves entities in other locations alone";
    Grid grid(0, 2);
    Entity removedEntity(0, "Daniel");
    Entity survivingEntity(1, "Michael");
    Location& removedLocation = grid.getLocations()[0];
    Location& survivingLocation = grid.getLocations()[1];
    std::string survivingLocationId = survivingLocation.getId();
    grid.addEntityToLocation(removedEntity, removedLocation);
    grid.addEntityToLocation(survivingEntity, survivingLocation);
    assert(grid.getNumEntities() == 2);
    grid.removeLocation(removedLocation);
    assert(survivingEntity.getLocationId() == survivingLocationId);
    assert(survivingEntity.getGridId() == grid.getId());
    assert(grid.isEntityPresent(survivingEntity) == true);
    assert(grid.getNumEntities() == 1);
    std::cout << " --- " << "Success" << std::endl;
}

void seedRandomNumberGenerator() {
    srand (time (NULL));
}

int main() {
    // seed RNG
    seedRandomNumberGenerator();
    
    // tests
    testPlacingEntityInLocation();
    testRemovingEntityFromLocation();
    testGeneratingGrid();
    testRetrievingFirstLocation();
    testRetrievingLocationAfterModification();
    testPlacingEntityInGrid();
    testRemovingEntityFromGrid();
    testPlacingEntityInEnvironment();
    testRemovingEntityFromEnvironment();
    testPlacingMultipleEntities();
    testResettingEntityLocationId();
    testRetrievingFirstEntity();
    testRetrievingEntityById();
    testRetrievingLocationById();
    testMovingEntityToNewLocation();
    testMovingEntityToNewLocationRepeatedly();
    testMovingMultipleEntitiesToNewLocations();
    testMovingMultipleEntitiesToNewLocationsRepeatedly();
    testMovingEntityUp();
    testMovingEntityRight();
    testMovingEntityDown();
    testMovingEntityLeft();
    testMovingEntityToRandomAdjacentLocation();
    testMovingEntityToRandomAdjacentLocationRepeatedly();
    testMovingMultipleEntitiesToRandomAdjacentLocations();
    testMovingMultipleEntitiesToRandomAdjacentLocationsRepeatedly();
    testSettingEnvironmentName();
    testPrintingEnvironmentInfo();
    testAddingEntityToSpecificLocation();
    testSettingGridSize();
    testAddingLocationToGrid();
    testRetrievingLocationByCoordinates();
    testRetrievingEntitiesFromLocation();
    testRetrievingNumEntitiesFromLocation();
    testRemovingLocationFromGrid();
    testRemovingLocationFromMiddleOfGrid();
    testRemovingLocationFromGridClearsEntityIds();
    testRemovingLocationFromGridLeavesOtherEntitiesAlone();
    return 0;
}