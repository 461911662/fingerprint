#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../include/socket.h"
#include "../include/wificfg.h"
#include "../include/common_toupcam.h"

static int setudpaddr(int fd, void *pdata)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input param is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    int iRet = 0;
    int iTotalSize = 0;
    unsigned int uiIPV4Def = 0;
    unsigned int uiIPV4Addr = 0;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.cmd = COMCMD_WIFICFG;
    stToupcamRespon.com.subcmd = CMD_SETUDPADDR;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(!g_pstTouPcam->m_hcam)
    {
        printf("toupcam->m_hcam is invaild.\n");
        return ERROR_FAILED;
    }

    if(TCP_REQUEST != pstToupcamReq->com.type)
    {
        printf("IPV6 is not support yet.\n");
        goto _exit0;
    }

#ifdef IPV4
    if(!iEndianness)
    {
        uiIPV4Addr = BIGLITTLESWAP32(pstToupcamReq->data.ipv4);
    }
    else
    {
        uiIPV4Addr = pstToupcamReq->data.ipv4;
    }
#elif IPV6
        /* printf("IPV6 is not support yet.\n"); */
        goto _exit0;
#endif

    uiIPV4Def = inet_addr(CLIENT_IP);
    if(uiIPV4Def == sock->cliaddr->sin_addr.s_addr)
    {
        if(uiIPV4Def == uiIPV4Addr)
        {
            stToupcamRespon.cc = ERROR_WIFI_ADDR_EXIST;
        }
        else
        {
            sock->cliaddr->sin_addr.s_addr = uiIPV4Addr;
            stToupcamRespon.cc = ERROR_SUCCESS;
        }
    }
    else
    {
        stToupcamRespon.cc = ERROR_WIFI_ADDR_HOLD;
        sock->cliaddr->sin_addr.s_addr = uiIPV4Def;
    }

    stToupcamRespon.com.size[0] = COMMON_BUFF_SIZE;
    iRet = send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE, 0);
    if(iRet <= 0)
    {
        printf("socket (%d) send failed!\n", fd);
        goto _exit0;
    }
    
    return ERROR_SUCCESS;

_exit0:
    stToupcamRespon.cc = ERROR_FAILED;
    send(fd, &stToupcamRespon, sizeof(TOUPCAM_COMMON_RESPON_S), 0);

    return ERROR_FAILED;

}


int common_wifi_cmd(int fd, void *pdata, unsigned short usSize)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input is invaild.\n", __func__);
        return ERROR_FAILED;
    }
    
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;
    unsigned int usCmd = 0;
    if(!iEndianness)
    {
        usCmd = BIGLITTLESWAP32(pstToupcamReq->com.subcmd);
    }
    else
    {
        usCmd = pstToupcamReq->com.subcmd;
    }
    
    switch(usCmd)
    {
        case CMD_SETUDPADDR:
            return setudpaddr(fd, pstToupcamReq);
        default:
            return NOTSUPPORT;
    }

    return ERROR_FAILED; 
}

