#pragma once

// Type used for size and capacity
#define SC_SIZE_TYPE uint32_t
//#define SC_SIZE_TYPE size_t


// How much capacity will increase when full
#define SC_CAPACITY_STEP 2


#define SC_ASSERT ASSERT_TRUE


// Change in case of name collision
#define SC_NAMESPACE sc
#define SC_NAMESPACE_BEGIN namespace SC_NAMESPACE {
#define SC_NAMESPACE_END }


// Compiler Hints
#define SC_HINT_LIKELY
#define SC_HINT_UNLIKELY
// Available in C++20
//#define SC_HINT_LIKELY [[likely]]
//#define SC_HINT_UNLIKELY [[unlikely]]
