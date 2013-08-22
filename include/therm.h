#ifndef THERM_H
#define THERM_H
#include <stdint.h>
#include <time.h>

#include "config.h"

struct {
	time_t timestamp;
	float temperature;
	float humidity;
} log_data[LOG_SIZE];

float avg_temperature;
float avg_humidity;

float sd_temperature;
float sd_humidity;

int log_num;
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

int logstdout(float data[2]);

void pid_control(float temp, float hum);
#endif
