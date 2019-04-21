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

/* unsigned char g_pcData[425984+1] = {0}; */

static int blackrorator(int fd, void *pdata)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input param is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    HRESULT hr;
    int iRet = 0;
    int *pibNegative = NULL;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_BLACKRORATOR;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(!g_pstTouPcam->m_hcam)
    {
        printf("%s: toupcam->m_hcam is invaild.\n", __func__);
        return ERROR_FAILED;
    }
    
    if(TCP_REQUEST != pstToupcamReq->com.type)
    {
        goto _exit0;
    }

    /* lock */
    pthread_mutex_lock(&g_PthreadMutexMonitor);
    pibNegative = &g_pstTouPcam->bNegative;
    hr = Toupcam_get_Negative(g_pstTouPcam->m_hcam, pibNegative);
    if (FAILED(hr))
    {
        printf("toupcam get Negative failed!\n");
        goto _exit0;
    }
    Toupcam_put_Negative(g_pstTouPcam->m_hcam, (*pibNegative)^1);
    if (FAILED(hr))
    {
        printf("toupcam put (%d) Negative failed!\n", (*pibNegative)^1);
        goto _exit0;
    }
    *pibNegative = (*pibNegative)^1;
    /* unlock */
    pthread_mutex_unlock(&g_PthreadMutexMonitor);

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
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE, 0);

    return ERROR_FAILED;

}

static int snapshot(int fd, void *pdata)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input param is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    HRESULT hr;
	int isize = 0;
    int iTotalSize = 0;
    char *pBuffer = NULL;
    char * pucJpgDest = NULL;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_SNAPPICTURE;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(TCP_REQUEST != pstToupcamReq->com.type)
    {
        goto _exit0;
    }
	
    if(!g_pstTouPcam->m_hcam)
    {
        printf("%s: toupcam->m_hcam is invaild.\n", __func__);
        return ERROR_FAILED;
    }
    hr = Toupcam_Snap(g_pstTouPcam->m_hcam, 1); /* 抓拍当前2048x2044 */
    if (FAILED(hr))
    {
        printf("picture arrived failly(%lld)!\n", hr);
        goto _exit0;
    }
    else
    {
        /* printf("picture arrived!\n"); */
        while(!g_pStaticImageDataFlag);
        pthread_mutex_lock(&g_PthreadMutexJpgDest);
        if(g_pstTouPcam->inMaxWidth > 0 && g_pstTouPcam->inMaxHeight > 0)
        {
            /* iTotalSize = TDIBWIDTHBYTES(g_pstTouPcam->inMaxWidth * 24) * g_pstTouPcam->inMaxHeight; */
            iTotalSize = giJpgSize;
            pBuffer = (char *)malloc(TCP_BUFFERSIZE+TOUPCAM_COMMON_RESPON_HEADER_SIZE);
            if(NULL == pBuffer)
            {
                perror("stToupcamRespon.pdata");
                goto _exit0;
            }

			pucJpgDest = (char *)g_pucJpgDest;
            /*
            FILE *fp = fopen("myjpeg.jpeg", "ab+");
            if(NULL == fp)
                goto _exit1;
            */

            memset(pBuffer, 0, TCP_BUFFERSIZE+TOUPCAM_COMMON_RESPON_HEADER_SIZE);
            stToupcamRespon.com.size[0] = COMMON_BUFF_SIZE;
            stToupcamRespon.com.size[1] = iTotalSize;
            m_memcopy(pBuffer, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE);
            send(fd, pBuffer, TOUPCAM_COMMON_RESPON_HEADER_SIZE, 0);
            
            while(1)
            {
                if(iTotalSize <= 0)
                {
                    break;
                }

				isize = iTotalSize>TCP_BUFFERSIZE?TCP_BUFFERSIZE:iTotalSize;
				stToupcamRespon.com.size[0] = isize;
                
				memset(pBuffer, 0, isize);
				m_memcopy(pBuffer, pucJpgDest, isize);

				send(fd, pBuffer, isize, 0);
                /* fwrite(pBuffer, isize, 1, fp); */
    
				pucJpgDest += isize;
				iTotalSize -= isize;
            }
            /* fclose(fp); */
            free(pBuffer);
            free(g_pucJpgDest);
			pBuffer = NULL;
            giJpgSize = 0;
            g_pucJpgDest = NULL;
        }

        /* usleep(100000); */
        /* 发送结束标志，size=0 */
        /* stToupcamRespon.com.size[0] = END_BUFF_SIZE; */
        /* send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE, 0); */

        g_pStaticImageDataFlag = 0;        
        pthread_mutex_unlock(&g_PthreadMutexJpgDest);
    }

    return ERROR_SUCCESS;

