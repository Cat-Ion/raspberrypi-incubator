#include <errno.h>
#include <sys/select.h>
#include <time.h>
#include "therm.h"

/* Delay s+ns/1e9 seconds, starting from the time stored in
   *last. Repeated calls to this should eliminate jitter as much as
   possible. */
void delay_ns(struct timespec *last, time_t s, long ns) {
	int ret;
	last->tv_sec += s + (last->tv_nsec + ns) / 1000000000L;
	last->tv_nsec = (last->tv_nsec + ns) % 1000000000L;
	do {
		ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, last, NULL);
	} while(ret == EINTR);
}
