
#include <sys/time.h>       /* gettimeofday */

#define BILLION 1000000000L
#define MILLION 1000000L

#define NORMALISE_TIMESPEC( ts, uint_milli )            \
    do {                                                \
        ts.tv_sec += uint_milli / 1000u;                \
        ts.tv_nsec += (uint_milli % 1000u) * MILLION;   \
        ts.tv_sec += ts.tv_nsec / BILLION;              \
        ts.tv_nsec = ts.tv_nsec % BILLION;              \
    } while (0)


/*
 * Get absolute future time for pthread timed calls
 *  Solution 1: microseconds granularity
 */
struct timespec get_abs_future_time_coarse(unsigned milli)
{
    struct timespec future;         /* ns since 1 Jan 1970 to 1500 ms in the future */
    clock_gettime(CLOCK_REALTIME, &future);
    NORMALISE_TIMESPEC( future, milli );
    return future;
}
