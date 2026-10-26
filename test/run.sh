#!/bin/sh
# Builds and runs the unit tests on Linux (g++, Qt6Core, ASan/UBSan).
set -e
cd "$(dirname "$0")/.."
mkdir -p build_test
g++ -std=c++17 -g -Wall -Wextra -fsanitize=address,undefined -fno-sanitize-recover=all -fPIC \
	-Isrc $(pkg-config --cflags Qt6Core) \
	test/core_test.cpp src/core/rules.cpp src/core/logic.cpp \
	$(pkg-config --libs Qt6Core) -o build_test/core_test
./build_test/core_test
