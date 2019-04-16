//#include "opencv2/core.hpp"
//#include "opencv2/imgproc.hpp"
//#include "opencv2/highgui.hpp"
//#include "opencv2/videoio.hpp"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include "../include/socket.h"
#include "../include/x264.h"
#include "../include/codeimg.h"
#include "../include/rtp.h"
#include "../include/toupcam.h"
#include "../include/common_toupcam.h"

//using namespace cv;
using namespace std;


struct sockets *sock; /* udp potr:5004 */
struct sockets *sock1; /* tcp potr:5005 */

X264Encoder x264Encoder;

//rtp 包结构
struct rtp_pack *rtp;
//rtp数据结构
//struct rtp_data *pdata;

//创建rtp头
struct rtp_pack_head head;

int sendnum = 0; /* 当前已发rtp数据包数量 */

HToupCam g_hcam = NULL;
void* g_pImageData = NULL;
void* g_pStaticImageData = NULL;
int g_pStaticImageDataFlag = 0; /* 检测静态图片是否捕获完成 */

unsigned g_total = 0;

int frame_num = 0;
int nWidth = 0, nHeight = 0;
int WIDTH = 0, HEIGHT = 0;

/* support for jpeg transmit */
unsigned char *g_pucJpgDest = NULL;//[1024*1022];
int giJpgSize=1024*1022;

/* 线程 */
#define MaxThreadNum      (4)
pthread_t g_PthreadId[MaxThreadNum] = {0};
unsigned int g_PthreadMaxNum = ARRAY_SIZE(g_PthreadId);
pthread_mutex_t g_PthreadMutex_h264;
pthread_mutex_t g_PthreadMutexJpgDest;

extern void Destory_sock(void);

void __stdcall EventCallback(unsigned nEvent, void* pCallbackCtx)
{
    HRESULT hr;
    ToupcamFrameInfoV2 info = { 0 };
    unsigned int *puiCallbackCtx = (unsigned int *)pCallbackCtx;
    *puiCallbackCtx = 0;

    switch(nEvent)
    {
        case TOUPCAM_EVENT_IMAGE:
            memset(g_pImageData, 0, sizeof(g_pstTouPcam->m_header.biSizeImage));
            hr = Toupcam_PullImageV2(g_hcam, g_pImageData, 24, &info);
            
            if (FAILED(hr))
                printf("failed to pull image, hr = %08x\n", hr);
            else
            {
                
                /* After we get the image data, we can do anything for the data we want to do */
                /* printf("pull image ok, total = %u, resolution = %u x %u\n", ++g_total, info.width, info.height); */
                if(frame_num == 0)
                {
                    encode_yuv((unsigned char *)g_pImageData);
                    frame_num = 0;
                }else
                {
                    frame_num++;
                }
            }
            break;
        case TOUPCAM_EVENT_STILLIMAGE:
            printf("toupcam event TOUPCAM_EVENT_STILLIMAGE(%d).\n", nEvent);
            memset(g_pStaticImageData, 0, sizeof(g_pstTouPcam->m_header.biSizeImage));
            hr = Toupcam_PullStillImageV2(g_hcam, g_pStaticImageData, 24, &info);
            if (FAILED(hr))
                printf("failed to pull image, hr = %08x\n", hr);
            else
            {
                /* After we get the image data, we can do anything for the data we want to do */
                printf("pull static image ok, total = %u, resolution = %u x %u\n", ++g_total, info.width, info.height);
                printf("encode.\n");
                encode_jpeg((unsigned char *)g_pStaticImageData);
                /*
                hr = Toupcam_PullStillImage(g_hcam, g_pStaticImageData, 24, NULL, NULL);
                if (SUCCEEDED(hr))
                {
                    printf("encode.\n");
                    encode_jpeg((unsigned char *)g_pStaticImageData);
                }
                else
                {
                    printf("miss static image, total = %u, resolution = %u x %u\n", ++g_total, info.width, info.height);
                }
                */
            }
            break;
        case TOUPCAM_EVENT_EXPOSURE:     /* exposure time changed */
        case TOUPCAM_EVENT_TEMPTINT:     /* white balance changed, Temp/Tint mode */
        case TOUPCAM_EVENT_WBGAIN:       /* white balance changed, RGB Gain mode */
        case TOUPCAM_EVENT_TRIGGERFAIL:  /* trigger failed */
        case TOUPCAM_EVENT_BLACK:        /* black balance changed */
        case TOUPCAM_EVENT_FFC:          /* flat field correction status changed */
        case TOUPCAM_EVENT_DFC:          /* dark field correction status changed */
            //printf("toupcam event: %d\n", nEvent);
            break;
        case TOUPCAM_EVENT_ERROR:        /* generic error */
            *puiCallbackCtx = TOUPCAM_EVENT_ERROR;
            break;
        case TOUPCAM_EVENT_DISCONNECTED: /* camera disconnected */
            *puiCallbackCtx = TOUPCAM_EVENT_DISCONNECTED;
            break;
        case TOUPCAM_EVENT_TIMEOUT:      /* timeout error */
            *puiCallbackCtx = TOUPCAM_EVENT_TIMEOUT;
            break;
        case TOUPCAM_EVENT_FACTORY:      /* restore factory settings */
            *puiCallbackCtx = TOUPCAM_EVENT_FACTORY;
            break;            
        default:
            printf("unknown event: %d\n", nEvent);
            break;
    }
}

