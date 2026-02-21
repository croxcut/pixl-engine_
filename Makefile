# Compiler and flags
CC = gcc
CXX = g++
CFLAGS = -Iexternal/gl/glad/include -Iexternal/gl -Ipixl -Wall -O2
CXXFLAGS = $(CFLAGS)
LDFLAGS = 

# Directories (base directories to search)
BASE_DIRS = pixl core drivers external scene server game misc
OBJ_DIR = obj
BIN_DIR = bin
WINDOWS_TARGET = $(BIN_DIR)/pixl-windowsnt
LINUX_TARGET   = $(BIN_DIR)/pixl-linux64

# Recursive function to get all subdirectories
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(d))

# Get all subdirectories recursively from base directories
ALL_SUBDIRS = $(filter-out %/.git %/.svn %/CVS,$(foreach dir,$(BASE_DIRS),$(call rwildcard,$(dir)/,)))

# Combine base dirs with all subdirs and remove duplicates
PIXL_DIRS = $(sort $(BASE_DIRS) $(ALL_SUBDIRS))

# Source files
define rwildcard_src
$(wildcard $1$2) $(foreach d,$(wildcard $1*/),$(call rwildcard_src,$d,$2))
endef

C_SOURCES  := $(foreach dir,$(PIXL_DIRS),$(call rwildcard_src,$(dir)/,*.c))
CPP_SOURCES:= $(foreach dir,$(PIXL_DIRS),$(call rwildcard_src,$(dir)/,*.cpp))

WINDOWS_OBJS = $(patsubst %.c,$(OBJ_DIR)/windows/%.o,$(C_SOURCES)) \
               $(patsubst %.cpp,$(OBJ_DIR)/windows/%.o,$(CPP_SOURCES))
LINUX_OBJS   = $(patsubst %.c,$(OBJ_DIR)/linux/%.o,$(C_SOURCES)) \
               $(patsubst %.cpp,$(OBJ_DIR)/linux/%.o,$(CPP_SOURCES))

# Windows libraries
WINDOWS_LIBS = external/gl/glfw/lib/libglfw3.a -lgdi32 -luser32 -lopengl32 -limm32 -lole32 -loleaut32 -luuid -lwinmm -lshell32
# Linux libraries
LINUX_LIBS = -lglfw -ldl -lGL -lm -lpthread

# Ensure obj/bin directories exist
$(shell mkdir -p $(OBJ_DIR)/windows $(OBJ_DIR)/linux $(BIN_DIR))

# Default target: just list available builds
all:
	@echo "Available targets:"
	@echo "  make windows_nt   # Build for Windows (MinGW)"
	@echo "  make linux64     # Build for Linux 64-bit"

# Windows build
windows_nt: $(WINDOWS_TARGET)

$(WINDOWS_TARGET): $(WINDOWS_OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(WINDOWS_LIBS)

# Compile C files for Windows
$(OBJ_DIR)/windows/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ files for Windows
$(OBJ_DIR)/windows/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Linux build
linux64: $(LINUX_TARGET)

$(LINUX_TARGET): $(LINUX_OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LINUX_LIBS)

# Compile C files for Linux
$(OBJ_DIR)/linux/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ files for Linux
$(OBJ_DIR)/linux/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean windows_nt linux64