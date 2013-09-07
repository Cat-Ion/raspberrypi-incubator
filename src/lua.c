#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include "therm.h"

char *tempscript;
char *humscript;

lua_State *L;

static const char *get_function_name(lua_userfun_t function);
float lua_call_control_function(float temp, float hum, lua_userfun_t function);
int lua_set_control_function(const char *str, lua_userfun_t function);

static const char *get_function_name(lua_userfun_t function) {
	switch(function) {
	case LUA_USERFUN_TEMP:
		return "temp";
	case LUA_USERFUN_HUM:
		return "hum";
	default:
		return NULL;
	}
}

float lua_call_control_function(float temp, float hum, lua_userfun_t function) {
	float result;
	lua_getglobal(L, get_function_name(function));
	lua_pushnumber(L, temp);
	lua_pushnumber(L, hum);
	lua_pushnumber(L, wanted_temperature);
	lua_pushnumber(L, wanted_humidity);
	if(lua_pcall(L, 4, 1, 0)) {
		fprintf(stderr, "Error running script: %s\n", lua_tostring(L, -1));
		return function == LUA_USERFUN_TEMP ? wanted_temperature :
			function == LUA_USERFUN_HUM ? wanted_humidity :
			0;
	}
	
	result = lua_tonumber(L, -1);
	lua_pop(L, 1);

	return result;
}

void lua_control(float temp, float hum) {
	wanted_humidity = lua_call_control_function(temp, hum, LUA_USERFUN_HUM);
	wanted_temperature = lua_call_control_function(temp, hum, LUA_USERFUN_TEMP);
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
	status = lua_set_control_function(tempscript, LUA_USERFUN_TEMP);
	if(status) {
		fprintf(stderr, "Error loading temperature script: %s\n", lua_tostring(L, -1));
		exit(1);
	}

	humscript  = loadfile("lua/hum.lua");
	if(humscript == NULL) {
		perror("Couldn't load lua/hum.lua");
		exit(1);
	}
	status = lua_set_control_function(humscript, LUA_USERFUN_HUM);
	if(status) {
		fprintf(stderr, "Error loading humidity script: %s\n", lua_tostring(L, -1));
		exit(1);
	}
}

void lua_reload() {
	lua_end();
	lua_init();
}

int lua_set_control_function(const char *str, lua_userfun_t function) {
	int status;
	status = luaL_loadstring(L, str);
	if(status) {
		return status;
	}
	lua_getglobal(L, "sandboxenv");
	lua_setupvalue(L, -2, 1);
	lua_setglobal(L, get_function_name(function));
	return 0;
}

