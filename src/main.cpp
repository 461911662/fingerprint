//#include "opencv2/core.hpp"
//#include "opencv2/imgproc.hpp"
//#include "opencv2/highgui.hpp"
//#include "opencv2/videoio.hpp"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include "../include/socket.h"
#include "../include/x264.h"
#include "../include/codeimg.h"
#include "../include/rtp.h"
#include "../include/toupcam.h"
#include "../include/common_toupcam.h"
#include "../include/mpp_encode_data.h"

//using namespace cv;
using namespace std;


struct sockets *sock; /* udp potr:5004 */
struct sockets *sock1; /* tcp potr:5005 */
fd_set rdfs;    /* select 监听字符集 */
int iMaxfd;     /* select 所监听的最大描述符          */

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
MPP_ENC_DATA_S *g_pstmpp_enc_data = NULL;
int g_pStaticImageDataFlag = 0; /* 检测静态图片是否捕获完成 */

unsigned g_total = 0;

int frame_num = 0;
int nWidth = 0, nHeight = 0;
int WIDTH = 0, HEIGHT = 0;

/* support for jpeg transmit */
unsigned char *g_pucJpgDest = NULL;//[1024*1022];
unsigned int giJpgSize=1024*1022;

/* 大小端标志 */
unsigned int iEndianness = 0;

/* 线程 */
#define MaxThreadNum      (4)
pthread_t g_PthreadId[MaxThreadNum] = {0};
unsigned int g_PthreadMaxNum = ARRAY_SIZE(g_PthreadId);
pthread_mutex_t g_PthreadMutex_h264;
pthread_mutex_t g_PthreadMutexJpgDest;
pthread_mutex_t g_PthreadMutexMonitor;
sem_t g_SemaphoreHistoram;

extern void Destory_sock(void);
extern MPP_RET mpp_ctx_deinit(MPP_ENC_DATA_S **data);

union { 
    char c[4]; 
    unsigned long l; 
} endian_test = { 'l', '?', '?', 'b' };

#define ENDIANNESS ((char)endian_test.l)


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
 #ifdef SOFT_ENCODE_H264 
                    encode_yuv((unsigned char *)g_pImageData);
#else
                    encode2hardware((unsigned char *)g_pImageData);
#endif

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
            hr = Toupcam_PullStillImageV2(g_hcam, NULL, 24, &info);
            if (FAILED(hr))
                printf("failed to pull image, hr = %08x\n", hr);
            else
            {
                /* After we get the image data, we can do anything for the data we want to do */
                /* printf("pull static image ok, total = %u, resolution = %u x %u\n", ++g_total, info.width, info.height); */
                
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
            }
            break;
        case TOUPCAM_EVENT_TEMPTINT:     /* white balance changed, Temp/Tint mode */
            printf("toupcam event TOUPCAM_EVENT_TEMPTINT(%d).\n", nEvent);
            pthread_mutex_trylock(&g_pstTouPcam->stWhiteBlc.mutex);
            if(g_pstTouPcam->stWhiteBlc.iauto)
            {
                hr = Toupcam_get_TempTint(g_pstTouPcam->m_hcam, &g_pstTouPcam->stWhiteBlc.Temp, &g_pstTouPcam->stWhiteBlc.Tint);
                if(FAILED(hr))
                {
                    printf("init Temp,Tint failed!\n");
                }
                printf("White balance Temp:%d, Tint:%d\n", g_pstTouPcam->stWhiteBlc.Temp, g_pstTouPcam->stWhiteBlc.Tint);
            }
            pthread_mutex_unlock(&g_pstTouPcam->stWhiteBlc.mutex);
            break;
        case TOUPCAM_EVENT_WBGAIN:       /* white balance changed, RGB Gain mode */
            /* not support it yet */
            break;
        case TOUPCAM_EVENT_EXPOSURE:     /* exposure time changed */
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

static void pHistramCallback(const float aHistY[256], const float aHistR[256], const float aHistG[256], const float aHistB[256], void* pCtx)
{
    if(!g_pstTouPcam->m_hcam)
    {
        return;
    }
    sem_wait(&g_SemaphoreHistoram);
    memcpy(g_pstTouPcam->stHistoram.aHistY, aHistY, sizeof(g_pstTouPcam->stHistoram.aHistY)); 
    memcpy(g_pstTouPcam->stHistoram.aHistY, aHistR, sizeof(g_pstTouPcam->stHistoram.aHistR)); 
    memcpy(g_pstTouPcam->stHistoram.aHistY, aHistG, sizeof(g_pstTouPcam->stHistoram.aHistG)); 
    memcpy(g_pstTouPcam->stHistoram.aHistY, aHistB, sizeof(g_pstTouPcam->stHistoram.aHistB));
    sem_post(&g_SemaphoreHistoram);
    return;
}



