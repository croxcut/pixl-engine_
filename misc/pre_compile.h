//misc/pre_compile.h
#ifndef PRE_COMPILE_H
#define PRE_COMPILE_H

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

// OpenGL / Windowing
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// C++ standard library :)*
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>

// C standard library :)*
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <ctime>

#endif