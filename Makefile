CROSSROOT=/home/clfs

CFLAGS+=-Os -Wall -pedantic -std=c99 -D_POSIX_C_SOURCE=199309L -I$(CROSSROOT)/include -Iinclude -g
LDFLAGS+=-L$(CROSSROOT)/lib -lrt -lmicrohttpd -lm -lz -static

SRC=main.c delay_ns.c sensor.c i2c.c httpd.c log.c stats.c pid_control.c dac.c persistent.c
OBJ=$(SRC:%.c=src/%.o)
BIN=main

all: $(BIN)

main: $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -f src/*.o $(BIN)

.PHONY: all clean
