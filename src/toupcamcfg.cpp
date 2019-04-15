#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "../include/toupcamcfg.h"
#include "../include/common_toupcam.h"
#include "../include/toupcam.h"
#include "../include/socket.h"

int fillresponheader(TOUPCAM_COMMON_RESPON_S *respon)
{
    if(NULL == respon)
    {
        printf("%s: input is invaild.\n", __func__);
        return ERROR_FAILED;
    }
    respon->com.cmd = COMCMD_TOUPCAMCFG;
    sprintf(respon->com.proto, "%s", "proto");
    respon->com.proto[4] = 'o';
    respon->com.type = TCP_RESPONSE;
    respon->com.size = INVAILD_BUFF_SIZE;
    respon->cc = ERROR_SUCCESS;
    respon->pdata = NULL;
}

int m_memcopy(void* des, void* src, unsigned int size)
{
    if(NULL == des || NULL == src)
    {
        return -1;
    }
    char *pdes = (char *)des;
    char *psrc = (char *)src;
    while(size--)
    {
        *pdes++ = *psrc++;
    }
//    printf("sw %d\n", *(char *)des);
//    printf("sw %d\n", *(char *)src);
    if(0 == size)
        return 0;
    else
        return -1;
}

static int snapshot(int fd)
{
    if(fd < 0)
    {
        printf("socket fd is invaild.\n");
        return ERROR_FAILED;
    }
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    int iTotalSize = 0;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_SNAPPICTURE;
    stToupcamRespon.com.size = INVAILD_BUFF_SIZE;

    if(!g_pstTouPcam->m_hcam)
    {
        printf("toupcam->m_hcam is invaild.\n");
        return ERROR_FAILED;
    }
    HRESULT hr = Toupcam_Snap(g_pstTouPcam->m_hcam, 1); /* 抓拍当前2048x2044 */
    if (FAILED(hr))
    {
        printf("picture arrived failly(%lld)!\n", hr);
        goto _exit1;
    }
    else
    {
        //printf("picture arrived!\n");
        while(!g_pStaticImageDataFlag);

        pthread_mutex_lock(&g_PthreadMutexJpgDest);
        if(g_pstTouPcam->inMaxWidth > 0 && g_pstTouPcam->inMaxHeight > 0)
        {
            iTotalSize = TDIBWIDTHBYTES(g_pstTouPcam->inMaxWidth * 24) * g_pstTouPcam->inMaxHeight;
            char *pBuffer = (char *)malloc(iTotalSize+12);
            if(NULL == pBuffer)
            {
                perror("stToupcamRespon.pdata");
                goto _exit1;
            }

            memset(pBuffer, 0, iTotalSize+12);
            m_memcopy(pBuffer, &stToupcamRespon, 12);
            strncat(pBuffer+12, (char *)g_pucJpgDest, iTotalSize);
            //m_memcopy(pBuffer+12, g_pucJpgDest, iTotalSize);
            int j = 0;
            for(j; j < 32; j++)
            {
                printf("%#x ", g_pucJpgDest[j]);
            }
            printf("\n");
            printf("%d %d\n", *((char *)&stToupcamRespon), *((char *)(&stToupcamRespon+1)));
            for(j=0; j < 32; j++)
            {
                printf("%#x ", (char)pBuffer[j]);
            }
            printf("\n");

            stToupcamRespon.pdata = (char *)pBuffer; 

#if 1
            if(iTotalSize > TCP_BUFFERSIZE)
            {
                stToupcamRespon.com.size = TCP_BUFFERSIZE;
            }
            else
            {
                stToupcamRespon.com.size = iTotalSize;
            }
#endif
            int isize;
            if(iTotalSize > TCP_BUFFERSIZE)
            {
                isize = iTotalSize;
            }
            else
            {
                isize = iTotalSize;
            }
            printf("align:%lld\n", iTotalSize);
            while(1)
            {
                if(iTotalSize <= 0)
                {
                    break;
                }

                //stToupcamRespon.com.size = iTotalSize>TCP_BUFFERSIZE?TCP_BUFFERSIZE:iTotalSize;
                stToupcamRespon.com.size = iTotalSize;
                isize = iTotalSize>TCP_BUFFERSIZE?TCP_BUFFERSIZE:iTotalSize;
                
                //send(fd, &stToupcamRespon, sizeof(TOUPCAM_COMMON_RESPON_S) - 4, 0);
                send(fd, pBuffer, iTotalSize+12, 0);
                //send(fd, (void *)(stToupcamRespon.pdata), stToupcamRespon.com.size, 0);
                //send(fd, (void *)(stToupcamRespon.pdata), iTotalSize, 0);
                    //send(fd, g_pucJpgDest, isize, 0);
                                
                //stToupcamRespon.pdata += iTotalSize>TCP_BUFFERSIZE?TCP_BUFFERSIZE:iTotalSize-1;

                //iTotalSize -= iTotalSize>TCP_BUFFERSIZE?TCP_BUFFERSIZE:iTotalSize;
                iTotalSize -= iTotalSize;
            }
            free(pBuffer);
            free(g_pucJpgDest);
            g_pucJpgDest = NULL;
            pBuffer = NULL;
            stToupcamRespon.pdata = NULL;
        }

        /* 发送结束标志，size=0 */
        //stToupcamRespon.com.size = END_BUFF_SIZE;
        //send(fd, &stToupcamRespon, sizeof(TOUPCAM_COMMON_RESPON_S)- sizeof(char*), 0);

        g_pStaticImageDataFlag = 0;        
        pthread_mutex_unlock(&g_PthreadMutexJpgDest);
    }

    return ERROR_SUCCESS;

_exit1:
    stToupcamRespon.cc = ERROR_FAILED;
    send(fd, &stToupcamRespon, sizeof(TOUPCAM_COMMON_RESPON_S), 0);

    return ERROR_FAILED;
}

int common_toupcam_cmd(int fd, void *pdata, unsigned short usSize)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input is invaild.\n", __func__);
        return ERROR_FAILED;
    }
    unsigned char *pucData = (unsigned char*)pdata;

    unsigned int usCmd = 0;
    if(0 != *pucData)
    {
        usCmd = *pucData | *(pucData+1);
    }
    else
    {
        usCmd = (unsigned int)*(pucData+1);
    }
    switch(usCmd)
    {
        case CMD_SNAPPICTURE:
            return snapshot(fd);
        case CMD_BLACKRORATOR:
            break;
        case CMD_ZONEEXPO:
            break;
        case CMD_BRIGHTNESSTYPE:
            break;
        case CMD_BRIGHTNESS:
            break;
        case CMD_EXPOTYPE:
            break;
        case CMD_EXPO:
            break;
        case CMD_CONTRASTTYPE:
            break;
        case CMD_CONTRAST:
            break;
        case CMD_HISTOGRAMTYPE:
            break;
        case CMD_HISTOGRAM:
            break;
        default:
            return NOTSUPPORT;
    }

    return ERROR_FAILED;
}


