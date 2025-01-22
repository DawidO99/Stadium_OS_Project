# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O0 -g


# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp $(SRC_DIR)/utils/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Executables
EXECUTABLES = manager technician fan

# Default target
all: $(EXECUTABLES)

# Rule to build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Rules for executables
main: $(BUILD_DIR)/main.o $(BUILD_DIR)/utils/ipc_utils.o
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$@ $^

manager: $(BUILD_DIR)/manager.o $(BUILD_DIR)/utils/ipc_utils.o
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$@ $^ -pthread

technician: $(BUILD_DIR)/technician.o $(BUILD_DIR)/utils/ipc_utils.o
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$@ $^ -pthread

fan: $(BUILD_DIR)/fan.o $(BUILD_DIR)/utils/ipc_utils.o
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$@ $^ -pthread

# Clean up
clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/$(EXECUTABLES)

# Phony targets
.PHONY: all clean
