#pragma once

#include <stdint.h>
using u8    = uint8_t;
using u16   = uint16_t;
using u32   = uint32_t;
using i32   = int32_t;
using i64   = int64_t;
using u64   = uint64_t;

using LiteralString = const char*;

struct uint2 { u32 x; u32 y; };
struct uint3 { u32 x; u32 y; u32 z; };
struct uint4 { u32 x; u32 y; u32 z; u32 w; };

struct float2 { float x; float y; };
struct float3 { float x; float y; float z; };
struct float4 { float x; float y; float z; float w; };

#include <string_view>
#include <vector>
using std::vector;

#include "smallcontainers/SmallVector.h"
#include "smallcontainers/SmallString.h"
using sc::SmallVector;
using sc::SmallString;

#include "flathashmap.h"

#include <memory>
using std::unique_ptr;
using std::shared_ptr;

template <typename T>
constexpr auto toUnderlyingType(T e)
{
    return static_cast<std::underlying_type<ShaderType>::type>(e);
}
