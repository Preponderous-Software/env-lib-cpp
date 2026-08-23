# remove executable
rm ./tests_executable_asan

# compile with AddressSanitizer to catch memory-safety regressions (e.g. new/free mismatches, use-after-free)
g++ -fsanitize=address -g src/*.cpp -o tests_executable_asan

# run executable (leak detection is on, so an unfreed allocation fails the run)
ASAN_OPTIONS=detect_leaks=1 ./tests_executable_asan
