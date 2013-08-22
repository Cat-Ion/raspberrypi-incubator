#include "therm.h"

struct pid_t {
	float p, i, d;
	float imax;
};

float temp_control(float temp) {
	float diff = temp - wanted_temperature;
	static float old = 0;

	float p = diff;
	static float i = 0;
	float d = (old - diff);

	old = diff;
	
	i += diff * PERIOD_S;
	if(i > T_IMAX) {
		i = T_IMAX;
	} else if(i < -T_IMAX) {
		i = -T_IMAX;
	}

	float r = T_P * p + T_I * i + T_D * d;
	return r;
}

void pid_control(float temp, float hum) {
	set_voltage(temp_control(temp));
}
