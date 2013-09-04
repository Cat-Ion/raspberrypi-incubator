#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <microhttpd.h>
#include <regex.h>
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

enum {
	TYPE_STRING,
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_DOUBLE
};

enum {
	VAR_TEMP_AVG,
	VAR_TEMP_SE,
	VAR_TEMP_WANTED,
	VAR_HUM_AVG,
	VAR_HUM_SE,
	VAR_HUM_WANTED,
	VAR_TEMP_PID_P,
	VAR_TEMP_PID_I,
	VAR_TEMP_PID_D,
	VAR_HUM_PID_P,
	VAR_HUM_PID_I,
	VAR_HUM_PID_D
};

struct template {
	int num;
	size_t maxsize;
	union member {
		int type;
		struct {
			int type;
			char *string;
		} c_string;
		struct {
			int type;
			int var;
			char *fmt;
		} c_number;
	} *members;
};

struct MHD_Daemon *daemon;
struct MHD_Response *mhd_404_response,
	*mhd_please_auth_response;
struct template *root = NULL;

struct postinfo {
	float temp, hum;
	float tp, ti, td, hp, hi, hd;
	struct MHD_PostProcessor *pp;
};

static void add_expires_header(struct MHD_Response *res, time_t exptime) {
	char buf[64];
	struct tm tm;

	MHD_add_response_header(res, "Cache-Control", "max-age=10");

	if(gmtime_r(&exptime, &tm) == NULL) {
		return;
	}

	strftime(buf, 64, "%a, %d %b %Y %T GMT", &tm);
	MHD_add_response_header(res, "Expires", buf);
}

