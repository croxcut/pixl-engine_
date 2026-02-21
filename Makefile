# Compiler and flags
CC = gcc
CXX = g++
CFLAGS = -Iexternal/gl/glad/include -Iexternal/gl -Wall -O2
CXXFLAGS = $(CFLAGS)
LDFLAGS = -static-libgcc -static-libstdc++
LIBS = external/gl/glfw/lib/libglfw3.a

# Directories
SRC_DIRS = src external/gl/glad/src
OBJ_DIR = obj
BIN_DIR = bin
TARGET = $(BIN_DIR)/pixl-engine

# Find all source files
C_SOURCES = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
CPP_SOURCES = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
OBJ_FILES = $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SOURCES)) $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CPP_SOURCES))

# Ensure obj and bin directories exist
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# Default target
all: $(TARGET)

# Link target
$(TARGET): $(OBJ_FILES)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LIBS)

# Compile C files
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ files
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean