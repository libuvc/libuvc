


#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>       /* gettimeofday */
#include <mach/mach_time.h> /* mach_absolute_time */
#include <mach/mach.h>      /* host_get_clock_service, mach_... */
#include <mach/clock.h>     /* clock_get_time */

#define BILLION 1000000000L
#define MILLION 1000000L

#define NORMALISE_TIMESPEC( ts, uint_milli )            \
    do {                                                \
        ts.tv_sec += uint_milli / 1000u;                \
        ts.tv_nsec += (uint_milli % 1000u) * MILLION;   \
        ts.tv_sec += ts.tv_nsec / BILLION;              \
        ts.tv_nsec = ts.tv_nsec % BILLION;              \
    } while (0)

static mach_timebase_info_data_t timebase = { 0, 0 }; /* numer = 0, denom = 0 */
static struct timespec           inittime = { 0, 0 }; /* nanoseconds since 1-Jan-1970 to init() */
static uint64_t                  initclock;           /* ticks since boot to init() */


/*
 * Get absolute future time for pthread timed calls
 *  Solution 1: microseconds granularity
 */
struct timespec get_abs_future_time_coarse(unsigned milli)
{
    struct timespec future;         /* ns since 1 Jan 1970 to 1500 ms in the future */
    struct timeval  micro = {0, 0}; /* 1 Jan 1970 */

    (void) gettimeofday(&micro, NULL);
    future.tv_sec = micro.tv_sec;
    future.tv_nsec = micro.tv_usec * 1000;
    NORMALISE_TIMESPEC( future, milli );
    return future;
}
