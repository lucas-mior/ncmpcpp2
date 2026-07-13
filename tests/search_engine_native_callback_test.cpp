#include <cassert>
#include <cstdlib>

#include "screens/nc_search_engine.h"

struct TestAbort { };

static void test_not_runnable(void);
static void test_completed_action(void);
static void test_prompt_abort(void);

int
main()
{
    test_not_runnable();
    test_completed_action();
    test_prompt_abort();
    std::exit(EXIT_SUCCESS);
}

static void
test_not_runnable()
{
    bool action_called = false;
    native_search_engine_compat::RunCurrentResult result;

    result = native_search_engine_compat::invoke_run_current<TestAbort>(
        []() { return false; },
        [&action_called]() { action_called = true; });
    assert(result
           == native_search_engine_compat::RunCurrentResult::NotRunnable);
    assert(!action_called);
}

static void
test_completed_action()
{
    bool action_called = false;
    native_search_engine_compat::RunCurrentResult result;

    result = native_search_engine_compat::invoke_run_current<TestAbort>(
        []() { return true; },
        [&action_called]() { action_called = true; });
    assert(result
           == native_search_engine_compat::RunCurrentResult::Completed);
    assert(action_called);
}

static void
test_prompt_abort()
{
    bool action_called = false;
    native_search_engine_compat::RunCurrentResult result;

    result = native_search_engine_compat::invoke_run_current<TestAbort>(
        []() { return true; },
        [&action_called]() {
            action_called = true;
            throw TestAbort();
        });
    assert(result
           == native_search_engine_compat::RunCurrentResult::Aborted);
    assert(action_called);
}