_exit0:
    stToupcamRespon.cc = ERROR_FAILED;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE, 0);

    return ERROR_FAILED;
}

static int setbrightnesstype(int fd, void *pdata)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input param is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    HRESULT hr;
    int iAutotype = 0;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_BRIGHTNESSTYPE;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(TCP_REQUEST != pstToupcamReq->com.type)
    {
        goto _exit0;
    }

    if(!g_pstTouPcam->m_hcam)
    {
        printf("%s: toupcam->m_hcam is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    pthread_mutex_lock(&g_pstTouPcam->stTexpo.mutex);
    if(g_pstTouPcam->stTexpo.bAutoExposure != pstToupcamReq->data.brightnesstype)
    {
        g_pstTouPcam->stTexpo.bAutoExposure = pstToupcamReq->data.brightnesstype;
        /* Toupcam_put_Brightness(g_pstTouPcam->m_hcam, g_pstTouPcam->stTexpo.bAutoExposure); */
        Toupcam_put_AutoExpoEnable(g_pstTouPcam->m_hcam, g_pstTouPcam->stTexpo.bAutoExposure);
        Toupcam_get_AutoExpoEnable(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.bAutoExposure);
        printf("cur expo mode:%s\n", g_pstTouPcam->stTexpo.bAutoExposure?"auto":"manu");
        if(g_pstTouPcam->stTexpo.bAutoExposure != pstToupcamReq->data.brightnesstype)
        {
            pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);
            goto _exit0;
        }
    }
    else
    {
        printf("cur expo mode:%s\n", g_pstTouPcam->stTexpo.bAutoExposure?"auto":"manu");
    }
    stToupcamRespon.data.brightness = g_pstTouPcam->stTexpo.bAutoExposure;

    pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);

    stToupcamRespon.com.size[0] = COMMON_BUFF_SIZE+1;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+1, 0);

    return ERROR_SUCCESS;

_exit0:
    stToupcamRespon.cc = ERROR_FAILED;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+1, 0);

    return ERROR_FAILED;

}

static int setbrightness(int fd, void *pdata)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input param is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    HRESULT hr;
    unsigned short usBrightness = 0;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_BRIGHTNESS;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(TCP_REQUEST != pstToupcamReq->com.type)
    {
        goto _exit0;
    }

    if(!g_pstTouPcam->m_hcam)
    {
        printf("%s: toupcam->m_hcam is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    if(!iEndianness)
    {
        usBrightness = BIGLITTLESWAP16(pstToupcamReq->data.brightness);
    }
    else
    {
        usBrightness = pstToupcamReq->data.brightness;
    }

    if(g_pstTouPcam->stTexpo.AnMin < usBrightness || g_pstTouPcam->stTexpo.AnMax > usBrightness)
    {
        printf("more than the range of brightness.\n");
        goto _exit0;
    }

    pthread_mutex_lock(&g_pstTouPcam->stTexpo.mutex);
    if(0x01 == g_pstTouPcam->stTexpo.bAutoExposure)
    {
        g_pstTouPcam->stTexpo.AGain = usBrightness;
        hr = Toupcam_put_ExpoAGain(g_pstTouPcam->m_hcam, g_pstTouPcam->stTexpo.AGain);
        if (FAILED(hr))
        {
            printf("set brightness failly(%lld)!\n", hr);
            pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);
            goto _exit0;
        }
        hr = Toupcam_get_ExpoAGain(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.AGain);
        if (FAILED(hr))
        {
            printf("set brightness failly(%lld)!\n", hr);
            pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);
            goto _exit0;
        }

        stToupcamRespon.data.brightness = g_pstTouPcam->stTexpo.AGain;
        stToupcamRespon.com.size[0] = COMMON_BUFF_SIZE+2;
    }
    else
    {
        printf("not set brightness,because of this is manu mode!!!\n");
        pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);
        goto _exit0;
    }
    pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);

    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+2, 0);

    return ERROR_SUCCESS;    

