// Copyright 2021 Accenture.

/**
 * \ingroup async
 */
#pragma once
 
#include "async/StaticRunnable.h"
#include "async/TaskContext.h"

#include "zephyr/kernel.h"

#include <etl/array.h>
#include <etl/error_handler.h>

namespace async
{
namespace internal
{
template<bool HasNestedInterrupts = (ASYNC_CONFIG_NESTED_INTERRUPTS != 0)>
class NestedInterruptLock : public LockType
{};

template<>
class NestedInterruptLock<false>
{};
} // namespace internal

template<class Binding>
class ZephyrAdapter
{
public:
    static size_t const TASK_COUNT     = Binding::TASK_COUNT;
    static size_t const WAIT_EVENTS_US = Binding::WAIT_EVENTS_US;

    using TaskContextType  = TaskContext<ZephyrAdapter>;
    using TaskFunctionType = typename TaskContextType::TaskFunctionType;
    using StackUsage       = typename TaskContextType::StackUsage;

    struct TaskConfigType
    {};

private:
    template<ContextType Context, size_t StackSize>
    class TaskImpl
    {
    protected:
        ~TaskImpl() = default;

    public:
        TaskImpl(char const* name, TaskFunctionType taskFunction, k_thread_stack_t* stack);

    private:
        k_thread _task;
        k_timer _timer;
    };

public:
    template<ContextType Context, size_t StackSize>
    struct Task : public TaskImpl<Context, StackSize>
    {
        explicit Task(char const* name, k_thread_stack_t* stack);

        Task(char const* name, TaskFunctionType taskFunction, k_thread_stack_t* stack);
    };

    static char const* getTaskName(size_t taskIdx);

    static ContextType getCurrentTaskContext();

    static void init();

    static void run();

    static bool getStackUsage(size_t taskIdx, StackUsage& stackUsage);

    static void execute(ContextType context, RunnableType& runnable);

    static void schedule(
        ContextType context,
        RunnableType& runnable,
        TimeoutType& timeout,
        uint32_t delay,
        TimeUnitType unit);

    static void scheduleAtFixedRate(
        ContextType context,
        RunnableType& runnable,
        TimeoutType& timeout,
        uint32_t delay,
        TimeUnitType unit);

    static void cancel(TimeoutType& timeout);

private:
    struct TaskInitializer : public StaticRunnable<TaskInitializer>
    {
        TaskInitializer(
            ContextType context,
            char const* name,
            k_thread& task,
            k_timer& timer,
            k_thread_stack_t* stack,
            size_t stackSize,
            TaskFunctionType taskFunction);

        void execute();

        k_thread_stack_t* _stack;
        size_t _stackSize;
        TaskFunctionType _taskFunction;
        k_thread& _task;
        k_timer& _timer;
        char const* _name;
        ContextType _context;
    };

    static void initTask(TaskInitializer& initializer);

    static void initTask(ContextType context, char const* name, k_timer& timer);

    static void TaskFunction(void* param);

