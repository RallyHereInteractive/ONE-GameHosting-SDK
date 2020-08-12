#pragma once

#include <functional>

namespace one {

void sleep(int ms);

// Loops for given number of times, calling the given function. If the function
// returns true, then the loop breaks. If the function returns false, then
// the loop sleeps ms_per_loop milliseconds and then continues.
void for_sleep(int count, int ms_per_loop, std::function<bool()> cb);

// Waits for the given ms, calling the interrupt callback periodically. Returns
// true if the wait period is exceeded and the interrupt function did not
// return true when called.
bool wait_with_cancel(int wait_ms, std::function<bool()> cb);

}  // namespace one
