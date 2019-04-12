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
 
 
int debug(char *, ...);
int fail(char *, ...);
int warn(char *, ...);
 
#endif /* ERROR_H_ */