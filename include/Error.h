/*
 * Error.h
 *
 *  Created on: Jun 1, 2013
 *      Author: Administrator
 */
 
#ifndef ERROR_H_
#define ERROR_H_
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct Errorinfo
{
    unsigned int errorid;
    char *info;
    char *definfo;
}ERRORINFO_S;

extern char *toupcamerrinfo(unsigned int uiCallbackContext);

int debug(char *, ...);
int fail(char *, ...);
int warn(char *, ...);
 
#endif /* ERROR_H_ */