#ifndef Entity_h
#define Entity_h

#include <string>

namespace envlibcpp {

    // @author Daniel McCoy Stephenson
    // @since August 26th, 2022
    class Entity {
        public:
            Entity(int id, std::string entityName);
            int getId();
            std::string getName();
            int getEnvironmentId();
            int getGridId();
            std::string getLocationId();
            void setName(std::string newName);
            void setEnvironmentId(int id);
            void setGridId(int id);
            void setLocationId(std::string id);
            void resetLocationId();
            void setId(int id);
        private:
            int id = -1;
            std::string name = "";
            int environmentId = -1;
            int gridId = -1;
            std::string locationId = "";
    };
}

#endif