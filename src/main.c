#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "therm.h"

void end() {
	i2c_end();
	httpd_end();
}

void init() {
	wanted_temperature = TEMP_DEF;
	wanted_humidity = HUM_DEF;
	i2c_init();
	httpd_init();
	logs_init();
	pid_init();

	struct sigaction sa = (struct sigaction) {
		.sa_handler = &reload,
		.sa_flags = 0,
		.sa_restorer = NULL
	};

	sigemptyset(&(sa.sa_mask));
	sigaddset(&(sa.sa_mask), SIGUSR1);

	if(sigaction(SIGUSR1, &sa, NULL) < 0) {
		fprintf(stderr, "Couldn't install signal handler.\n");
		exit(1);
	}
}

void reload(int signal) {
	httpd_reload();
}

int main(int argc, char **argv) {
	float data[2];

	init();

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	for(;;) {
		if(!read_data(data)) {
			continue;
		}

		log_values(data[1], data[0]);
		pid_control(data[1], data[0]);
		stats();

		if(logstdout(data) == EOF) {
			break;
		}

		delay_ns(&ts, PERIOD_S, 0);
	}

	end();
}
