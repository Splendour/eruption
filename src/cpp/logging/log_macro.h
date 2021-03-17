#pragma once

#include "globals.h"
#include "logger.h"

#define logInfo(...) globals::getRef<Logger>().print(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#define logError(...) globals::getRef<Logger>().error(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
