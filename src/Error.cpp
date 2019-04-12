/*
 * Error.c
 *
 *  Created on: Jun 1, 2013
 *      Author: Administrator
 */
 
#include "../include/Error.h"
 
int debug(char *fmt, ...) {
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return r;
}
 
int warn(char *fmt, ...) {
    int r;
    va_list ap;
    va_start(ap, fmt);
    r = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return r;
}
int fail(char *fmt, ...) {
    int r;
    va_list ap;
    va_start(ap, fmt);
    r = vfprintf(stderr, fmt, ap);
    exit(1);
    /* notreached */
    va_end(ap);
    return r;
}