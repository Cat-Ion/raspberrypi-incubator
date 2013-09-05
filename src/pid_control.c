#include "therm.h"

struct pid_t {
	float p, i, d;
	float imax;

	float old;
	float acc;
} pid_temp, pid_hum;

/* Initialize the PID values to the defaults */
void pid_init() {
	pid_temp.p   = T_P;
	pid_temp.i   = T_I;
	pid_temp.d   = T_D;
	pid_temp.old = 0;
	pid_temp.acc = 0;
	
	pid_hum.p   = H_P;
	pid_hum.i   = H_I;
	pid_hum.d   = H_D;
	pid_hum.old = 0;
	pid_hum.acc = 0;
}

/* Writes the current temp/humidity PID values to the given
   pointers */
void pid_getvalues(float *tp, float *ti, float *td,
                   float *hp, float *hi, float *hd) {
	*tp = pid_temp.p;
	*ti = pid_temp.i;
	*td = pid_temp.d;
	*hp = pid_hum.p;
	*hi = pid_hum.i;
	*hd = pid_hum.d;
}

/* Sets the current temp/humidity PID values from the given floats */
void pid_setvalues(float tp, float ti, float td,
                   float hp, float hi, float hd) {
	pid_temp.p = tp;
	pid_temp.i = ti;
	pid_temp.d = td;
	pid_hum.p  = hp;
	pid_hum.i  = hi;
	pid_hum.d  = hd;
}

/* Returns the voltage to use for heating, as a fraction of the
   maximum voltage, given the current temperature. */
static float temp_control(float temp) {
	float diff = temp - wanted_temperature;

	pid_temp.acc += diff * PERIOD_S;
	
	if(pid_temp.acc > T_IMAX) {
		pid_temp.acc = T_IMAX;
	} else if(pid_temp.acc < -T_IMAX) {
		pid_temp.acc = -T_IMAX;
	}

	float p  = diff;
	float i  = pid_temp.acc;
	float d  = (pid_temp.old - diff);

	pid_temp.old = diff;
	
	float r = pid_temp.p * p + pid_temp.i * i + pid_temp.d * d;
	return r;
}

/* Control the heating and humidifier as a function of the current
   environment values */
void pid_control(float temp, float hum) {
	set_voltage(temp_control(temp));
}
