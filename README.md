# asyncc - async for (embedded) C

A lightweight and useful async library for C focused on embedded systems
applications. This is heavily inspired by both Adam Dunkels' Protothreads
library, and Sandro Magi's async.h library (which was also inspired by
Protothreads).  The goal is to give a more authentic async/await experience
in C with minimal overhead (both in memory and developer experience).

# Why This Library?
Existing stackless event-driven libraries for C work great in smaller scopes
where the lack of stack local variables is easily overcome with static function
variables or a few custom async state variables with minimal nesting, but in
more complex projects with many async functions this becomes a non-trivial
exercise that hinders understanding and refactoring, and leads to the
temptation to revert to a more traditional state-machine-based event-driven
architecture in the code (or to take the hit to RAM and use a new thread in
your RTOS).

It is the opinion of this library is that there is a wide range of applications
that are too complex for a stackless concurrency library, but not complex
enough to warrant the overhead (and additional complexity) of true
multithreading with preemptive context switching and/or parallel execution.
The goal of this library is to provide something in the middle that scales
easily as a project grows (and as async infects everything in the system).

If you want to avoid the complexity and overhead of RTOS threads, but also
aren't in the mood for hand-crafted state machines in a custom time-slicing
task scheduler or superloop, then this library could be for you.

# Design Decisions

## Stacks

To implement this middle path approach to blocking code semantics we need a
stack to keep track of the execution context of each async thread, but we also
need it to be portable, scalable, and easy to use.  Also, the main focus of
this library is embedded sysems, so we will be focused on static allocations
throughout the design.

## Batteries-Included

There are a few core components that almost any async application will require,
so to make it easier to get started we provide a basic async runtime that
includes the main event loop, timer/sleep functionality, and a simple event
polling system (and queue) to support a broad range of application needs.

A secondary motivation for this opinionated batteries-included approach is to
drive consistency in how async functions are driven and wired together (more
like the runtimes used in other languages with official async support).  In an
embedded system the imagined architecture would be one event loop per thread of
an RTOS, or only a single event loop in a bare metal system.

## Assumptions

This library assumes that it is being used in a resource-constrained embedded
system.  As such many of the features are optional, but we try to pack in as
much as possible into the core functionality.

This library assumes a single thread of execution per event loop with no
shared memory access with event loops running in other threads.  Nothing is
stopping you from using RTOS IPC to coordinate async runtimes/event loops if
you so choose.  For embedded systems the use case of spreading async tasks over
a pool of several concurrent threads is largely covered by manually spreading
async tasks across several threads with separate async runtimes.  In theory we
support dynamic spawning of async tasks, but the intended use of this library
is for statically defined embedded systems.

## Magic

This library uses the same switch case macro magic used in Protothreads and
async.h, and there is no intention of using some of the other techniques that
let you use switch cases in async functions.  It is assumed that anyone using
this library will know about the limitations on switch cases.

We take the magic to the next level by attempting to handle stack allocations
using macros, but the main goal is for this library to be understandable and
simple enough that it doesn't scare away users due to impossible-to-debug
complex macro magic.

# Examples

NOTE: this is all currently speculation at this point (the code only exists in
my head for these functions and macros).  This was a helpful exercise in
thinking about the API I would want to use, but I do think I know how to make
this all work behind the scenes.

```C
#include "asyncc.h"

// Defined in asyncc.h is the all-important struct async_state:
// TODO: possibly automagically handle state tracking on the stack itself
struct async_state {
    uint16_t idx;       // Index into the stack
    uint16_t max;       // Stack depth
    uint8_t *stack;     // Pointer to the stack
};

async example(struct async *s)
{
    // Built-in support for defining local stack variables, checks stack
    // depth and calls global error callback on any overflow (handled safely)
    ASYNC_BEGIN(s,
        uint8_t i,
        uint8_t buff[8],
        struct async_state s1,
        uint8_t s1_stack[32],
        ASYNC_STATE(s2, 64));

    // Stack variables accessible via convenience macros which expand to
    // access via pointer to the stack location
    for(i=0;i<8;i++) {
        // Easily await other async functions, just pass along the state
        buff[i] = await(get_byte(s));
    }

    // Note: the fork-join parallelism scenario does require explicit
    // creation of new "sub-stacks" at the forking level, but these can be
    // created in the begin macro like any other "local" variable (see above)

    await(some_func(&s1) & some_func(&s2));     // Wait until both complete
    await(some_func(&s1) | some_func(&s2));     // Wait until one completes

    ASYNC_END();
}

struct async_runtime runtime;
uint8_t stack[32];
struct async_state s;

// This callback must be defined somewhere
void async_error(struct async_state *s

// A runtime comes with basic timing functionality that needs to be driven
// by a hardware timer ISR or an RTOS timer event of some kind.
void timer_isr_or_callback(void)
{
    ASYNC_TICK(&runtime, ISR_TICK_IN_MS);
}

int main(void)
{
    // Set up runtime and add an async task to it
    async_init(&runtime);
    async_sched(&runtime, example, &s, stack);

    // Add multiple tasks, this time using a helper macro
    ASYNC_SCHED(&runtime, example, 32);     // Can run multiple instances
    ASYNC_SCHED(&runtime, example_2, 64);

    // Helper macro to define and init new async_state instances
    ASYNC_STATE(s2, 128);

    for(;;) {
        // Runs next runnable task
        async_next(&runtime);

        // You can also manually drive threads if you so choose
        ASYNC_CALL(example, s2);
    }
}

```

# Why Shouldn't You Use This Library?

The synchronous/blocking coding style that async and true threads unlock is a
lie.  Explicit event-driven state machines are sometimes the right answer,
especially if the problem is not very linear/sequential.  The event polling
system of this library does make it easy to support explicit state machines,
but it's possible that a library focused on event-driven hierarchical state
machines could be a better fit for some applications.

# Ideas

## Stacks all the way down

I had this idea while considering the best way to support the fork-join
parallelism in this library: just use the stack for everything.  This would
also include things like timer support, event support, and other things that
would typically require some fixed fields in the state struct or in the runtime
struct.

This would make it harder to get good estimates of the stack depth required,
but for complex use cases this is already a tough problem without profiling the
code or doing a detailed analysis.  We could clean up the user experience in
other areas by just skimming a few bytes off the bottom of the stack in the
runtime code before the application code is any the wiser.  The user would only
need to provide a stack and initialize it (no more state variables).

One downside to this is that state or runtime variables would be slightly
harder to watch and debug if they are allocated on our stack, but there are a
few half-measures that could make it not horrible (fixed order, or use one
struct for everything that we could possibly need).

I think for fork-join parallelism scenarios this could be enough of a QOL
improvement (half the number of variable declarations) that it is worth the
trade-off.  And some of the downsides can be overcome with some special
debugging tools for inspecting the states on the stack.

TODO: see what this looks like in code
