// Copyright 2023 Accenture.

#include "lifecycle/console/LifecycleControlCommand.h"

#include <async/Async.h>
#include <lifecycle/ILifecycleManager.h>

#include <etl/error_handler.h>

namespace
{
enum Id
{
    ID_REBOOT,
    ID_POWEROFF,
    ID_UDEF,
    ID_PABT,
    ID_DABT,
    ID_ASSERT
};

}

namespace lifecycle
{
DEFINE_COMMAND_GROUP_GET_INFO_BEGIN(LifecycleControlCommand, "lc", "lifecycle command")
COMMAND_GROUP_COMMAND(ID_REBOOT, "reboot", "reboot the system")
COMMAND_GROUP_COMMAND(ID_POWEROFF, "poweroff", "poweroff the system")
COMMAND_GROUP_COMMAND(ID_UDEF, "udef", "forces an undefined instruction exception")
COMMAND_GROUP_COMMAND(ID_PABT, "pabt", "forces a prefetch abort exception")
COMMAND_GROUP_COMMAND(ID_DABT, "dabt", "forces a data abort exception")
COMMAND_GROUP_COMMAND(ID_ASSERT, "assert", "forces an assert")
DEFINE_COMMAND_GROUP_GET_INFO_END

LifecycleControlCommand::LifecycleControlCommand(ILifecycleManager& lifecycleManager)
: _lifecycleManager(lifecycleManager)
{}

void LifecycleControlCommand::executeCommand(::util::command::CommandContext& /*context*/, uint8_t idx)
{
    switch (idx)
    {
        case ID_REBOOT:
        {
            _lifecycleManager.transitionToLevel(0);
            break;
        }
        case ID_POWEROFF:
        {
            _lifecycleManager.transitionToLevel(0);
            break;
        }
        case ID_UDEF:
        {
            break;
        }
        case ID_PABT:
        {
            break;
        }
        case ID_DABT:
        {
            break;
        }
        case ID_ASSERT:
        {
            ETL_ASSERT(false, ETL_ERROR_GENERIC("executeCommand(ID_ASSERT)"));
            break;
        }
        default:
        {
            break;
        }
    }
}

} // namespace lifecycle
