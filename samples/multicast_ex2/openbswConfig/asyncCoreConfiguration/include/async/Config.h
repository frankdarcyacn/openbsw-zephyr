// Copyright 2023 Accenture.

#pragma once

#define ASYNC_CONFIG_TICK_IN_US        (100U)
#define ASYNC_CONFIG_NESTED_INTERRUPTS (1)

enum
{
    // highest priority task has lowest number
    TASK_SYSADMIN,
    TASK_CAN,
    TASK_DEMO,
    TASK_UDS,
    TASK_BACKGROUND,
    // --------------------
    ASYNC_CONFIG_TASK_COUNT,
};

enum
{
    ISR_GROUP_TEST = 0,
    // ------------
    ISR_GROUP_CAN,
    ISR_GROUP_ETHERNET,

    ISR_GROUP_COUNT,
};
