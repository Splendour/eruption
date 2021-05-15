#pragma once

#ifdef _MSC_VER
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE __attribute__((always_inline))
#endif

#define UNUSED_VAR(x) do { (void)(x); } while (0)


#ifdef _MSC_VER
#define BREAKPOINT __debugbreak
#else
#define BREAKPOINT __builtin_debugtrap
#endif

#define STRINGIFY(x) #x
#define ALIGNED_TYPE(alignment) _declspec(align(alignment))
#define ALIGNED_TYPE16  ALIGNED_TYPE(16)
#define ALIGNED_TYPE256 ALIGNED_TYPE(256)

#define DISCARD(x) static_cast<void>(x)

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

#pragma warning(disable:4201) // anon struct/union