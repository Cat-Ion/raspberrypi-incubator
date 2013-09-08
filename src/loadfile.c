#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include "therm.h"

char *loadfile(const char *name) {
	struct stat st;
	int fd;
	char *ret;
	
	if(stat(name, &st) < 0) {
		return NULL;
	}
	ret = malloc(st.st_size + 1);
	fd = open(name, O_RDONLY);
	if(fd < 0) {
		free(ret);
		return NULL;
	}
	read(fd, ret, st.st_size);
	ret[st.st_size] = '\0';
	close(fd);
	return ret;
}
