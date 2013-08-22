#include <stdio.h>
#include <time.h>
#include "therm.h"

static void formattime(char *buf, size_t len) {
	time_t t = time(NULL);
	struct tm now;
	localtime_r(&t, &now);
	strftime(buf, len, "%Y-%m-%d_%H:%M:%S", &now);
}

int logstdout(float data[2]) {
	char timestr[64];
	formattime(timestr, sizeof(timestr));
	printf("%s %4.1f %4.1f\n", timestr, data[0], data[1]);
	return fflush(stdout);
}
