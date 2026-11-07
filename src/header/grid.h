#ifndef Grid_h
#define Grid_h

#include <iostream>
#include <vector>

#include "location.h"

namespace envlibcpp {

    // @author Daniel McCoy Stephenson
    // @since August 26th, 2022
    class Grid {
        public:
            Grid() = delete;
            Grid(int id, int gridSize);
            int getId();
            std::vector<Location>& getLocations();
            Location& getFirstLocation();
            Location& getLocationByCoordinates(int x, int y);
            int getSize();
            int getNumEntities();
            void setId(int newId);
            void setSize(int newSize);
            void addLocation(Location& location);
            void removeLocation(Location& location);
            void addEntity(Entity& entity);
            bool addEntityToLocation(Entity& entity, Location& location);
            void removeEntity(Entity& entity);
            bool isEntityPresent(Entity& entity);
            Location& getLocation(std::string locationId);
            Location& getRandomLocation();
        private:
            int id = -1;
            int size = -1;
            std::vector<Location> locations;

            void generateLocations();
            void updateIdsOfDetachedEntity(Entity* entity, std::string removedLocationId);
    };
}

#endif