// A small example used to record docs/images/random-walk.gif.
// Ten entities are placed in an 8x8 environment and moved to random adjacent
// locations; after each step the environment prints itself with
// printConsoleRepresentation(). Every frame of the GIF is that output.
//
// Built and recorded by docs/capture.sh; not part of the library or the tests.
#include <cstdlib>
#include <iostream>
#include <list>
#include "../src/header/environment.h"

using envlibcpp::Entity;
using envlibcpp::Environment;

int main(int argc, char* argv[]) {
    srand(argc > 1 ? atoi(argv[1]) : 7);
    std::list<Entity> entities;  // a list keeps each entity at a stable address
    for (int i = 0; i < 10; i++) {
        entities.emplace_back(i, "walker");
    }
    Environment environment(0, "Earth", 8);
    for (Entity& entity : entities) {
        environment.addEntity(entity);
    }
    for (int step = 0; step <= 20; step++) {
        std::cout << "step " << step << "\n";
        environment.printConsoleRepresentation();
        for (Entity& entity : entities) {
            environment.moveEntityToRandomAdjacentLocation(entity.getId());
        }
    }
}