_exit0:
    stToupcamRespon.cc = ERROR_FAILED;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+2, 0);

    return ERROR_FAILED;
}

static int setexpotype(int fd, void *pdata)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input param is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    HRESULT hr;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_EXPOTYPE;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(TCP_REQUEST != pstToupcamReq->com.type)
    {
        goto _exit0;
    }

    if(!g_pstTouPcam->m_hcam)
    {
        printf("%s: toupcam->m_hcam is invaild.\n", __func__);
        return ERROR_FAILED;
    }
    
    pthread_mutex_lock(&g_pstTouPcam->stTexpo.mutex);
    if(g_pstTouPcam->stTexpo.bAutoExposure != pstToupcamReq->data.expotype)
    {
        g_pstTouPcam->stTexpo.bAutoExposure = pstToupcamReq->data.brightnesstype;
        Toupcam_put_AutoExpoEnable(g_pstTouPcam->m_hcam, g_pstTouPcam->stTexpo.bAutoExposure);
        Toupcam_get_AutoExpoEnable(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.bAutoExposure);
        printf("cur expo mode:%s\n", g_pstTouPcam->stTexpo.bAutoExposure?"auto":"manu");
        if(g_pstTouPcam->stTexpo.bAutoExposure != pstToupcamReq->data.brightnesstype)
        {
            pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);
            goto _exit0;
        }
    }
    else
    {
        printf("cur expo mode:%s\n", g_pstTouPcam->stTexpo.bAutoExposure?"auto":"manu");
    }
    stToupcamRespon.data.expotype = g_pstTouPcam->stTexpo.bAutoExposure;
    pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);

    stToupcamRespon.com.size[0] = COMMON_BUFF_SIZE+1;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+1, 0);
    return ERROR_SUCCESS;   

_exit0:
    stToupcamRespon.cc = ERROR_FAILED;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+1, 0);

    return ERROR_FAILED;
}

static int setexpo(int fd, void *pdata)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input param is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    HRESULT hr;
    unsigned short usExpo = 0;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_EXPO;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(TCP_REQUEST != pstToupcamReq->com.type)
    {
        goto _exit0;
    }

    if(!g_pstTouPcam->m_hcam)
    {
        printf("%s: toupcam->m_hcam is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    if(!iEndianness)
    {
        usExpo = BIGLITTLESWAP16(pstToupcamReq->data.expo);
    }
    else
    {
        usExpo = pstToupcamReq->data.expo;
    }

    pthread_mutex_lock(&g_pstTouPcam->stTexpo.mutex);
    if(!g_pstTouPcam->stTexpo.bAutoExposure)
    {        
        hr = Toupcam_put_AutoExpoTarget(g_pstTouPcam->m_hcam, usExpo);
        if (FAILED(hr))
        {
            printf("set brightness failly(%lld)!\n", hr);
            pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);
            goto _exit0;
        }
        hr = Toupcam_get_AutoExpoTarget(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.AutoTarget);
        if (FAILED(hr))
        {
            printf("get brightness failly(%lld)!\n", hr);
            pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);
            goto _exit0;
        }
        
        stToupcamRespon.data.expo = g_pstTouPcam->stTexpo.AutoTarget;
        stToupcamRespon.com.size[0] = COMMON_BUFF_SIZE + 2;
    }
    else
    {
        printf("not set expo,because of this is manu mode!!!\n");
        pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);
        goto _exit0;
    }

    pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);

    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+2, 0);

    return ERROR_SUCCESS;    


_exit0:
    stToupcamRespon.cc = ERROR_FAILED;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+2, 0);

    return ERROR_FAILED;    
}

