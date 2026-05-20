// Copyright 2025 Accenture.

#include "bsp/timer/SystemTimer.h"
#include "zephyr/kernel.h"

#include <etl/chrono.h>

uint32_t getSystemTimeUs32Bit(void) 
{
    return k_cyc_to_us_floor32(k_cycle_get_64());
}

uint32_t getSystemTimeMs32Bit(void) 
{
    return k_cyc_to_ms_floor32(k_cycle_get_64());
}

uint64_t getSystemTimeNs(void)
{
    return k_cyc_to_ns_floor64(k_cycle_get_64());
}

uint32_t getSystemTicks32Bit(void)
{
    return k_cycle_get_32();
}

uint64_t systemTicksToTimeNs(uint64_t ticks)
{
    return k_cyc_to_ns_floor64(ticks);
}

uint64_t systemTicksToTimeUs(uint64_t ticks)
{
    return k_cyc_to_us_floor64(ticks);
}