static char *format_logs(log_data_t *data, int num, size_t *len) {
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

static struct MHD_Response *gzip_if_possible_buffer(struct MHD_Connection *connection,
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

static int iterate_post(void *cls, enum MHD_ValueKind kind, const char *key, const char *filename, const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size) {
	struct postinfo *pi = cls;
	if(size == 0) {
		return MHD_YES;
	}
	if(!strcmp(key, "temp")) {
		pi->temp = strtof(data, NULL);
	} else if(!strcmp(key, "hum")) {
		pi->hum = strtof(data, NULL);
	}
#if defined(HTTP_CONFIG_PID) && HTTP_CONFIG_PID
	else if(key[2] == '\0') {
		switch(key[0]) {
		case 'T': switch(key[1]) {
			case 'P': pi->tp = strtof(data, NULL); break;
			case 'I': pi->ti = strtof(data, NULL); break;
			case 'D': pi->td = strtof(data, NULL); break;
			}
			break;
		case 'H': switch(key[1]) {
			case 'P': pi->hp = strtof(data, NULL); break;
			case 'I': pi->hi = strtof(data, NULL); break;
			case 'D': pi->hd = strtof(data, NULL); break;
			}
			break;
		}
	}
#endif

	return MHD_YES;
}

static void maybe_resize(void **ptr, size_t size, int *num, int *max) {
	void *tmp;
	if(*num == *max) {
		*max *= 2;
		tmp = realloc(*ptr, *max * size);
		if(!tmp) {
			fprintf(stderr, "Couldn't allocate %ld bytes\n", (long)(*max * size));
			exit(1);
		}
		*ptr = tmp;
	}
}

static void parse_template(struct template *tgt, char *src) {
	char *pos = src,
		*next;
	int max = 8;

	const char *regex = "[{][{]([HT]_AVG|[HT]_SE|[HT]_WANTED|[HT]_PID_[PID]):(%[-#0 +']*[0-9]*[.][0-9]*[hlL]?[dieEfFgG])}}";
	regex_t re;

	if(regcomp(&re, regex, REG_EXTENDED) != 0) {
		fprintf(stderr, "Can't compile regex: %d\n", regcomp(&re, regex, REG_EXTENDED));
		exit(1);
	}

	tgt->num = 0;
	tgt->maxsize = 0;
	tgt->members = calloc(max, sizeof(union member));
	
	for(pos = src; *pos; pos = next) {
		size_t nmatch = 8;
		regmatch_t pmatch[8];

		if(regexec(&re, pos, nmatch, pmatch, 0) != 0) {
			break;
		}

		if(pmatch[0].rm_so > 0) {
			maybe_resize((void **)&tgt->members, sizeof(union member), &tgt->num, &max);
			tgt->members[tgt->num].c_string.type = TYPE_STRING;
			tgt->members[tgt->num].c_string.string = strndup(pos, pmatch[0].rm_so);
			tgt->num++;
			tgt->maxsize += pmatch[0].rm_so;
		}

		maybe_resize((void **)&tgt->members, sizeof(union member), &tgt->num, &max);
		tgt->members[tgt->num].c_number.fmt  = strndup(pos + pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
		if(!strncmp("T_", pos + pmatch[1].rm_so, 2)) {
			if(!strncmp("AVG", pos + pmatch[1].rm_so + 2, pmatch[1].rm_eo - pmatch[1].rm_so - 2)) {
				tgt->members[tgt->num].c_number.type = TYPE_FLOAT;
				tgt->members[tgt->num].c_number.var  = VAR_TEMP_AVG;
			} else if(!strncmp("SE", pos + pmatch[1].rm_so + 2, pmatch[1].rm_eo - pmatch[1].rm_so - 2)) {
				tgt->members[tgt->num].c_number.type = TYPE_FLOAT;
				tgt->members[tgt->num].c_number.var  = VAR_TEMP_SE;
			} else if(!strncmp("WANTED", pos + pmatch[1].rm_so + 2, pmatch[1].rm_eo - pmatch[1].rm_so - 2)) {
				tgt->members[tgt->num].c_number.type = TYPE_FLOAT;
				tgt->members[tgt->num].c_number.var  = VAR_TEMP_WANTED;
			} else if(!strncmp("PID_P", pos + pmatch[1].rm_so + 2, pmatch[1].rm_eo - pmatch[1].rm_so - 2)) {
				tgt->members[tgt->num].c_number.type = TYPE_FLOAT;
				tgt->members[tgt->num].c_number.var  = VAR_TEMP_PID_P;
			} else if(!strncmp("PID_I", pos + pmatch[1].rm_so + 2, pmatch[1].rm_eo - pmatch[1].rm_so - 2)) {
				tgt->members[tgt->num].c_number.type = TYPE_FLOAT;
				tgt->members[tgt->num].c_number.var  = VAR_TEMP_PID_I;
			} else if(!strncmp("PID_D", pos + pmatch[1].rm_so + 2, pmatch[1].rm_eo - pmatch[1].rm_so - 2)) {
				tgt->members[tgt->num].c_number.type = TYPE_FLOAT;
				tgt->members[tgt->num].c_number.var  = VAR_TEMP_PID_I;
			} else {
				fprintf(stderr, "Invalid format string\n");
				exit(1);
			}
		} else if(!strncmp("H_", pos + pmatch[1].rm_so, 2)) {
			if(!strncmp("AVG", pos + pmatch[1].rm_so + 2, pmatch[1].rm_eo - pmatch[1].rm_so - 2)) {
				tgt->members[tgt->num].c_number.type = TYPE_FLOAT;
				tgt->members[tgt->num].c_number.var  = VAR_HUM_AVG;
			} else if(!strncmp("SE", pos + pmatch[1].rm_so + 2, pmatch[1].rm_eo - pmatch[1].rm_so - 2)) {
				tgt->members[tgt->num].c_number.type = TYPE_FLOAT;
				tgt->members[tgt->num].c_number.var  = VAR_HUM_SE;
			} else if(!strncmp("WANTED", pos + pmatch[1].rm_so + 2, pmatch[1].rm_eo - pmatch[1].rm_so - 2)) {
				tgt->members[tgt->num].c_number.type = TYPE_FLOAT;
				tgt->members[tgt->num].c_number.var  = VAR_HUM_WANTED;
			} else if(!strncmp("PID_P", pos + pmatch[1].rm_so + 2, pmatch[1].rm_eo - pmatch[1].rm_so - 2)) {
				tgt->members[tgt->num].c_number.type = TYPE_FLOAT;
				tgt->members[tgt->num].c_number.var  = VAR_HUM_PID_P;
			} else if(!strncmp("PID_I", pos + pmatch[1].rm_so + 2, pmatch[1].rm_eo - pmatch[1].rm_so - 2)) {
				tgt->members[tgt->num].c_number.type = TYPE_FLOAT;
				tgt->members[tgt->num].c_number.var  = VAR_HUM_PID_I;
			} else if(!strncmp("PID_D", pos + pmatch[1].rm_so + 2, pmatch[1].rm_eo - pmatch[1].rm_so - 2)) {
				tgt->members[tgt->num].c_number.type = TYPE_FLOAT;
				tgt->members[tgt->num].c_number.var  = VAR_HUM_PID_I;
			} else {
				fprintf(stderr, "Invalid format string\n");
				exit(1);
			}
		}
		tgt->num++;
		tgt->maxsize += 32;
		next = pos + pmatch[0].rm_eo;
	}
	regfree(&re);
	if(*pos) {
		maybe_resize((void **)&tgt->members, sizeof(union member), &tgt->num, &max);
		tgt->members[tgt->num].c_string.type = TYPE_STRING;
		tgt->members[tgt->num].c_string.string = strdup(pos);
		tgt->num++;
		tgt->maxsize += strlen(pos);
	}
}

static char *use_template(struct template *tgt) {
	char *ret = malloc(tgt->maxsize);
	char *pos = ret;

	float fv;

	float tp, ti, td, hp, hi, hd;
	pid_getvalues(&tp, &ti, &td, &hp, &hi, &hd);

	for(int i = 0; i < tgt->num; i++) {
		switch(tgt->members[i].type) {
		case TYPE_STRING:
			pos += snprintf(pos, tgt->maxsize - (pos - ret), "%s", tgt->members[i].c_string.string);
			break;
		case TYPE_FLOAT:
			switch(tgt->members[i].c_number.var) {
			case VAR_TEMP_AVG:			fv = avg_temperature; break;
			case VAR_TEMP_SE:			fv = sd_temperature; break;
			case VAR_TEMP_WANTED:		fv = wanted_temperature; break;
			case VAR_HUM_AVG:			fv = avg_humidity; break;
			case VAR_HUM_SE:			fv = sd_humidity; break;
			case VAR_HUM_WANTED:		fv = wanted_humidity; break;
			case VAR_TEMP_PID_P:		fv = tp; break;
			case VAR_TEMP_PID_I:		fv = ti; break;
			case VAR_TEMP_PID_D:		fv = td; break;
			case VAR_HUM_PID_P:			fv = hp; break;
			case VAR_HUM_PID_I:			fv = hi; break;
			case VAR_HUM_PID_D:			fv = hd; break;
			default:					fv = 0; break;
			}
			pos += snprintf(pos, tgt->maxsize - (pos - ret),
						   tgt->members[i].c_number.fmt,
						   fv);
			break;
		case TYPE_INT:
			break;
		case TYPE_DOUBLE:
			break;
		}
	}
	return ret;
}

static int handle_conn(void *cls, struct MHD_Connection *connection, 
                       const char *url, 
                       const char *method, const char *version, 
                       const char *upload_data, 
                       size_t *upload_data_size, void **con_cls) {
	struct MHD_Response *mhd_response;
	int ret = MHD_NO;

	if(!strcmp(url, "/")) {
		char *response = use_template(root);
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

	else if(!strcmp(url, "/log.json")) {
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

	else if(!strcmp(url, "/daylog.json")) {
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

	else if(!strcmp(url, "/set")) {
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
			pi->tp = pi->ti = pi->td = pi->hp = pi->hi = pi->hd = NAN;
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

		if(pi->temp == NAN || pi->hum == NAN
#if defined(HTTP_CONFIG_PID) && HTTP_CONFIG_PID
		   || pi->tp == NAN || pi->ti == NAN || pi->td == NAN
		   || pi->hp == NAN || pi->hi == NAN || pi->hd == NAN
#endif
		   ) {
			return MHD_NO;
		}

		float f_temp = pi->temp,
			f_hum = pi->hum;
#if defined(HTTP_CONFIG_PID) && HTTP_CONFIG_PID
		pid_setvalues(pi->tp, pi->ti, pi->td,
					  pi->hp, pi->hi, pi->hd);
#endif
		free(pi);

		if(f_temp >= TEMP_MIN && f_temp <= TEMP_MAX &&
		   f_hum  >= HUM_MIN && f_hum  <= HUM_MAX) {
			wanted_temperature = f_temp;
			wanted_humidity = f_hum;
		}

		persistent_write();

		mhd_response = MHD_create_response_from_data(0, NULL, MHD_NO, MHD_NO);
		ret = MHD_queue_response(connection, MHD_HTTP_FOUND, mhd_response);
		MHD_add_response_header(mhd_response, "Location", "/");
		MHD_destroy_response(mhd_response);
		return ret;
	}

	else {
		ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, mhd_404_response);
		return ret;
	}
}

/* Exposed functions start here */
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
}

void httpd_reload() {
	char *root_template = NULL;

	struct stat st;
	stat("roottemplate", &st);
	root_template = malloc(st.st_size + 1);
	int tpfd = open("roottemplate", O_RDONLY);
	if(tpfd < 0) {
		perror("Couldn't open root template");
		exit(1);
	}
	read(tpfd, root_template, st.st_size);
	root_template[st.st_size] = '\0';
	close(tpfd);

	if(root != NULL) {
		for(int i = 0; i < root->num; i++) {
			switch(root->members[i].type) {
			case TYPE_STRING:
				free(root->members[i].c_string.string);
				break;
			case TYPE_INT:
			case TYPE_FLOAT:
			case TYPE_DOUBLE:
				free(root->members[i].c_number.fmt);
				break;
			}
		}
		root->num = 0;
		free(root->members);
		free(root);
	}
	root = calloc(1, sizeof(struct template));

	parse_template(root, root_template);
	free(root_template);
}
