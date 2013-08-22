#include <math.h>
#include "therm.h"

void stats() {
	int end = log_num < LOG_SIZE ? log_num : LOG_SIZE;

	float t = 0, h = 0;
	
	for(int i = 0; i < end; i++) {
		t += log_data[i].temperature;
		h += log_data[i].humidity;
	}

	avg_temperature = t / end;
	avg_humidity = h / end;

	t = h = 0;
	for(int i = 0; i < end; i++) {
		float u = log_data[i].temperature - avg_temperature;
		t += u*u;

		u = log_data[i].humidity - avg_humidity;
		h += u*u;
	}

	sd_temperature = sqrt(t/end);
	sd_humidity = sqrt(h/end);
}