static int setcontrasttype(int fd, void *pdata)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input param is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    HRESULT hr;
    unsigned short usExpo = 0;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_CONTRASTTYPE;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(TCP_REQUEST != pstToupcamReq->com.type)
    {
        goto _exit0;
    }

    if(!g_pstTouPcam->m_hcam)
    {
        printf("%s: toupcam->m_hcam is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    pthread_mutex_lock(&g_pstTouPcam->stTcolor.mutex);
    if(g_pstTouPcam->stTcolor.bAutoColor != pstToupcamReq->data.contrasttype)
    {
        g_pstTouPcam->stTcolor.bAutoColor = pstToupcamReq->data.contrasttype;
    }
    printf("cur contrast mode:%s\n", g_pstTouPcam->stTcolor.bAutoColor?"auto":"manu");
    stToupcamRespon.data.contrasttype = g_pstTouPcam->stTcolor.bAutoColor;
    pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);

    stToupcamRespon.com.size[0] = COMMON_BUFF_SIZE+1;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+1, 0);
    
    return ERROR_SUCCESS;

_exit0:
    stToupcamRespon.cc = ERROR_FAILED;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+1, 0);

    return ERROR_FAILED;     
}

static int setcontrast(int fd, void *pdata)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input param is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    HRESULT hr;
    int iContrast;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_CONTRAST;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(TCP_REQUEST != pstToupcamReq->com.type)
    {
        goto _exit0;
    }

    if(!g_pstTouPcam->m_hcam)
    {
        printf("%s: toupcam->m_hcam is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    if(!iEndianness)
    {
        iContrast = BIGLITTLESWAP32(pstToupcamReq->data.contrast);
    }
    else
    {
        iContrast = pstToupcamReq->data.contrast;
    }    
    
    pthread_mutex_lock(&g_pstTouPcam->stTcolor.mutex);
    if(!g_pstTouPcam->stTcolor.bAutoColor)
    {
        hr = Toupcam_put_Contrast(g_pstTouPcam->m_hcam, iContrast);
        if (FAILED(hr))
        {
            printf("set contrast failly(%lld)!\n", hr);
            pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);
            goto _exit0;
        }
        hr = Toupcam_get_Contrast(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTcolor.Contrast);
        if (FAILED(hr))
        {
            printf("get contrast failly(%lld)!\n", hr);
            pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);
            goto _exit0;
        }

        stToupcamRespon.data.contrast = g_pstTouPcam->stTcolor.Contrast;
        stToupcamRespon.com.size[0] = COMMON_BUFF_SIZE + 4;
    }
    else
    {
        printf("not set contrast,because of this is manu mode!!!\n");
        pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);
        goto _exit0;
    }

    pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);
    
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+4, 0);
    return ERROR_SUCCESS;

_exit0:
    stToupcamRespon.cc = ERROR_FAILED;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+4, 0);

    return ERROR_FAILED;      
}

static int setzoneexpo(int fd, void *pdata)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input param is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    HRESULT hr;
    RECT stExpoRect;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_ZONEEXPO;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(TCP_REQUEST != pstToupcamReq->com.type)
    {
        goto _exit0;
    }

    if(!g_pstTouPcam->m_hcam)
    {
        printf("%s: toupcam->m_hcam is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    if(!iEndianness)
    {
        stExpoRect.left = BIGLITTLESWAP32(pstToupcamReq->data.expozone.left);
        stExpoRect.bottom = BIGLITTLESWAP32(pstToupcamReq->data.expozone.bottom);
        stExpoRect.right = BIGLITTLESWAP32(pstToupcamReq->data.expozone.right);
        stExpoRect.top = BIGLITTLESWAP32(pstToupcamReq->data.expozone.top);
    }
    else
    {
        stExpoRect.left = pstToupcamReq->data.expozone.left;
        stExpoRect.bottom = pstToupcamReq->data.expozone.bottom;
        stExpoRect.right = pstToupcamReq->data.expozone.right;
        stExpoRect.top = pstToupcamReq->data.expozone.top;
    }
    
    if(!g_pstTouPcam->stTexpo.bAutoExposure)
    {
        hr = Toupcam_put_AEAuxRect(g_pstTouPcam->m_hcam, &stExpoRect);
        if (FAILED(hr))
        {
            printf("set expo zone failly(%lld)!\n", hr);
            goto _exit0;
        }
        hr = Toupcam_get_AEAuxRect(g_pstTouPcam->m_hcam, &stExpoRect);
        if (FAILED(hr))
        {
            printf("get expo zone failly(%lld)!\n", hr);
            goto _exit0;
        }
        stToupcamRespon.data.expozone = stExpoRect;
    }
    else
    {
        printf("not set zone expo,because of this is manu mode!!!\n");
        goto _exit0;
    }

    stToupcamRespon.com.size[0] = COMMON_BUFF_SIZE + sizeof(EXPO_RECT_S);
    
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+sizeof(RECT), 0);

    return ERROR_SUCCESS;
    
_exit0:
    stToupcamRespon.cc = ERROR_FAILED;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+sizeof(RECT), 0);

    return ERROR_FAILED;      
}