void *pthread_link_task1(void *argv)
{
    if(NULL == argv)
    {
        return NULL;
    }
    
    int fd = *(int *)argv;
    int pCtx = 0;
    HRESULT hr;
    int pHistoramCtx = 0;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;
    stToupcamRespon.com.cmd = COMCMD_TOUPCAMCFG;
    sprintf(stToupcamRespon.com.proto, "%s", "proto");
    stToupcamRespon.com.proto[4] = 'o';
    stToupcamRespon.com.type = TCP_RESPONSE;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;
    stToupcamRespon.com.size[1] = 0;
    stToupcamRespon.cc = ERROR_SUCCESS;
    memset(stToupcamRespon.data.reserve, 0, ARRAY_SIZE(stToupcamRespon.data.reserve));

    stToupcamRespon.com.type = TCP_DIRECTTSM;
    stToupcamRespon.com.cmd = CMD_DATATSM;

    char *pcdes = (char *)malloc(sizeof(g_pstTouPcam->stHistoram.aHistY)*4+TOUPCAM_COMMON_RESPON_HEADER_SIZE);
    char *pcfinger = NULL;
    if(NULL == pcdes)
    {
        perror("pthread_link_task1");
        return NULL;
    }

    while(1)
    {
        if(!g_pstTouPcam->m_hcam)
        {
            continue;
        }
        pcfinger = pcdes;
        void *pstr = &pCtx;
        memset(pcfinger, 0, sizeof(g_pstTouPcam->stHistoram.aHistY)*4);
        hr = Toupcam_GetHistogram(g_pstTouPcam->m_hcam, pHistramCallback, (void*)&pHistoramCtx);
        if(FAILED(hr))
        {
            //printf("get Historam data failed(%lld)\n", hr);
            pthread_mutex_unlock(&g_PthreadMutexMonitor);
            continue;
        }
        

        stToupcamRespon.com.size[0] = ARRAY_SIZE(g_pstTouPcam->stHistoram.aHistY)*4;
        stToupcamRespon.cc = ERROR_SUCCESS;

        sem_wait(&g_SemaphoreHistoram);
        memcpy(pcfinger, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE);
        pcfinger+=TOUPCAM_COMMON_RESPON_HEADER_SIZE;
        memcpy(pcfinger, g_pstTouPcam->stHistoram.aHistY, sizeof(g_pstTouPcam->stHistoram.aHistY)); 
        pcfinger+=sizeof(g_pstTouPcam->stHistoram.aHistY);
        memcpy(pcfinger, g_pstTouPcam->stHistoram.aHistR, sizeof(g_pstTouPcam->stHistoram.aHistR)); 
        pcfinger+=sizeof(g_pstTouPcam->stHistoram.aHistR);
        memcpy(pcfinger, g_pstTouPcam->stHistoram.aHistG, sizeof(g_pstTouPcam->stHistoram.aHistG)); 
        pcfinger+=sizeof(g_pstTouPcam->stHistoram.aHistG);
        memcpy(pcfinger, g_pstTouPcam->stHistoram.aHistB, sizeof(g_pstTouPcam->stHistoram.aHistB)); 
        sem_post(&g_SemaphoreHistoram);

        usleep(5000);
        if(fd < 0)
        {
            break;
        }
        
        send(fd, pcdes, sizeof(g_pstTouPcam->stHistoram.aHistY)*4+TOUPCAM_COMMON_RESPON_HEADER_SIZE, 0);

    }

}

void link_task(int fd)
{
    if(fd < 0)
    {
        return;
    }
    int ifd = fd;

    pthread_t  pt;

    /* create link task */
    pthread_create(&pt, NULL, pthread_link_task1, (void *)&ifd);

    pthread_join(pt, NULL);

    return ;
}

