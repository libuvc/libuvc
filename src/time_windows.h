#define BILLION 1000000000L
#define MILLION 1000000L

#define NORMALISE_TIMESPEC( ts, uint_milli )            \
    do {                                                \
        ts.tv_sec += uint_milli / 1000u;                \
        ts.tv_nsec += (uint_milli % 1000u) * MILLION;   \
        ts.tv_sec += ts.tv_nsec / BILLION;              \
        ts.tv_nsec = ts.tv_nsec % BILLION;              \
    } while (0)

struct timespec get_abs_future_time_coarse(unsigned milli)
{
    struct timespec future;
    uint t_ms = timeGetTime(); // time in ms
    future.tv_sec = t_ms / 1000u;
    future.tv_nsec = (t_ms % 1000u) * MILLION;
    NORMALISE_TIMESPEC( future, milli );
    return future;
}