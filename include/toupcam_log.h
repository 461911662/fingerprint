#ifndef _TOUPCAM_LOG_H_
#define _TOUPCAM_LOG_H_
#include <stdarg.h>

enum LOG_LEVEL
{
	LOG_DEBUG,
	LOG_INFO = 0x01,
	LOG_WARNNING = 0x02,
	LOG_ERROR = 0x04,
};

void _toupcam_err(const char *tag, int level, const char *fmt, const char *fname, ...);
void _toupcam_log(const char *tag, int level, const char *fmt, const char *fname, ...);

#define toupcam_log(level, fmt, ...) _toupcam_log(MODULE_TAG, level, fmt, NULL, ## __VA_ARGS__)
#define toupcam_err(level, fmt, ...) _toupcam_err(MODULE_TAG, level, fmt, NULL, ## __VA_ARGS__)

#define _toupcam_dbg(debug, flag, fmt, ...) \
				do { \
					if(debug & flag) \
						toupcam_log(debug&flag, fmt, ## __VA_ARGS__); \
				} while (0)

#define toupcam_dbg(flag, fmt, ...) _toupcam_dbg(toupcam_debug, flag, fmt, ## __VA_ARGS__)

#define toupcam_log_f(level, fmt, ...) _toupcam_log(MODULE_TAG, level, fmt, __FUNCTION__, ## __VA_ARGS__)
#define toupcam_err_f(level, fmt, ...) _toupcam_log(MODULE_TAG, level, fmt, __FUNCTION__, ## __VA_ARGS__)
#define _toupcam_dbg_f(debug, flag, fmt, ...) \
					do { \
						if (debug & flag) \
							toupcam_log_f(debug&flag, fmt, ## __VA_ARGS__); \
					   } while (0)

#define toupcam_dbg_f(flag, fmt, ...) _toupcam_dbg_f(toupcam_debug, flag, fmt, ## __VA_ARGS__)

extern unsigned int toupcam_debug;
extern int init_toupcam_log();

#endif
