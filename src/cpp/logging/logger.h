#pragma once

#define FMT_EXCEPTIONS 0
#define SPDLOG_NO_EXCEPTIONS
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/common.h>
#include <spdlog/logger.h>

class Logger
{
public:
    Logger();

    template<typename... Args>
    void print(spdlog::source_loc _srcInfo, fmt::basic_string_view<char> _fmt, const Args&... _args) const
    {
        m_console->log(_srcInfo, spdlog::level::info, _fmt, _args...);
    }

    template<typename... Args>
    void error(spdlog::source_loc _srcInfo, fmt::basic_string_view<char> _fmt, const Args&... _args) const
    {
        m_console->log(_srcInfo, spdlog::level::critical, _fmt, _args...);
    }

    void flush();

private:
    std::shared_ptr<spdlog::logger> m_console;
};