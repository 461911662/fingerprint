#ifndef _TOUPCAMCFG_H_
#define _TOUPCAMCFG_H_

typedef struct Toupcam_cfg_RadioButton
{
    unsigned short cmd_id;
    char buttonValue;
}TPUPCAM_CFG_RB_S;


extern int common_toupcam_cmd(int fd, void *pdata, unsigned short usSize);

#endif