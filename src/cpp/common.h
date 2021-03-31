#pragma once

#define DEVELOPMENT_MODE
#define ENABLE_ASSERTS

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#define APP_NAME "eruption"
#define FRAME_LATENCY 3

#include "core/compiler.h"
#include "globals.h"

#include "asserts.h"
#include "logging/log_macro.h"
#include "core/types.h"

template <typename T>
inline T max(T a, T b) { return a < b ? b : a; }
template <typename T, typename... Args>
inline T max(T a, T b, Args... args) { return max(max(a, b), max(args...)); }

template <typename T>
inline T min(T a, T b) { return b < a ? b : a; }
template <typename T, typename... Args>
inline T min(T a, T b, Args... args) { return min(min(a, b), min(args...)); }

#ifdef DEVELOPMENT_MODE
static LiteralString SHADERS_FOLDER = SHADERS_DIR;
#endif
