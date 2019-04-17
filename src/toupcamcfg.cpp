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

static int blackrorator(int fd)
{
    if(fd < 0)
    {
        printf("socket fd is invaild.\n");
        return ERROR_FAILED;
    }
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    int iTotalSize = 0;
	int isize = 0;
    int iRet = 0;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_BLACKRORATOR;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

    if(!g_pstTouPcam->m_hcam)
    {
        printf("toupcam->m_hcam is invaild.\n");
        return ERROR_FAILED;
    }

    /* lock */
    pthread_mutex_lock(&g_PthreadMutexMonitor);
    int *pibNegative = &g_pstTouPcam->bNegative;
    HRESULT hr = Toupcam_get_Negative(g_pstTouPcam->m_hcam, pibNegative);
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

    stToupcamRespon.com.size[0] = END_BUFF_SIZE;
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

static int snapshot(int fd)
{
    if(fd < 0)
    {
        printf("socket fd is invaild.\n");
        return ERROR_FAILED;
    }
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    int iTotalSize = 0;
	int isize = 0;

    fillresponheader(&stToupcamRespon);
    stToupcamRespon.com.subcmd = CMD_SNAPPICTURE;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;

	/*
	*debug: foreach send memcpy

	//iTotalSize = 425984 + (425984/TCP_BUFFERSIZE + (425984%TCP_BUFFERSIZE?1:0))*12;
	iTotalSize = 425984;
	printf("iTotalSize:%d\n", iTotalSize);
	char *pBuffer = (char *)malloc(TCP_BUFFERSIZE+12);
	if(NULL == pBuffer)
	{
		perror("stToupcamRespon.pdata");
		return ERROR_FAILED;
	}

	g_pcData[0] = 'a';
	g_pcData[5] = 'b';
	g_pcData[425979] = 'a';
	g_pcData[425983] = 'b';	
	memset(pBuffer, 0, TCP_BUFFERSIZE+12);

	if(iTotalSize > TCP_BUFFERSIZE)
	{
		isize = TCP_BUFFERSIZE;
	}
	else
	{
		isize = iTotalSize;
	}

	char * g_pclData = (char *)g_pcData;

	while(1)
	{
		if(iTotalSize <= 0)
		{
			break;
		}
		
		isize = iTotalSize>TCP_BUFFERSIZE?TCP_BUFFERSIZE:iTotalSize;
		stToupcamRespon.com.size = isize;
		
		memset(pBuffer, 0, isize+TOUPCAM_COMMON_RESPON_HEADER_SIZE);
		m_memcopy(pBuffer, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE);
		m_memcopy(pBuffer+TOUPCAM_COMMON_RESPON_HEADER_SIZE, g_pclData, isize);
		
		send(fd, pBuffer, isize+TOUPCAM_COMMON_RESPON_HEADER_SIZE, 0);
		
		g_pclData += isize;
		iTotalSize -= isize;

	}

	free(pBuffer);
	return ERROR_SUCCESS;
	*/	

    if(!g_pstTouPcam->m_hcam)
    {
        printf("toupcam->m_hcam is invaild.\n");
        return ERROR_FAILED;
    }
    HRESULT hr = Toupcam_Snap(g_pstTouPcam->m_hcam, 1); /* 抓拍当前2048x2044 */
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
            char *pBuffer = (char *)malloc(TCP_BUFFERSIZE+TOUPCAM_COMMON_RESPON_HEADER_SIZE);
            if(NULL == pBuffer)
            {
                perror("stToupcamRespon.pdata");
                goto _exit0;
            }

			char * pucJpgDest = (char *)g_pucJpgDest;
            /*
            FILE *fp = fopen("myjpeg.jpeg", "ab+");
            if(NULL == fp)
                goto _exit1;
            */

            memset(pBuffer, 0, TCP_BUFFERSIZE+TOUPCAM_COMMON_RESPON_HEADER_SIZE);
            stToupcamRespon.com.size[0] = iTotalSize;
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

                /*
				memset(pBuffer, 0, isize+TOUPCAM_COMMON_RESPON_HEADER_SIZE);
				m_memcopy(pBuffer, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE);
				m_memcopy(pBuffer+TOUPCAM_COMMON_RESPON_HEADER_SIZE, pucJpgDest, isize);
                */
                
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

    /*
    if(0 != *pucData)
    {
        usCmd = *pucData | *(pucData+1);
    }
    else
    {
        usCmd = (unsigned int)*(pucData+1);
    }
    */
    switch(usCmd)
    {
        case CMD_SNAPPICTURE:
            return snapshot(fd);
        case CMD_BLACKRORATOR:
            return blackrorator(fd);
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