void *pthread_server(void *pdata)
{
    int iClientFd, iServerFd, iRet, i;
    unsigned int uiSize = 0;
    TOUPCAM_COMMON_REQUES_S stToupcam_common_req;
    struct sockaddr stClientaddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if(sock1->local < 0)
    {
        printf("socket fd(%d).\n", sock1->local);
        return NULL;
    }
    iServerFd = sock1->local;

	struct timeval tv = {5, 0};
	FD_ZERO(&rdfs);
	FD_SET(iServerFd, &rdfs);
    fd_set tmprdfs = rdfs;
	iMaxfd = iServerFd;
	while(1)
	{
	    sleep(1);
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
                    //link_task(iClientFd);
				}
				else
				{
					memset(g_cBuffData, 0, ARRAY_SIZE(g_cBuffData));
					iRet = read(i, g_cBuffData, sizeof(TOUPCAM_COMMON_REQUES_S));
					if(iRet > 0)
					{
						char *pcBuffData = strstr(g_cBuffData, "proto");
                        /*
                        char *pcBuff = pcBuffData;
                        
                        while(1)
                        {
                            pcBuff = strstr(pcBuff, "proto");
                            if(NULL == pcBuff)
                            {
                                break;
                            }
                            pcBuffData = pcBuff;
                            if(pcBuff >= g_cBuffData+ARRAY_SIZE(g_cBuffData)-5)
                            {
                                break;
                            }
                            pcBuff += 5;
                        }
                        */
                        
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
						memcpy(&stToupcam_common_req, pcBuffData, sizeof(TOUPCAM_COMMON_REQUES_S));
                        if(!iEndianness)
                        {
                           uiSize = BIGLITTLESWAP32(stToupcam_common_req.com.size[0]);
                        }
                        else
                        {
                            uiSize = stToupcam_common_req.com.size[0];
                        }
						g_ReqResFlag = stToupcam_common_req.com.type;
                        common_hander(i, (void *)&stToupcam_common_req, uiSize);
					}
                    else
                    {
                        printf("sock unconnect...\n");
                        FD_CLR(i, &rdfs);
                        close(i);
                    }
				}
			}
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
    /* 异常关闭所监听的描述符 */
    int i = 0;
    for(i=3; i<iMaxfd+1; i++)
    {
        close(i);
    }
    return;
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

void semaphore_inits(void)
{
    sem_init(&g_SemaphoreHistoram, 0, 1);
    return;
}

void pthread_mutex_inits(void)
{
    /* 初始化jpeg线程数据保护锁 */
    pthread_mutex_init(&g_PthreadMutexJpgDest, NULL);
    pthread_mutex_init(&g_PthreadMutexMonitor, NULL);
    return;
}

void pthread_mutex_destroys()
{
    /* 销毁线程锁 */
    pthread_mutex_destroy(&g_PthreadMutexJpgDest);
    pthread_mutex_destroy(&g_PthreadMutexMonitor);
    return;
}

static int SetupToupcam(void)
{
    unsigned int iRet = ERROR_SUCCESS;
    TOUPCAM_S *pstTouPcam = NULL;
    iRet = init_Toupcam((void *)g_pstTouPcam);
    iRet += g_pstTouPcam->OpenDevice();
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }

    iRet = g_pstTouPcam->PreInitialDevice((void *)g_pstTouPcam);
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }

    iRet = g_pstTouPcam->ConfigDevice((void *)g_pstTouPcam);
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }

    iRet = g_pstTouPcam->StartDevice((void *)g_pstTouPcam);
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }    

    g_pstTouPcam->CloseDevice((void *)g_pstTouPcam);

    return ERROR_SUCCESS;
}

/*
* 主函数main
*/
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

    /* 大小端检测 */
    if(ENDIANNESS == 'l')
    {
        iEndianness = 1;
    }
    else
    {
        iEndianness = 0;
    }

    /* 初始化线程锁 */
    pthread_mutex_inits();
    semaphore_inits();
    
    /* tcp server thread */
    iRet = pthread_create(&g_PthreadId[0], NULL, pthread_server, (void *)&iPthredArg);

	/* toupcam health's monitor thread */
    iRet += pthread_create(&g_PthreadId[1], NULL, pthread_health_monitor, (void *)&iPthredArg);
    if(ERROR_FAILED == iRet)
    {
        goto exit0_;
    }

    /* 启动摄像头 */
    iRet = SetupToupcam();
    if(ERROR_FAILED == iRet)
    {
        goto exit1_;
    }

#if 0    
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
#endif

    pthread_mutex_destroys();

#ifndef SOFT_ENCODE_H264
    mpp_ctx_deinit(&g_pstmpp_enc_data);
#endif

exit1_:
    Destory_Toupcam();
exit0_:
    Destory_sock();

    return 0;
}
