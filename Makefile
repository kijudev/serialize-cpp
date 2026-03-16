CXX         := clang++
CXXOPT      := -O1 -g
PKG_CONFIG  := pkg-config
YYJSON_CFLAGS := $(shell $(PKG_CONFIG) --cflags yyjson)
YYJSON_LIBS   := $(shell $(PKG_CONFIG) --libs yyjson)
CXXFLAGS    := -std=c++23 -Wall -Wextra -Iinclude $(YYJSON_CFLAGS)
LDFLAGS     := $(YYJSON_LIBS)

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
	$(CXX) $(CXXFLAGS) $(CXXOPT) $< -o $@ $(LDFLAGS)

build/tests/%: tests/%.cpp
	mkdir -p build/tests
	$(CXX) $(CXXFLAGS) $(CXXOPT) $< -o $@ $(LDFLAGS)

compile-commands:
	bear -- $(MAKE) examples tests

clean:
	rm -rf build
	rm -f compile_commands.json
