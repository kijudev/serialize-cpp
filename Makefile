CXX      := clang++
CXXOPT   := -O1 -g
CXXFLAGS := -std=c++23 -Wall -Wextra -Iinclude

EXAMPLES      := $(wildcard examples/*/main.cpp)
EXAMPLES_BIN  := $(patsubst examples/%/main.cpp,build/%,$(EXAMPLES))

TESTS     := $(wildcard tests/*.cpp)
TESTS_BIN := $(patsubst tests/%.cpp,build/tests/%,$(TESTS))

.PHONY: all examples tests clean compile-commands

all: examples tests compile-commands

examples: $(EXAMPLES_BIN)

tests: $(TESTS_BIN)

build/%: examples/%/main.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(CXXOPT) $< -o $@

build/tests/%: tests/%.cpp
	mkdir -p build/tests
	$(CXX) $(CXXFLAGS) $(CXXOPT) $< -o $@

compile-commands:
	bear -- $(MAKE) examples tests

clean:
	rm -rf build
	rm -f compile_commands.json
