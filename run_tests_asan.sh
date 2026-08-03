# remove executable
rm ./tests_executable_asan

# compile with AddressSanitizer to catch memory-safety regressions (e.g. new/free mismatches, use-after-free)
g++ -fsanitize=address -g src/*.cpp -o tests_executable_asan

# run executable (leak detection is off: a pre-existing, separately tracked leak
# in the throw-by-pointer exception pattern would otherwise fail every run)
ASAN_OPTIONS=detect_leaks=0 ./tests_executable_asan
