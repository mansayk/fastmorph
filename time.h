#ifndef __TIME_H_
#define __TIME_H_

#include <sys/time.h>		/*   gettimeofday	   */

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

#endif /* __TIME_H_ */
