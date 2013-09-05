#include <stdio.h>
#include <stdlib.h>
#include "therm.h"

/* Loads settings from persistent storage, returning the number of
   items read, or -1 on error. */
int persistent_load() {
	FILE *f = fopen(SAVEPATH, "r");
	float pid[6];
	int ret = 0;

	if(!f) {
		return -1;
	}

	ret += fread(&wanted_temperature, sizeof(wanted_temperature), 1, f);
	ret += fread(&wanted_humidity, sizeof(wanted_humidity), 1, f);
	
	ret += fread(pid, sizeof(float), 6, f);
	pid_setvalues(pid[0], pid[1], pid[2], pid[3], pid[4], pid[5]);

	fclose(f);
	return ret;
}

/* Writes settings to persistent storage, returning the number of
   items written, or -1 on error. */
int persistent_write() {
	FILE *f = fopen(SAVEPATH, "w");
	float pid[6];
	int ret = 0;

	if(!f) {
		fprintf(stderr, "Couldn't open " SAVEPATH " for writing.\n");
		return -1;
	}

	ret += fwrite(&wanted_temperature, sizeof(wanted_temperature), 1, f);
	ret += fwrite(&wanted_humidity, sizeof(wanted_humidity), 1, f);
	
	pid_getvalues(pid+0, pid+1, pid+2, pid+3, pid+4, pid+5);
	ret += fwrite(pid, sizeof(float), 6, f);

	fclose(f);

	fprintf(stderr, "Done.\n");
	return ret;
}
