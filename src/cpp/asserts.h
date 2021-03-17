#pragma once

#include "globals.h"
#include "logging/logger.h"

namespace
{
    template<typename... Args>
    void logAssert(spdlog::source_loc _srcInfo, fmt::basic_string_view<char> _fmt, const Args&... _args)
    {
        globals::getRef<Logger>().error(_srcInfo, _fmt, _args...);
    }

    template<typename... Args>
    void criticalAssert(bool _expression, spdlog::source_loc _srcInfo, fmt::basic_string_view<char> _fmt, const Args&... _args)
    {
        if (_expression)
            return;

        logAssert(_srcInfo, _fmt, _args...);
        BREAKPOINT();
    }

    template<typename... Args>
    void pedanticAssert(bool _expression, spdlog::source_loc _srcInfo, fmt::basic_string_view<char> _fmt, const Args&... _args)
    {
#ifndef ENABLE_ASSERTS
        return;
#else
        if (_expression)
            return;

        logAssert(_srcInfo, _fmt, _args...);
        BREAKPOINT();
#endif
    }
}

#define ASSERT_TRUE(expr) pedanticAssert((expr), spdlog::source_loc { __FILE__, __LINE__, SPDLOG_FUNCTION }, #expr)
#define VERIFY_TRUE(expr) criticalAssert((expr), spdlog::source_loc { __FILE__, __LINE__, SPDLOG_FUNCTION }, #expr)

#define ASSERT_TRUE_MSG(expr, ...) pedanticAssert((expr), spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#define VERIFY_TRUE_MSG(expr, ...) criticalAssert((expr), spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)

#define ASSERT_NOT_IMPLEMENTED() criticalAssert(false, spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, "Not implemented")

#define ThrowIfFailed(hr) VERIFY_TRUE(hr >= 0)