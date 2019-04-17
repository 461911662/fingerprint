#ifndef _WIFICFG_H_
#define _WIFICFG_H_
#include "../include/common_toupcam.h"

extern int common_wifi_cmd(int fd, void *pdata, unsigned short usSize);
extern int fillresponheader(TOUPCAM_COMMON_RESPON_S *respon);

#endif