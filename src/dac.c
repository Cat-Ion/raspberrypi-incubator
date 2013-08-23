#include "therm.h"

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
