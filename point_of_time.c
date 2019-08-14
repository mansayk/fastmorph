#include<stdio.h>
#include <sys/time.h>
#include "point_of_time.h"

void time_start(struct timeval *tv) { gettimeofday(tv, NULL); }

long time_stop(struct timeval *tv)

{
  struct timeval tv2,dtv;
  gettimeofday(&tv2, NULL);

  dtv.tv_sec= tv2.tv_sec  - tv->tv_sec;

  dtv.tv_usec=tv2.tv_usec - tv->tv_usec;

  if(dtv.tv_usec<0) { dtv.tv_sec--; dtv.tv_usec+=1000000; }

  return dtv.tv_sec*1000+dtv.tv_usec/1000;
}

