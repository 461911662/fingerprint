#ifndef _TOUPCAMCFG_H_
#define _TOUPCAMCFG_H_

#include "../include/common_toupcam.h"

typedef struct Toupcam_cfg_RadioButton
{
    unsigned short cmd_id;
    char buttonValue;
}TPUPCAM_CFG_RB_S;

extern unsigned char *g_pucJpgDest;//[1024*1022];
extern unsigned int giJpgSize;

/* 线程锁 */
extern pthread_mutex_t g_PthreadMutexJpgDest;
extern pthread_mutex_t g_PthreadMutexMonitor;

extern int common_toupcam_cmd(int fd, void *pdata, unsigned short usSize);
extern int fillresponheader(TOUPCAM_COMMON_RESPON_S *respon);

#endif