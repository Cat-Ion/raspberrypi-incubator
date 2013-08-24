#include <stdio.h>
#include <time.h>
#include "therm.h"

#define LOG_DAY_SIZE (24*3600/PERIOD_S)

int sorted;

int log_day_num;
log_data_t log_day_data[LOG_DAY_SIZE],
	log_day_sorted[LOG_DAY_SIZE],
	log_sorted[LOG_SIZE];

static void formattime(char *buf, size_t len) {
	time_t t = time(NULL);
	struct tm now;
	localtime_r(&t, &now);
	strftime(buf, len, "%Y-%m-%d_%H:%M:%S", &now);
}

void log_values(float temp, float humidity) {
	log_data[log_num%LOG_SIZE].timestamp = time(NULL);
	log_data[log_num%LOG_SIZE].temperature = temp;
	log_data[log_num%LOG_SIZE].humidity = humidity;
	if((log_data[log_num%LOG_SIZE].timestamp % 60) < 10) {
		log_day_data[log_day_num % LOG_DAY_SIZE] = log_data[log_num];
		log_day_num++;
	}
	log_num++;
	sorted = 0;
}

int logstdout(float data[2]) {
	char timestr[64];
	formattime(timestr, sizeof(timestr));
	printf("%s %4.1f %4.1f\n", timestr, data[0], data[1]);
	return fflush(stdout);
}

static void sortlogs() {
	if(sorted) {
		return;
	}
	if(log_num < LOG_SIZE) {
		log_sorted[log_num-1] = log_data[log_num-1];
	} else {
		for(int i = 0; i < log_num; i++) {
			log_sorted[i] = log_data[(log_num+i)%LOG_SIZE];
		}
	}
	if(log_day_num < LOG_DAY_SIZE) {
		log_day_sorted[log_day_num-1] = log_day_data[log_day_num-1];
	} else {
		for(int i = 0; i < log_num; i++) {
			log_day_sorted[i] = log_day_data[(log_day_num+i)%LOG_DAY_SIZE];
		}
	}
}

log_data_t *getlog(int *num) {
	sortlogs();
	*num = (log_num < LOG_SIZE) ? log_num : LOG_SIZE;
	return log_sorted;
}

log_data_t *getdaylog(int *num) {
	sortlogs();
	*num = (log_day_num < LOG_DAY_SIZE) ? log_day_num : LOG_DAY_SIZE;
	return log_day_sorted;
}
