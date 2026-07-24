#pragma once

#include <util/logger/IComponentMapping.h>
#include <util/logger/ILoggerOutput.h>

using namespace util::logger;

class OpenBSWZephyrLogger
: public IComponentMapping
, public ILoggerOutput
{
    bool isEnabled(uint8_t componentIndex, Level level) const override;

    Level getLevel(uint8_t componentIndex) const override;

    LevelInfo getLevelInfo(Level level) const override;

    ComponentInfo getComponentInfo(uint8_t componentIndex) const override;

    void logOutput(
        ComponentInfo const& componentInfo,
        LevelInfo const& levelInfo,
        char const* str,
        va_list ap) override;
};
