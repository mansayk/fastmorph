#ifndef _TIME_H_
#define _TIME_H_

#include <sys/time.h>

struct timezone *tv;
struct timeval *tz;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Get start time
 */
void time_start(struct timeval *, struct timezone *);


/*
 * Get finish time and calculate the difference
 */
long time_stop(struct timeval *, struct timezone *);

#ifdef __cplusplus
}
#endif

#endif /* _TIME_H_ */
