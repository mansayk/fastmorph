#include "time.h"

/*
 * Get start time
 */
void time_start(struct timeval *tv, struct timezone *tz)
{
    gettimeofday(tv, tz);
}


/*
 * Get finish time and calculate the difference
 */
long time_stop(struct timeval *tv_begin, struct timezone *tz)
{
    struct timeval tv, dtv;
    gettimeofday(&tv, &tz);
    dtv.tv_sec = tv.tv_sec - tv_begin->tv_sec;
    dtv.tv_usec = tv.tv_usec - tv_begin->tv_usec;
    if(dtv.tv_usec < 0) {
	dtv.tv_sec--;
	dtv.tv_usec += 1000000;
    }
    return (dtv.tv_sec * 1000) + (dtv.tv_usec / 1000);
}
