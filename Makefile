# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp $(SRC_DIR)/utils/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Executables
EXECUTABLES = manager technician fan main

# Default target
all: $(EXECUTABLES)

# Rule for building object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Rules for executables
manager: $(BUILD_DIR)/manager.o $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$@ $(BUILD_DIR)/manager.o

technician: $(BUILD_DIR)/technician.o $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$@ $(BUILD_DIR)/technician.o

fan: $(BUILD_DIR)/fan.o $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$@ $(BUILD_DIR)/fan.o

main: $(BUILD_DIR)/main.o $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$@ $(BUILD_DIR)/main.o

# Clean
clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/$(EXECUTABLES)

# Phony targets
.PHONY: all clean
