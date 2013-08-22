#include <fcntl.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "therm.h"

int fd;

void i2c_init() {
	fflush(stdout);
	fd = open("/dev/i2c-1", O_RDWR);
	fflush(stdout);
	if(fd == -1) {
		perror("Error opening i2c fd");
		exit(1);
	}
}

void i2c_end() {
	close(fd);
}

size_t i2c_send(uint8_t addr, uint8_t *data, size_t len) {
	if(ioctl(fd, I2C_SLAVE, addr) < 0) {
		perror("Failed to acquire bus access and/or talk to slave.\n");
		exit(1);
	}
	return write(fd, data, len);
}

size_t i2c_read(uint8_t addr, size_t len, uint8_t *out) {
	if(ioctl(fd, I2C_SLAVE, addr) < 0) {
		perror("Failed to acquire bus access and/or talk to slave.\n");
		exit(1);
	}
	return read(fd, out, len);
}
