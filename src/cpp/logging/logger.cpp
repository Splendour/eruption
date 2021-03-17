#include "common.h"

#include "logger.h"

#pragma warning( push )
#pragma warning( disable : 4244)
#include <spdlog/spdlog.h>

#if defined(_WIN32)
#include <spdlog/sinks/msvc_sink.h>
#else
#include <spdlog/sinks/stdout_sinks.h>
#endif

#pragma warning( pop )

Logger::Logger()
{
#ifdef _WIN32
    auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
    auto sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
#endif
    m_console = std::make_shared<spdlog::logger>("DebugLogger", sink);

    // https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
    m_console->set_pattern("[%l][%s:%#] %v");

    m_console->set_level(spdlog::level::trace);

    spdlog::set_default_logger(m_console);
}

void Logger::flush()
{
    m_console->flush();
}


