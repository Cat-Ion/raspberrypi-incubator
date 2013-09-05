#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "therm.h"

static void end() {
	logs_end();
	i2c_end();
	httpd_end();
	exit(0);
}

static void init() {
	i2c_init();
	httpd_init();
	logs_init();
	pid_init();

	/* Load persistent settings, or set the default values. */
	if(persistent_load() == -1) {
		wanted_temperature = TEMP_DEF;
		wanted_humidity = HUM_DEF;
	}

	struct sigaction sa = (struct sigaction) {
		.sa_handler = &reload,
		.sa_flags = 0,
		.sa_restorer = NULL
	};

	sigemptyset(&(sa.sa_mask));
	sigaddset(&(sa.sa_mask), SIGUSR1);
	sigaddset(&(sa.sa_mask), SIGINT);
	sigaddset(&(sa.sa_mask), SIGTERM);

	/* Catch USR1, and reload settings */
	if(sigaction(SIGUSR1, &sa, NULL) < 0) {
		fprintf(stderr, "Couldn't install signal handler.\n");
		exit(1);
	}

	/* Catch INT and TERM, then quit gracefully */
	sa.sa_handler = &end;
	
	if(sigaction(SIGTERM, &sa, NULL) < 0 || sigaction(SIGINT, &sa, NULL) < 0) {
		fprintf(stderr, "Couldn't install signal handler.\n");
		exit(1);
	}
}

static void reload(int signal) {
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
