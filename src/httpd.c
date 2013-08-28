#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <zlib.h>
#include "therm.h"

#define POSTBUFFERSIZE (32*1024)

enum {
	GET,
	POST,
	OTHER
};

struct MHD_Daemon *daemon;
struct MHD_Response *mhd_404_response,
	*mhd_please_auth_response;
char *root_template = NULL;

struct postinfo {
	float temp, hum;
	struct MHD_PostProcessor *pp;
};

int iterate_post(void *cls, enum MHD_ValueKind kind, const char *key, const char *filename, const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size) {
	struct postinfo *pi = cls;
	if(size == 0) {
		return MHD_YES;
	}
	if(!strcmp(key, "temp")) {
		pi->temp = strtof(data, NULL);
	} else if(!strcmp(key, "hum")) {
		pi->hum = strtof(data, NULL);
	}

	return MHD_YES;
}

struct MHD_Response *gzip_if_possible_buffer(struct MHD_Connection *connection,
                                             size_t len,
                                             void *response,
                                             int flags) {
	const char *comp = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Accept-Encoding");
	if(comp == NULL) {
		return MHD_create_response_from_buffer(len, response, MHD_RESPMEM_MUST_FREE);
	} else if(strstr(comp, "deflate")) {
		uint64_t deflen = compressBound(len);
		unsigned char *def = malloc(deflen);
		compress2(def, (uLongf *) &deflen, response, len, 9);
		free(response);
		struct MHD_Response *ret = MHD_create_response_from_buffer(deflen, def, MHD_RESPMEM_MUST_FREE);
		MHD_add_response_header(ret, "Content-Encoding", "deflate");
		return ret;
	} else {
		return MHD_create_response_from_buffer(len, response, MHD_RESPMEM_MUST_FREE);
	}
}

void add_expires_header(struct MHD_Response *res, time_t exptime) {
	char buf[64];
	struct tm tm;

	MHD_add_response_header(res, "Cache-Control", "max-age=10");

	if(gmtime_r(&exptime, &tm) == NULL) {
		return;
	}

	strftime(buf, 64, "%a, %d %b %Y %T GMT", &tm);
	MHD_add_response_header(res, "Expires", buf);
}

char *format_logs(log_data_t *data, int num, size_t *len) {
		char *response = malloc(
		                        strlen("{\n\"logdata\": [\n")
		                        + num * strlen("\t[ \"YYYY-MM-DDThh:mm:ss.000+ZZZZ\", tt.tt, hhh.hh ],\n")
		                        + strlen("],\n")
		                        + strlen("\"temp\": [ ttt.t, t.tt, ttt.t ],\n")
		                        + strlen("\"hum\":  [ hhh.h, h.hh, hhh.h ]\n")
		                        + strlen("}")
		                        );
		*len = 0;
		*len += sprintf(response, "{\n\"logdata\": [\n");
		for(int i = 0; i < num; i++) {
			struct tm tm;
			localtime_r(&data[i].timestamp, &tm);

			*len += sprintf (response + *len, "\t[ \"");
			*len += strftime(response + *len, 32, "%Y-%m-%dT%H:%M:%S.000%z", &tm);
			*len += sprintf (response + *len, "\", %5.2f, %6.2f ]%c\n",
							 data[i].temperature,
							 data[i].humidity,
							 ", "[i == (num - 1)]);
		}
		*len += sprintf(response + *len,
		               "],\n"
		               "\"temp\": [ %5.1f, %4.2f, %5.1f ],\n"
		               "\"hum\":  [ %5.1f, %4.2f, %5.1f ]\n"
		               "}",
		               avg_temperature, sd_temperature, wanted_temperature,
		               avg_humidity, sd_humidity, wanted_humidity);
		return response;
}

