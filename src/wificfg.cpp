#include <stdio.h>
#include "../include/wificfg.h"
#include "../include/common_toupcam.h"

int common_wifi_cmd(int fd, void *pdata, unsigned short usSize)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input is invaild.\n", __func__);
        return ERROR_FAILED;
    }
    unsigned char *pucdata = (unsigned char *)pdata;

    unsigned int usCmd = 0;
    if(0 != *pucdata)
    {
        usCmd = *pucdata | *(pucdata+1);
    }
    else
    {
        usCmd = (unsigned int)*(pucdata+1);
    }
    
    switch(usCmd)
    {
        default:
            return NOTSUPPORT;
    }

    return ERROR_FAILED; 
}

