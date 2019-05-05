#include "../include/toupcam_log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define TOUPCAM_LOG_MAX_LEN 256
#define LINE_SZ 1024
#define MODULE_TAG "toupcam"
typedef void (*toupcam_log_callback)(const char*, const char*, va_list);

unsigned int toupcam_debug = 1;

static const char *msg_log_warning = "log message is long\n";
static const char *msg_log_nothing = "\n";

static void os_log(const char* tag, const char *msg, va_list list)
{
	char line[LINE_SZ] = {0};
	snprintf(line, sizeof(line), "%s: %s", tag, msg);
	vfprintf(stdout, line, list);
}

static void os_err(const char* tag, const char *msg, va_list list)
{
	char line[LINE_SZ] = {0};
	snprintf(line, sizeof(line), "%s: %s", tag, msg);
	vfprintf(stderr, line, list);
}

static void __toupcam_log(toupcam_log_callback func, int level, const char *tag, const char *fmt,
						const char *fname, va_list args)
{
	char msg[TOUPCAM_LOG_MAX_LEN + 1];
	char *tmp = msg;
	const char *buf = fmt;
	size_t len_fmt = strnlen(fmt, TOUPCAM_LOG_MAX_LEN);
	size_t len_name = (fname) ? (strnlen(fname, TOUPCAM_LOG_MAX_LEN)) : (0);
	size_t buf_left = TOUPCAM_LOG_MAX_LEN;
	size_t len_all = len_fmt + len_name;

	if(NULL == tag)
	{
		tag = MODULE_TAG;
	}

	if(len_name)
	{
		buf = msg;
		buf_left -= snprintf(msg, buf_left, "%s ", fname);
		tmp += len_name + 1;
	}

	if(0 == len_all)
	{
		buf = msg_log_nothing;
	}
	else if(len_all >= TOUPCAM_LOG_MAX_LEN)
	{
		buf_left -= snprintf(tmp, buf_left, "%s", msg_log_warning);
	}
	else
	{
		snprintf(tmp, buf_left, "%s", fmt);
		if(fmt[len_fmt - 1] != '\n')
		{
			tmp[len_fmt] = '\n';
			tmp[len_fmt + 1] = '\0';
		}
		buf = msg;
	}
	
	if(LOG_INFO == level)
	{
		printf("\033[40;32m[%5s] ", "INFO");
	}
	else if(LOG_WARNNING == level)
	{
		printf("\033[40;33m[%5s] ", "WARN");
	}
	else if(LOG_ERROR == level)
	{
		printf("\033[40;31m[%5s] ", "ERROR");
	}
	else if(LOG_DEBUG == level)
	{
		;
	}
	else
	{
		;
	}
	
	func(tag, buf, args);
	printf("\033[0m");
}

void _toupcam_log(const char *tag, int level, const char *fmt, const char *fname, ...)
{
	va_list args;
	va_start(args, fname);
	__toupcam_log(os_log, level, tag, fmt, fname, args);
	va_end(args);
}

void _toupcam_err(const char *tag, int level, const char *fmt, const char *fname, ...)
{
	va_list args;
	va_start(args, fname);
	__toupcam_log(os_err, level, tag, fmt, fname, args);
	va_end(args);
}

int init_toupcam_log()
{
	//toupcam_debug
	char *pcvalue = secure_getenv("toupcam_debug");
	if(NULL != pcvalue)
	{
		toupcam_debug = strtol(pcvalue, NULL, 10);
	}
}


/*
int main()
{
	_toupcam_log(NULL, LOG_ERROR, "%s", "toupcam", "toupcam log...");
	_toupcam_log(NULL, LOG_INFO, "%s", "toupcam", "toupcam log...");
	_toupcam_log(NULL, LOG_WARNNING, "%s", "toupcam", "toupcam log1...");

	toupcam_dbg(1, "%s:%d", "xll qulu", 2);
	toupcam_dbg(1, "%s:%d\n", "xll 2 qulu", 3);
	toupcam_log_f(LOG_INFO, "%s:%d\n", "xll 2 qulu 8", 3);
	return 0;
}
*/
