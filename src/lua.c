#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include "therm.h"

char *tempscript;
char *humscript;

lua_State *L;

int lua_loaduserfun(const char *str, const char *name) {
	int status;
	status = luaL_loadstring(L, str);
	if(status) {
		return status;
	}
	lua_getglobal(L, "sandboxenv");
	lua_setupvalue(L, -2, 1);
	lua_setglobal(L, name);
	return 0;
}

void lua_end() {
	lua_close(L);
}

void lua_init() {
	char *sandbox;
	int status;

	L = luaL_newstate();
	luaL_openlibs(L);

	sandbox = loadfile("lua/env.lua");
	if(sandbox == NULL) {
		perror("Couldn't load lua/env.lua");
		exit(1);
	}
	status = luaL_loadstring(L, sandbox);
	if(status) {
		fprintf(stderr, "Error loading sandbox: %s\n", lua_tostring(L, -1));
		exit(1);
	}
	
	status = lua_pcall(L, 0, 0, 0);
	if(status != 0) {
		fprintf(stderr, "Error initializing sandbox: %s\n", lua_tostring(L, -1));
	}

	tempscript = loadfile("lua/temp.lua");
	if(tempscript == NULL) {
		perror("Couldn't load lua/temp.lua");
		exit(1);
	}
	status = lua_loaduserfun(tempscript, "temp");
	if(status) {
		fprintf(stderr, "Error loading temperature script: %s\n", lua_tostring(L, -1));
		exit(1);
	}

	humscript  = loadfile("lua/hum.lua");
	if(humscript == NULL) {
		perror("Couldn't load lua/hum.lua");
		exit(1);
	}
	status = lua_loaduserfun(humscript, "hum");
	if(status) {
		fprintf(stderr, "Error loading humidity script: %s\n", lua_tostring(L, -1));
		exit(1);
	}
}

void lua_reload() {
	lua_end();
	lua_init();
}

float lua_get_humidity_ref(float temp, float hum) {
	float result;
	lua_getglobal(L, "hum");
	lua_pushnumber(L, temp);
	lua_pushnumber(L, hum);
	lua_pushnumber(L, wanted_temperature);
	lua_pushnumber(L, wanted_humidity);
	if(lua_pcall(L, 4, 1, 0)) {
		fprintf(stderr, "Error running humidity script: %s\n", lua_tostring(L, -1));
		return wanted_humidity;
	}
	
	result = lua_tonumber(L, -1);
	lua_pop(L, 1);

	return result;
}

float lua_get_temperature_ref(float temp, float hum) {
	float result;
	lua_getglobal(L, "temp");
	lua_pushnumber(L, temp);
	lua_pushnumber(L, hum);
	lua_pushnumber(L, wanted_temperature);
	lua_pushnumber(L, wanted_humidity);
	if(lua_pcall(L, 4, 1, 0)) {
		fprintf(stderr, "Error running temperature script: %s\n", lua_tostring(L, -1));
		return wanted_temperature;
	}
	
	result = lua_tonumber(L, -1);
	lua_pop(L, 1);

	return result;
}

void lua_control(float temp, float hum) {
	wanted_humidity = lua_get_humidity_ref(temp, hum);
	wanted_temperature = lua_get_temperature_ref(temp, hum);
}