static int handle_conn(void *cls, struct MHD_Connection *connection, 
                       const char *url, 
                       const char *method, const char *version, 
                       const char *upload_data, 
                       size_t *upload_data_size, void **con_cls) {
	struct MHD_Response *mhd_response;
	int ret = MHD_NO;

	if(!strcmp(url, "/")) {
		char *response = malloc(strlen(root_template)*2);
		sprintf(response,
		        root_template,
		        wanted_temperature,
		        wanted_humidity,
		        avg_temperature,
		        avg_humidity,
		        sd_temperature,
		        sd_humidity
		        );
		mhd_response = gzip_if_possible_buffer(connection,
		                                       strlen(response),
		                                       (void*) response,
		                                       MHD_RESPMEM_MUST_FREE);
		MHD_add_response_header(mhd_response, "Content-Type", "text/html");
		add_expires_header(mhd_response, lastlogtime() + PERIOD_S);
		ret = MHD_queue_response(connection, MHD_HTTP_OK, mhd_response);
		MHD_destroy_response(mhd_response);
		return ret;
	}

	if(!strcmp(url, "/log.json")) {
		int num;
		size_t len;
		log_data_t *data = getlog(&num);
		char *response = format_logs(data, num, &len);
		mhd_response = gzip_if_possible_buffer(connection, len, (void *) response, MHD_RESPMEM_MUST_FREE);
		MHD_add_response_header(mhd_response, "Content-Type", "text/plain");
		MHD_add_response_header(mhd_response, "Content-Disposition", "inline");
		add_expires_header(mhd_response, lastlogtime() + PERIOD_S);
		ret = MHD_queue_response(connection, MHD_HTTP_OK, mhd_response);
		MHD_destroy_response(mhd_response);
		return ret;
	}

	if(!strcmp(url, "/daylog.json")) {
		int num;
		size_t len;
		log_data_t *data = getdaylog(&num);
		char *response = format_logs(data, num, &len);
		mhd_response = gzip_if_possible_buffer(connection, len, (void *) response, MHD_RESPMEM_MUST_FREE);
		MHD_add_response_header(mhd_response, "Content-Type", "text/plain");
		MHD_add_response_header(mhd_response, "Content-Disposition", "inline");
		add_expires_header(mhd_response, lastlogtime() + PERIOD_DAY_S);
		ret = MHD_queue_response(connection, MHD_HTTP_OK, mhd_response);
		MHD_destroy_response(mhd_response);
		return ret;
	}

	if(!strcmp(url, "/set")) {
		if(strcmp(method, "POST")) {
			return(MHD_NO);
		}

		char *user = NULL,
			*pass = NULL;
		user = MHD_basic_auth_get_username_password(connection, &pass);
		int authd = user != NULL &&
			pass != NULL &&
			!strcmp(user, USER) &&
			!strcmp(pass, PASS);
		free(user);
		free(pass);
		if(!authd) {
			ret = MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, mhd_please_auth_response);
			return ret;
		}

		struct postinfo *pi = *con_cls;
		if(pi == NULL) {
			pi = malloc(sizeof(struct postinfo));
			pi->pp = MHD_create_post_processor(connection, 1024, iterate_post, pi);
			pi->temp = NAN;
			pi->hum = NAN;
			*con_cls = pi;
			return MHD_YES;
		}

		if(*upload_data_size) {
			MHD_post_process(pi->pp, upload_data, *upload_data_size);
			*upload_data_size = 0;
			return MHD_YES;
		} else {
			MHD_destroy_post_processor(pi->pp);
		}

		if(pi->temp == NAN || pi->hum == NAN) {
			return MHD_NO;
		}

		float f_temp = pi->temp,
			f_hum = pi->hum;
		free(pi);

		if(f_temp >= TEMP_MIN && f_temp <= TEMP_MAX &&
		   f_hum  >= HUM_MIN && f_hum  <= HUM_MAX) {
			wanted_temperature = f_temp;
			wanted_humidity = f_hum;
		}

		mhd_response = MHD_create_response_from_data(0, NULL, MHD_NO, MHD_NO);
		ret = MHD_queue_response(connection, MHD_HTTP_FOUND, mhd_response);
		MHD_add_response_header(mhd_response, "Location", "/");
		MHD_destroy_response(mhd_response);
		return ret;
	}

	ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, mhd_404_response);
	return ret;
}

void httpd_init() {
	daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY|MHD_USE_DEBUG, PORT, NULL, NULL,
	                          &handle_conn, NULL, MHD_OPTION_END);
	if(daemon == NULL) {
		fprintf(stderr, "Failed to start the httpd\n");
		exit(1);
	} else {
		fprintf(stderr, "httpd started.\n");
	}
	mhd_404_response = MHD_create_response_from_buffer(3, (void *)"404", MHD_RESPMEM_PERSISTENT);
	mhd_please_auth_response = MHD_create_response_from_buffer(strlen("Please authenticate"),
	                                                           (void *)"Please authenticate",
	                                                           MHD_RESPMEM_PERSISTENT);
	MHD_add_response_header(mhd_please_auth_response,
	                        MHD_HTTP_HEADER_WWW_AUTHENTICATE,
	                        "Basic realm=\"tortoises\"");

	httpd_reload();
}

void httpd_end() {
	MHD_stop_daemon(daemon);
	MHD_destroy_response(mhd_404_response);
	MHD_destroy_response(mhd_please_auth_response);
	free(root_template);
}

void httpd_reload() {
	struct stat st;
	stat("roottemplate", &st);
	if(root_template != NULL) {
		free(root_template);
	}
	root_template = malloc(st.st_size + 1);

	int tpfd = open("roottemplate", O_RDONLY);
	if(tpfd < 0) {
		perror("Couldn't open root template");
		exit(1);
	}
	read(tpfd, root_template, st.st_size);
	root_template[st.st_size] = '\0';
	close(tpfd);
}
