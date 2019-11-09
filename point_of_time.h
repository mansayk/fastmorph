#ifndef _POINT_OF_TIME_H_
#define _POINT_OF_TIME_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

void time_start(struct timeval *);
long time_stop(struct timeval *);

#ifdef __cplusplus
}
#endif

#endif /* _POINT_OF_TIME_H_ */
