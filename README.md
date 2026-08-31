# environment-lib-cpp
This C++ library provides base classes for creating 2D virtual worlds.

## Classes
Name | Description
----- | -----
Entity | Represents an entity that can exist in a location.
Environment | Represents a virtual environment with an underlying 2D grid of locations that can contain entities.
Grid | Represents a grid of locations.
Location | Represents a location that can contain entities.

Entities exist in locations. Locations exist in grids. Grids exist in environments. Environments serve as the interface for developers to affect the locations and entities within.

## Building

A C++ compiler (`g++`) and `make` are the only prerequisites.

```
make
```

The `Makefile` has a single target, `tests`, which is also the default. It compiles every source file under `src/` — the four classes plus the test suite in `src/tests.cpp` — into a `tests_executable` binary at the repository root. No install, packaging, or shared-library target is defined, so a consuming project compiles the sources in `src/` alongside its own and includes the headers from `src/header/`.

`tests_executable` is listed in `.gitignore` and is never committed.

## Running Tests

The test suite lives in `src/tests.cpp` as a set of `assert()`-based functions registered in `main()`. A failing assertion aborts the process, so a nonzero exit status means the suite failed.

```
bash run_tests.sh
```

`run_tests.sh` is the canonical gate: it deletes any previous binary, runs `make`, and executes the result. This script is expected to pass before a change is merged.

```
bash run_tests_asan.sh
```

`run_tests_asan.sh` is the AddressSanitizer variant. It compiles the same sources with `-fsanitize=address -g` into `tests_executable_asan` and runs it with `ASAN_OPTIONS=detect_leaks=1`, so an unfreed allocation or a memory-safety error fails the run. Running it is additionally expected for any change that affects allocation or object lifetime.

## py_env_lib

This project is based on py_env_lib, the repository for which can be found [here](https://github.com/Preponderous-Software/py_env_lib).

## 📄 License

env-lib-cpp is licensed under the [MIT License](LICENSE).

Copyright © 2022–2025 Daniel McCoy Stephenson. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the “Software”), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies 
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT 
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

---

### Why MIT?
The **MIT License** was chosen for its simplicity and permissiveness, ensuring that developers across industries—whether in games, simulations, or research—can freely adopt and extend **env-lib-cpp**. This fosters collaboration, encourages integration with other languages and tools, and helps establish it as a go-to C++ foundation for environment simulation.

### Open Source Commitment
There are **no plans to move away from open source** for **env-lib-cpp**. The library will remain freely available under an OSI-approved license, and contributions from the community are welcomed.
