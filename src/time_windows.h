#include <stdio.h>  
#include <sys/timeb.h>  
#include <time.h>  

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
	struct _timeb timebuffer;
	_ftime(&timebuffer); // C4996  
						 // Note: _ftime is deprecated; consider using _ftime_s instead  

    struct timespec future;
    future.tv_sec = timebuffer.time;
    future.tv_nsec = timebuffer.millitm * MILLION;
    NORMALISE_TIMESPEC( future, milli );
    return future;
}