void *pthread_server(void *pdata)
{
    int iClientFd, iServerFd, iRet, i;
    unsigned short usSize = 0;
    struct sockaddr stClientaddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if(sock1->local < 0)
    {
        printf("socket fd(%d).\n", sock1->local);
        return NULL;
    }
    iServerFd = sock1->local;

	struct timeval tv = {5, 0};
    fd_set rdfs;
	FD_ZERO(&rdfs);
	FD_SET(iServerFd, &rdfs);
    fd_set tmprdfs = rdfs;
	int iMaxfd = iServerFd;
	while(1)
	{
		tmprdfs = rdfs;
		iRet = select(iMaxfd+1, &tmprdfs, NULL, NULL, &tv);
		if(iRet == 0)
		{
			//printf("select time out.\n");
			continue;
		}
		else if(iRet < 0)
		{
			//printf("select fail.\n");
			continue;
		}
		for(i = 0; i < iMaxfd+1; i++)
		{
			if(FD_ISSET(i, &tmprdfs))
			{
				if(iServerFd == i)
				{
					iClientFd = accept(iServerFd, &stClientaddr, &addrlen);
					if(iClientFd < 0)
					{
						perror("stream accept");
						continue;
					}
					if(iClientFd > iMaxfd)
					{
						iMaxfd = iClientFd;
					}
					FD_SET(iClientFd, &rdfs);
					printf("tcp(%d) listen(%d)...\n", i, iClientFd);
				}
				else
				{
					memset(g_cBuffData, 0, ARRAY_SIZE(g_cBuffData));
					iRet = read(i, g_cBuffData, ARRAY_SIZE(g_cBuffData));
					if(iRet > 0)
					{
						char *pcBuffData = strstr(g_cBuffData, "proto");
						if(NULL == pcBuffData)
						{
							printf("no proto.\n");
							FD_CLR(i, &tmprdfs);
							continue;
						}
						/*
						*debug
						int k;
						for(k=0; k<11; k++)
						{
							printf("%2x ", g_cBuffData[k]);
						}
						printf("\n");
						*/
						usSize = *(pcBuffData+5) | *(pcBuffData+6);
                        /* printf("size:%d\n", usSize); */
						g_ReqResFlag = *(pcBuffData+7);
						common_hander(i, pcBuffData+8, usSize-1);
					}
                    else
                    {
                        printf("sock unconnect...\n");
                        FD_CLR(i, &rdfs);
                        close(i);
                    }
					/* close */
				}
			}
            FD_CLR(i, &tmprdfs);
		}
	}

	return NULL;
}

void *pthread_health_monitor(void *pdata)
{
	
	return NULL;
}

void Destory_sock(void)
{
    if(sock)
    {
        close(sock->local);
        free(sock);
        sock = NULL;
        printf("destory dgram sock...\n");
    }
    if(sock1)
    {
        close(sock1->local);
        free(sock1);
        sock1 = NULL;
        printf("destory stream sock...\n");
    }
}

void Destory_Toupcam(void)
{
    if(g_hcam)
    {
        Toupcam_Close(g_hcam);
        g_hcam = NULL;
    }
    
    if(g_pImageData)
    {
        free(g_pImageData);
        g_pImageData = NULL;
    }
    
    if(g_pStaticImageData)
    {
        free(g_pStaticImageData);
        g_pStaticImageData = NULL;
    }

    if(g_pstTouPcam)
    {
        free(g_pstTouPcam);
        g_pstTouPcam = NULL;
    }
        
}

int init_sock(void)
{
    int iRet = ERROR_SUCCESS;
    
    /* step 1 init dgram */
    iRet = socket_dgram_init();
    if(ERROR_SUCCESS == iRet)
    {
        printf("dgram sock init is ok.\n");
    }
    else
    {
        printf("dgram sock init is not ok.\n");
    }
    /* step 2 init stream */
    iRet = socket_stream_init();
    if(ERROR_SUCCESS == iRet)
    {
        printf("stream sock init is ok.\n");
    }
    else
    {
        printf("stream sock init is not ok.\n");
    }

	return iRet;
}

void pthread_mutex_inits(void)
{
    /* 初始化jpeg线程数据保护锁 */
    pthread_mutex_init(&g_PthreadMutexJpgDest, NULL);
    return;
}

int main(int, char**)
{
	int inWidth = 0, inHeight = 0;
    int iPthredArg = 0;
    int iRet = 0;
	
    /* 初始化sock数据,用于wifi网络视频帧传输和控制协议传输 */
    iRet = init_sock();
    if(ERROR_FAILED == iRet)
    {
        goto exit0_;
    }

    /* 初始化线程锁 */
    pthread_mutex_inits();
    
    /* tcp server thread */
    iRet = pthread_create(&g_PthreadId[0], NULL, pthread_server, (void *)&iPthredArg);

	/* toupcam health's monitor thread */
    iRet = pthread_create(&g_PthreadId[1], NULL, pthread_health_monitor, (void *)&iPthredArg);

	getc(stdin);
	while(1);
	
    iRet = init_Toupcam();
    if(ERROR_FAILED == iRet)
    {
        goto exit1_;
    }
    printf("init Toupcam callback.\n");
    
    /* 安装一个Toupcam摄像头设备 */
    iRet = SetupToupcam(g_pstTouPcam);
    if(ERROR_FAILED == iRet)
    {
        goto exit1_;
    }


exit1_:
    Destory_Toupcam();
exit0_:
    Destory_sock();

    return 0;
}