static int sethistogramtype(int fd, void *pdata)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input param is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    HRESULT hr;
    EXPO_RECT_S stExpoRect;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_HISTOGRAMTYPE;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(TCP_REQUEST != pstToupcamReq->com.type)
    {
        goto _exit0;
    }

    if(!g_pstTouPcam->m_hcam)
    {
        printf("%s: toupcam->m_hcam is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    pthread_mutex_lock(&g_pstTouPcam->stHistoram.mutex);
    if(g_pstTouPcam->stHistoram.bAutoHis != pstToupcamReq->data.historamtype)
    {
        hr = Toupcam_LevelRangeAuto(g_pstTouPcam->m_hcam);
        if(FAILED(hr))
        {
            goto _exit0;
        }
        g_pstTouPcam->stHistoram.bAutoHis = pstToupcamReq->data.historamtype;
        printf("cur contrast mode:%s\n", g_pstTouPcam->stHistoram.bAutoHis?"auto":"manu");
    }
    else
    {
        printf("cur contrast mode:%s\n", g_pstTouPcam->stHistoram.bAutoHis?"auto":"manu");
    }
    pthread_mutex_unlock(&g_pstTouPcam->stHistoram.mutex);
    
    stToupcamRespon.com.size[0] = COMMON_BUFF_SIZE+1;
    stToupcamRespon.data.historamtype = g_pstTouPcam->stHistoram.bAutoHis;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+1, 0);

    return ERROR_SUCCESS;    

_exit0:
    stToupcamRespon.cc = ERROR_FAILED;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+sizeof(EXPO_RECT_S), 0);

    return ERROR_FAILED;  
}

static int sethistogram(int fd, void *pdata)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input param is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    HRESULT hr;
    int i;
    HISTORAM_DATA_S stHistoram;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_HISTOGRAM;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(TCP_REQUEST != pstToupcamReq->com.type)
    {
        goto _exit0;
    }

    if(!g_pstTouPcam->m_hcam)
    {
        printf("%s: toupcam->m_hcam is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    /* big endianness swap */
    for(i=0; i<4; i++)
    {
        if(!iEndianness)
        {
            stHistoram.aLow[i] = BIGLITTLESWAP16(pstToupcamReq->data.historam.aLow[i]);
        }
        else
        {
            stHistoram.aLow[i] = pstToupcamReq->data.historam.aLow[i];
        }
    }    

    pthread_mutex_lock(&g_pstTouPcam->stHistoram.mutex);
    if(!g_pstTouPcam->stHistoram.bAutoHis)
    {
        for(i=0; i<4; i++)
        {
            if(stHistoram.aLow[i] != g_pstTouPcam->stHistoram.aLow[i] || stHistoram.aHigh[i] != g_pstTouPcam->stHistoram.aHigh[i])
            {
                hr = Toupcam_put_LevelRange(g_pstTouPcam->m_hcam, stHistoram.aLow, stHistoram.aHigh);
                if(FAILED(hr))
                {
                    goto _exit0;
                }
                hr = Toupcam_get_LevelRange(g_pstTouPcam->m_hcam, g_pstTouPcam->stHistoram.aLow, g_pstTouPcam->stHistoram.aHigh);
                if(FAILED(hr))
                {
                    goto _exit0;
                }
                memcpy(stToupcamRespon.data.historam.aLow, g_pstTouPcam->stHistoram.aLow, 8);
                memcpy(stToupcamRespon.data.historam.aHigh, g_pstTouPcam->stHistoram.aHigh, 8);
                stToupcamRespon.com.size[0] = COMMON_BUFF_SIZE + 8;
                break;
            }
        }    
        if(4 == i)
        {
            printf("no need to set it.\n");
            goto _exit0;
        }
    }
    else
    {
        printf("not set histogram,because of this is manu mode!!!\n");
        goto _exit0;
    }
    pthread_mutex_unlock(&g_pstTouPcam->stHistoram.mutex);

    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+8, 0);
    return ERROR_SUCCESS;

_exit0:
    stToupcamRespon.cc = ERROR_FAILED;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+8, 0);

    return ERROR_FAILED; 

}

