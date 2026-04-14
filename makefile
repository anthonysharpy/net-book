# ==================== HOW TO USE ==================== #
#
# Release build: sudo make pgo-gen -> make release
# Profile build: sudo make pgo-gen -> make profile
# Run tests: make test
# Normal build (not recommended): make
#
# NOTE: profile build may require some extra software.
#
# ==================================================== #


# ===== Configuration ===== #

SUPPRESSED_WARNINGS = -Wno-interference-size
COMPILER = g++-12
BINARY_NAME = netbook
TESTS_BINARY_NAME = unit_tests
SRC_DIR = src
GOOGLE_TEST_INCLUDE_DIR = third_party/googletest/googletest/include
BUILD_DIR = build
TESTS_DIR = tests

BASE_COMPILE_FLAGS = -DNDEBUG -std=c++23 -march=native -flto=auto -Ofast -Wall -Wextra -Wpedantic -pipe -MMD -MP -I$(SRC_DIR) $(SUPPRESSED_WARNINGS) $(shell pkg-config --cflags libdpdk | sed 's/-I/-isystem /g')
BASE_LINK_FLAGS = -flto $(shell pkg-config --libs libdpdk)
TEST_COMPILE_FLAGS = -std=c++23 -O0 -g -Wall -Wextra -march=native -Wpedantic -pipe -MMD -MP -I$(SRC_DIR) -I$(GOOGLE_TEST_INCLUDE_DIR) $(SUPPRESSED_WARNINGS)
TEST_LINK_FLAGS =
# Use -Ofast optimisation. More risky, but should work assuming the code is correct.
RELEASE_COMPILE_FLAGS = $(BASE_COMPILE_FLAGS) -fprofile-use=pgodata -fprofile-correction
RELEASE_LINK_FLAGS = $(BASE_LINK_FLAGS) -fprofile-use=pgodata
PROFILE_COMPILE_FLAGS  = $(BASE_COMPILE_FLAGS) -g -fno-omit-frame-pointer
PROFILE_LINK_FLAGS = $(BASE_LINK_FLAGS)
PGO_COMPILE_FLAGS = $(BASE_COMPILE_FLAGS) -fprofile-generate=pgodata
PGO_LINK_FLAGS = $(BASE_LINK_FLAGS) -fprofile-generate=pgodata
BENCHMARK_COMPILE_FLAGS = $(BASE_COMPILE_FLAGS)
BENCHMARK_LINK_FLAGS = $(BASE_LINK_FLAGS)

# ===== Vars ===== #

CPP_FILES = $(shell find $(SRC_DIR) -name '*.cpp')
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_FILES))
DEPENDENCY_FILES = $(OBJ_FILES:.o=.d)
COMPILE_FLAGS = $(BASE_COMPILE_FLAGS)
LINK_FLAGS = $(BASE_LINK_FLAGS)
MAIN_CPP_FILE = $(SRC_DIR)/main.cpp

# Files containing our actual tests
TEST_CPP_FILES = $(shell find $(TESTS_DIR) -name '*.cpp')
TEST_OBJ_FILES = $(patsubst $(TESTS_DIR)/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_CPP_FILES))
# Files from the project that need building in test mode
NON_MAIN_CPP_FILES = $(filter-out $(MAIN_CPP_FILE),$(CPP_FILES))
TEST_CORE_OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/tests/src/%.o,$(NON_MAIN_CPP_FILES))
TEST_DEPENDENCY_FILES = $(TEST_CORE_OBJ_FILES:.o=.d) $(TEST_OBJ_FILES:.o=.d)

GTEST_SRC_DIR = third_party/googletest
GTEST_BUILD_DIR = $(BUILD_DIR)/googletest
GTEST_CACHE = $(GTEST_BUILD_DIR)/CMakeCache.txt
GTEST_BUILT = $(GTEST_BUILD_DIR)/.built

# ===== Build ===== #

.PHONY: clean profile pgo-gen release test benchmark

all: $(BINARY_NAME)

# Build our binary
$(BINARY_NAME): $(OBJ_FILES)
	$(COMPILER) $^ -o $@ $(LINK_FLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(COMPILER) $(COMPILE_FLAGS)  -c $< -o $@

-include $(DEPENDENCY_FILES)

# ===== Build (Release) ===== #

release: pgo-gen
	$(MAKE) all COMPILE_FLAGS="$(RELEASE_COMPILE_FLAGS)" LINK_FLAGS="$(RELEASE_LINK_FLAGS)"

# ===== Performance-Guided Optimisation ===== #

pgo-gen: clean
	rm -rf pgodata
	$(MAKE) all COMPILE_FLAGS="$(PGO_COMPILE_FLAGS)" LINK_FLAGS="$(PGO_LINK_FLAGS)"
	./$(BINARY_NAME) --runtime=10
	$(MAKE) clean

# ===== Profile ===== #

profile: clean
	$(MAKE) all COMPILE_FLAGS="$(PROFILE_COMPILE_FLAGS)" LINK_FLAGS="$(PROFILE_LINK_FLAGS)"
	sudo perf record -e cycles,cache-references,LLC-load-misses --sample-cpu -F 16000 -g -- ./$(BINARY_NAME) --runtime=10
	sudo hotspot perf.data

# ===== Benchmark ===== #

benchmark: clean
	$(MAKE) all COMPILE_FLAGS="$(BENCHMARK_COMPILE_FLAGS)" LINK_FLAGS="$(BENCHMARK_LINK_FLAGS)"

# ===== Run/Build Tests ===== #

test: $(TESTS_BINARY_NAME)
	./$(TESTS_BINARY_NAME)

# Configure GoogleTest
$(GTEST_SRC_DIR)/CMakeLists.txt:
	git submodule update --init --recursive $(GTEST_SRC_DIR)

$(GTEST_CACHE): $(GTEST_SRC_DIR)/CMakeLists.txt
	mkdir -p $(GTEST_BUILD_DIR)
	cmake -S $(GTEST_SRC_DIR) -B $(GTEST_BUILD_DIR) -DCMAKE_CXX_COMPILER=$(COMPILER)

# Build GoogleTest
$(GTEST_BUILT): $(GTEST_CACHE)
	cmake --build $(GTEST_BUILD_DIR)
	touch $@

# Build test files
$(BUILD_DIR)/tests/%.o: $(TESTS_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(COMPILER) $(TEST_COMPILE_FLAGS) -c $< -o $@

$(BUILD_DIR)/tests/src/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(COMPILER) $(TEST_COMPILE_FLAGS) -c $< -o $@

$(TESTS_BINARY_NAME): $(GTEST_BUILT) $(TEST_CORE_OBJ_FILES) $(TEST_OBJ_FILES)
	$(COMPILER) -o $@ $(TEST_CORE_OBJ_FILES) $(TEST_OBJ_FILES) \
		$(GTEST_BUILD_DIR)/lib/libgtest_main.a \
		$(GTEST_BUILD_DIR)/lib/libgtest.a \
		$(TEST_LINK_FLAGS) -pthread

-include $(TEST_DEPENDENCY_FILES)

# ===== Clean ===== #

clean:
	rm -rf $(BUILD_DIR) $(BINARY_NAME) $(TESTS_BINARY_NAME)

