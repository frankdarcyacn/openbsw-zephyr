
#include "OpenBSWZephyrLogger.h"
#include <stdio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(openbsw, 4);

bool OpenBSWZephyrLogger::isEnabled(uint8_t componentIndex, Level level) const { return true; }

Level OpenBSWZephyrLogger::getLevel(uint8_t componentIndex) const { return LEVEL_DEBUG; }

LevelInfo OpenBSWZephyrLogger::getLevelInfo(Level level) const
{ 
    return LevelInfo(LevelInfo::getDefaultTable() + level);
}

ComponentInfo OpenBSWZephyrLogger::getComponentInfo(uint8_t componentIndex) const
{
    return ComponentInfo{};
}

void OpenBSWZephyrLogger::logOutput(
    ComponentInfo const& componentInfo,
    LevelInfo const& levelInfo,
    char const* str,
    va_list ap)
{
    char buf[256];
    vsnprintf(buf, sizeof(buf), str, ap);
    switch (levelInfo.getLevel())
    {
    case LEVEL_CRITICAL:
        LOG_ERR("%s", buf);
        break;
    case LEVEL_ERROR:
        LOG_ERR("%s", buf);
        break;
    case LEVEL_WARN:
        LOG_WRN("%s", buf);
        break;
    case LEVEL_INFO:
        LOG_INF("%s", buf);
        break;
    case LEVEL_DEBUG:
        LOG_DBG("%s", buf);
        break;
    default:
        break;
    }
}