    static ::etl::array<TaskContextType, TASK_COUNT> _taskContexts;
};

/**
 * Inline implementations.
 */
template<class Binding>
::etl::array<typename ZephyrAdapter<Binding>::TaskContextType, ZephyrAdapter<Binding>::TASK_COUNT>
    ZephyrAdapter<Binding>::_taskContexts;

template<class Binding>
inline char const* ZephyrAdapter<Binding>::getTaskName(size_t const taskIdx)
{
    return _taskContexts[taskIdx].getName();
}

template<class Binding>
ContextType ZephyrAdapter<Binding>::getCurrentTaskContext()
{
    if(k_is_in_isr())
    {
        return CONTEXT_INVALID;
    }
    return static_cast<ContextType>(k_thread_priority_get(k_current_get()) - 1);
}

template<class Binding>
void ZephyrAdapter<Binding>::initTask(TaskInitializer& initializer)
{
    ContextType const context = initializer._context;
    initTask(context, initializer._name, initializer._timer);
    TaskContextType& taskContext = _taskContexts[static_cast<size_t>(context)];
    taskContext.createTask(
        context,
        initializer._task,
        initializer._name,
        // assume context IDs start with 0, 1 is the highest priority
        static_cast<int>(context) + 1,
        initializer._stack,
        initializer._stackSize,
        initializer._taskFunction);
}

template<class Binding>
void ZephyrAdapter<Binding>::initTask(
    ContextType const context, char const* const name, k_timer& timer)
{
    TaskContextType& taskContext = _taskContexts[static_cast<size_t>(context)];
    taskContext.createTimer(timer, name);
}

template<class Binding>
void ZephyrAdapter<Binding>::init()
{
    TaskInitializer::run();
}

template<class Binding>
void ZephyrAdapter<Binding>::run()
{
    for (size_t i = 0; i < _taskContexts.size(); i++)
    {
        _taskContexts[i].startTask();
    }
}

template<class Binding>
bool ZephyrAdapter<Binding>::getStackUsage(size_t const taskIdx, StackUsage& stackUsage)
{
    if (taskIdx < TASK_COUNT)
    {
        return _taskContexts[taskIdx].getStackUsage(stackUsage);
    }
    return false;
}

template<class Binding>
inline void ZephyrAdapter<Binding>::execute(ContextType const context, RunnableType& runnable)
{
    _taskContexts[static_cast<size_t>(context)].execute(runnable);
}

template<class Binding>
inline void ZephyrAdapter<Binding>::schedule(
    ContextType const context,
    RunnableType& runnable,
    TimeoutType& timeout,
    uint32_t const delay,
    TimeUnitType const unit)
{
    _taskContexts[static_cast<size_t>(context)].schedule(runnable, timeout, delay, unit);
}

template<class Binding>
inline void ZephyrAdapter<Binding>::scheduleAtFixedRate(
    ContextType const context,
    RunnableType& runnable,
    TimeoutType& timeout,
    uint32_t const delay,
    TimeUnitType const unit)
{
    _taskContexts[static_cast<size_t>(context)].scheduleAtFixedRate(runnable, timeout, delay, unit);
}

template<class Binding>
inline void ZephyrAdapter<Binding>::cancel(TimeoutType& timeout)
{
    LockType const lock;
    ContextType const context = timeout._context;
    if (context != CONTEXT_INVALID)
    {
        timeout._context = CONTEXT_INVALID;
        _taskContexts[static_cast<size_t>(context)].cancel(timeout);
    }
}

template<class Binding>
ZephyrAdapter<Binding>::TaskInitializer::TaskInitializer(
    ContextType const context,
    char const* const name,
    k_thread& task,
    k_timer& timer,
    k_thread_stack_t* stack,
    size_t stackSize,
    TaskFunctionType const taskFunction)
: _stack(stack)
, _stackSize(stackSize)
, _taskFunction(taskFunction)
, _task(task)
, _timer(timer)
, _name(name)
, _context(context)
{}

template<class Binding>
void ZephyrAdapter<Binding>::TaskInitializer::execute()
{
    ZephyrAdapter<Binding>::initTask(*this);
}

template<class Binding>
template<ContextType Context, size_t StackSize>
ZephyrAdapter<Binding>::Task<Context, StackSize>::Task(
    char const* const name, k_thread_stack_t* stack)
: TaskImpl<Context, StackSize>(name, TaskFunctionType(), stack)
{}

template<class Binding>
template<ContextType Context, size_t StackSize>
ZephyrAdapter<Binding>::Task<Context, StackSize>::Task(
    char const* const name, TaskFunctionType const taskFunction, k_thread_stack_t* stack)
: TaskImpl<Context, StackSize>(name, taskFunction, stack)
{}

template<class Binding>
template<ContextType Context, size_t StackSize>
ZephyrAdapter<Binding>::TaskImpl<Context, StackSize>::TaskImpl(
    char const* const name, TaskFunctionType const taskFunction, k_thread_stack_t* stack)
{
    ETL_ASSERT(StackSize >= sizeof(TaskInitializer),
        ETL_ERROR_GENERIC("StackSize too small"));
    new (K_THREAD_STACK_BUFFER(stack))
        TaskInitializer(Context, name, _task, _timer, stack, StackSize, taskFunction);
}

} // namespace async
