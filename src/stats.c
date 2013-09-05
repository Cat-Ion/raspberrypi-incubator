#include <math.h>
#include "therm.h"

/* Calculate the mean and standard deviation for the num data points
   in data, and put the results into the four float pointers. */
void stats_data(log_data_t *data, int num,
				float *avg_temp, float *avg_hum,
				float *std_temp, float *std_hum) {
	float t = 0,
		h = 0;
	for(int i = 0; i < num; i++) {
		t += data[i].temperature;
		h += data[i].humidity;
	}

	*avg_temp = t / num;
	*avg_hum = h / num;

	t = h = 0;
	for(int i = 0; i < num; i++) {
		float u = data[i].temperature - *avg_temp;
		t += u*u;

		u = data[i].humidity - *avg_hum;
		h += u*u;
	}

	*std_temp = sqrt(t/num);
	*std_hum = sqrt(h/num);
}

/* Calculate the mean and standard deviation for the recent and daily
   logs, writing them to the global variables defined in therm.h */
void stats() {
	log_data_t *data;
	int num;

	data = getlog(&num);
	stats_data(data, num,
			   &avg_temperature, &avg_humidity,
			   &sd_temperature, &sd_humidity);
	data = getdaylog(&num);
	stats_data(data, num,
			   &avg_day_temperature, &avg_day_humidity,
			   &sd_day_temperature, &sd_day_humidity);
	
}

