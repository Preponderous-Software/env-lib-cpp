all: tests

tests: src/tests.cpp
	@echo "---"
	@echo "Compiling tests.cpp"

	g++ src/*.cpp /Catch2/src/catch2/*.cpp -o tests_executable

	@echo "Finished compiling tests.cpp"