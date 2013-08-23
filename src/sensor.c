#include "therm.h"

uint16_t crc16(uint8_t *data, size_t len, uint8_t low, uint8_t high) {
	uint16_t crc = 0xFFFF;
	while(len--) {
		crc ^= *data++;
		for(int i = 0; i < 8; i++) {
			if(crc & 1) {
				crc >>= 1;
				crc ^= 0xA001;
			} else {
				crc >>= 1;
			}
		}
	}
	return crc == (low | (high << 8));
}

int read_data(float buf[2]) {
	static uint8_t cmd[3] = { 0x03, 0x00, 0x04 };
	uint8_t response[8];

	int valid = 0;
	
	for(int i = 0; i < 8; i++) {
		response[i] = 0xFF;
	}

	for(int i = 0; i < 5; i++) {
		i2c_send(SENSOR_ADDR, cmd, 3);
		nanosleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 150000000L }, NULL);
		if(i2c_read(SENSOR_ADDR, 8, response) < 8) {
			continue;
		}

		if(response[0] == 0x03 &&
		   response[1] == 0x04 &&
		   crc16(response, 6, response[6], response[7])) {
			valid = 1;
			break;
		}
	}

	if(valid) {
		int32_t rh = (response[2] << 8) | response[3];
		int32_t tm = (response[4] << 8) | response[5];

		if(tm & (1<<15)) {
			tm = -(tm ^ (1<<15));
		}
		buf[0] = rh * 0.1;
		buf[1] = tm * 0.1;
	}
	
	return valid;
}

int set_voltage_uint16(uint16_t v) {
	if(v > 4095) {
		v = 4095;
	}

	v <<= 4;
	
	uint8_t data[3];
	data[0] = 0x40;
	data[1] = (v >> 8) & 0xFF;
	data[2] = (v >> 0) & 0xFF;
	return i2c_send(DAC_ADDR, data, 3) != 3;
}

int set_voltage_uint16_eeprom(uint16_t v) {
	if(v > 4095) {
		v = 4095;
	}

	v <<= 4;
	
	uint8_t data[3];
	data[0] = 0x60;
	data[1] = (v >> 8) & 0x0F;
	data[2] = (v >> 0) & 0xFF;
	return i2c_send(DAC_ADDR, data, 3) != 3;
}

int set_voltage(float v) {
	if(v < 0) v = 0;
	if(v > 1) v = 1;

	uint16_t volt = (uint16_t) (v * 4095);
	volt <<= 4;

	return set_voltage_uint16(volt);
}
