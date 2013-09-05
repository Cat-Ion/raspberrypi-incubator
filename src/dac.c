#include "therm.h"

/* Set the voltage directly, using a 12-bit value */
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

/* Set the voltage as above, but also store it in the EEPROM, so the
   DAC uses this value after powering on. */
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

/* Set the voltage as a float between 0 and 1, scaling it to the
   effective range MIN_VOLTATE to 1 */
int set_voltage(float v) {
	if(v < 0) v = 0;
	if(v > 1) v = 1;

	/* Adjust for voltage drop through any transistors */
	if(v > 0) {
		v = MIN_VOLTAGE + v * (1-MIN_VOLTAGE);
	}

	uint16_t volt = (uint16_t) (v * 4095);
	volt <<= 4;

	return set_voltage_uint16(volt);
}
