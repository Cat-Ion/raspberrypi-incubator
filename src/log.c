#include <stdio.h>
#include <time.h>
#include "therm.h"

#define LOG_DAY_SIZE (24*3600/PERIOD_DAY_S)
#define LOG_SIZE (LOG_S/PERIOD_S)

int sorted;

int log_num;
int log_day_num;

log_data_t log_data[LOG_SIZE],
	log_day_data[LOG_DAY_SIZE],
	log_sorted[LOG_SIZE],
	log_day_sorted[LOG_DAY_SIZE];

static void sortlogs() {
	if(sorted) {
		return;
	}
	if(log_num < LOG_SIZE) {
		if(log_num > 0) {
			log_sorted[log_num-1] = log_data[log_num-1];
		}
	} else {
		for(int i = 0; i < LOG_SIZE; i++) {
			log_sorted[i] = log_data[(log_num+i)%LOG_SIZE];
		}
	}
	if(log_day_num < LOG_DAY_SIZE) {
		if(log_day_num > 0) {
			log_day_sorted[log_day_num-1] = log_day_data[log_day_num-1];
		}
	} else {
		for(int i = 0; i < LOG_DAY_SIZE; i++) {
			log_day_sorted[i] = log_day_data[(log_day_num+i)%LOG_DAY_SIZE];
		}
	}
	sorted = 1;
}

void logs_load() {
	FILE *f = fopen(LOGPATH, "r");
	
	if(f == NULL) {
		return;
	}

	if(fread(&log_num, sizeof(int), 1, f) < 1) {
		return;
	}

	log_num = fread(log_data, sizeof(log_data_t), log_num, f);
	for(int i = 0; i < log_num; i++) {
		log_sorted[i] = log_data[i];
	}

	if(fread(&log_day_num, sizeof(int), 1, f) < 1) {
		return;
	}
	
	log_day_num = fread(log_day_data, sizeof(log_data_t), log_day_num, f);
	for(int i = 0; i < log_day_num; i++) {
		log_sorted[i] = log_day_data[i];
	}

	fclose(f);
}

void logs_save() {
	FILE *f = fopen(LOGPATH, "w");
	int n;
	if(f == NULL) {
		return;
	}

	sortlogs();

	n = log_num > LOG_SIZE ? LOG_SIZE : log_num;
	if(fwrite(&n, sizeof(int), 1, f) < 1) {
		return;
	}
	if(n > 0) {
		if(fwrite(log_sorted, sizeof(log_data_t), n, f) < log_num) {
			return;
		}
	}

	n = log_day_num > LOG_DAY_SIZE ? LOG_DAY_SIZE : log_day_num;
	if(fwrite(&n, sizeof(int), 1, f) < 1) {
		return;
	}
	
	if(n > 0) {
		if(fwrite(log_day_sorted, sizeof(log_data_t), n, f) < n) {
			return;
		}
	}
	fclose(f);
}

void logs_end() {
	logs_save();
}

void logs_init() {
	log_num = 0;
	log_day_num = 0;
	sorted = 1;

	logs_load();
}
static void formattime(char *buf, size_t len) {
	time_t t = time(NULL);
	struct tm now;
	localtime_r(&t, &now);
	strftime(buf, len, "%Y-%m-%d_%H:%M:%S", &now);
}

void log_values(float temp, float humidity) {
	static float smoothed_temp = 0, smoothed_hum = 0;

	log_data[log_num%LOG_SIZE].timestamp = time(NULL);
	log_data[log_num%LOG_SIZE].temperature = temp;
	log_data[log_num%LOG_SIZE].humidity = humidity;

	if(log_num == 0) {
		smoothed_temp = temp;
		smoothed_hum = humidity;
	} else {
		smoothed_temp = (15 * smoothed_temp + temp) / 16;
		smoothed_hum = (15 * smoothed_hum + humidity) / 16;
	}
	if((log_data[log_num%LOG_SIZE].timestamp % PERIOD_DAY_S) < PERIOD_S) {
		log_day_data[log_day_num % LOG_DAY_SIZE].timestamp = time(NULL);
		log_day_data[log_day_num % LOG_DAY_SIZE].temperature = smoothed_temp;
		log_day_data[log_day_num % LOG_DAY_SIZE].humidity = smoothed_hum;
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

time_t lastlogtime() {
	if(log_num == 0) {
		return 0;
	}
	return log_data[(log_num-1)%LOG_SIZE].timestamp;
}

time_t lastdaylogtime() {
	if(log_day_num == 0) {
		return 0;
	}
	return log_day_data[(log_day_num-1)%LOG_DAY_SIZE].timestamp;
}
