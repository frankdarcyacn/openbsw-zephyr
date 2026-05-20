// Copyright 2021 Accenture.

#include "async/FutureSupport.h"

#include <async/AsyncBinding.h>

namespace async
{
static uint32_t const FUTURE_SUPPORT_BITS_TO_WAIT = 0x01U;

FutureSupport::FutureSupport(ContextType const context) : _context(context)
{
    k_event_init(&_eventObject);
}

void FutureSupport::wait()
{
    while (true)
    {
        if (k_event_wait(
                &_eventObject,
                FUTURE_SUPPORT_BITS_TO_WAIT,
                false /* don't reset events */,
                K_USEC(AsyncBinding::WAIT_EVENTS_US))
            == FUTURE_SUPPORT_BITS_TO_WAIT)
        {
            k_event_clear(&_eventObject, FUTURE_SUPPORT_BITS_TO_WAIT);
            return;
        }
    }
}

void FutureSupport::notify() { k_event_set(&_eventObject, FUTURE_SUPPORT_BITS_TO_WAIT); }

void FutureSupport::assertTaskContext()
{
    ETL_ASSERT(verifyTaskContext(),
        ETL_ERROR_GENERIC("Wrong task context"));
}

bool FutureSupport::verifyTaskContext()
{
    return _context == AsyncBinding::AdapterType::getCurrentTaskContext();
}

} // namespace async