static int syncserverdata(int fd, void *pdata)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input param is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    HRESULT hr;
    int i;
    HISTORAM_DATA_S stHistoram;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_HISTOGRAM;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(TCP_REQUEST != pstToupcamReq->com.type)
    {
        goto _exit0;
    }

    if(!g_pstTouPcam->m_hcam)
    {
        printf("%s: toupcam->m_hcam is invaild.\n", __func__);
        return ERROR_FAILED;
    }

    /* init server data to repson */
    stToupcamRespon.data.totaldata.brightnesstype = g_pstTouPcam->stTexpo.bAutoExposure;
    stToupcamRespon.data.totaldata.brightness = g_pstTouPcam->stTexpo.AGain;
    stToupcamRespon.data.totaldata.expotype = g_pstTouPcam->stTexpo.bAutoExposure;
    stToupcamRespon.data.totaldata.expo = g_pstTouPcam->stTexpo.AutoTarget;
    stToupcamRespon.data.totaldata.contrasttype = g_pstTouPcam->stTcolor.bAutoColor;
    stToupcamRespon.data.totaldata.contrast = g_pstTouPcam->stTcolor.Contrast;
    stToupcamRespon.data.totaldata.historamtype = g_pstTouPcam->stHistoram.bAutoHis;
    for(i=0; i<4; i++)
    {
        stToupcamRespon.data.totaldata.historam.aHigh[i] = g_pstTouPcam->stHistoram.aHigh[i];
        stToupcamRespon.data.totaldata.historam.aLow[i] = g_pstTouPcam->stHistoram.aLow[i];
    }
      
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+sizeof(TOTAL_DATA_S), 0);
    return ERROR_SUCCESS;
    
_exit0:
    stToupcamRespon.cc = ERROR_FAILED;
    send(fd, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE+sizeof(TOTAL_DATA_S), 0);

    return ERROR_FAILED;     
}

int common_toupcam_cmd(int fd, void *pdata, unsigned short usSize)
{
    if(fd < 0 || NULL == pdata)
    {
        printf("%s: input is invaild.\n", __func__);
        return ERROR_FAILED;
    }
    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pdata;

    unsigned short usCmd = 0;
    if(!iEndianness)
    {
        usCmd = BIGLITTLESWAP16(pstToupcamReq->com.subcmd);
    }
    else
    {
        usCmd = pstToupcamReq->com.subcmd;
    }

    switch(usCmd)
    {
        case CMD_SNAPPICTURE:
            return snapshot(fd, pdata);
        case CMD_BLACKRORATOR:
            return blackrorator(fd, pdata);
        case CMD_ZONEEXPO:
            return setzoneexpo(fd, pdata);
        case CMD_BRIGHTNESSTYPE:
            return setbrightnesstype(fd, pdata);
        case CMD_BRIGHTNESS:
            return setbrightness(fd, pdata);
        case CMD_EXPOTYPE:
            return setexpotype(fd, pdata);
        case CMD_EXPO:
            return setexpo(fd, pdata);
        case CMD_CONTRASTTYPE:
            return setcontrasttype(fd, pdata);
        case CMD_CONTRAST:
            return setcontrast(fd, pdata);
        case CMD_HISTOGRAMTYPE:
            return sethistogramtype(fd, pdata);
        case CMD_HISTOGRAM:
            return sethistogram(fd, pdata);
        case CMD_TOTALDATA:
            return syncserverdata(fd, pdata);
        default:
            return NOTSUPPORT;
    }

    return ERROR_FAILED;
}


