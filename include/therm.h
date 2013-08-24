#ifndef THERM_H
#define THERM_H
#include <stdint.h>
#include <time.h>

#include "config.h"

typedef struct {
	time_t timestamp;
	float temperature;
	float humidity;
} log_data_t;

float avg_temperature;
float avg_humidity;

float sd_temperature;
float sd_humidity;

float avg_day_temperature;
float avg_day_humidity;

float sd_day_temperature;
float sd_day_humidity;

float wanted_temperature;
float wanted_humidity;

void init();
void end();
void reload();

void httpd_init();
void httpd_end();
void httpd_reload();

void i2c_init();
void i2c_end();
size_t i2c_send(uint8_t addr, uint8_t *data, size_t len);
size_t i2c_read(uint8_t addr, size_t len, uint8_t *out);

int read_data(float buf[2]);
int set_voltage(float v);
int set_voltage_uint16(uint16_t v);
int set_voltage_uint16_eeprom(uint16_t v);

void delay_ns(struct timespec *last, time_t s, long ns);

void stats();

void logs_init();
void log_values(float temp, float humidity);
int logstdout(float data[2]);
log_data_t *getlog(int *num);
log_data_t *getdaylog(int *num);
time_t lastlogtime();
time_t lastdaylogtime();

void pid_control(float temp, float hum);
#endif
