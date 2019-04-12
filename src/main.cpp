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

extern "C"
{
    #include "../include/codeimg.h"
    #include "../include/rtp.h"
    #include "../include/toupcam.h"
    #include "../include/common_toupcam.h"
    #include "../include/socket.h"
}


struct sockets *sock; /* udp potr:5004 */
struct sockets *sock1; /* tcp potr:5005 */

X264Encoder x264Encoder;

//rtp 包结构
struct rtp_pack *rtp;
//rtp数据结构
//struct rtp_data *pdata;

//创建rtp头
struct rtp_pack_head head;

int sendnum = 0;

#define VENC_FPS 30

//using namespace cv;
using namespace std;

HToupCam g_hcam = NULL;
void* g_pImageData = NULL;
void* g_pStaticImageData = NULL;
int g_pStaticImageDataFlag = 0; /* 检测静态图片是否捕获完成 */

unsigned g_total = 0;

int frame_num = 0;
int nWidth = 0, nHeight = 0;
int WIDTH = 0, HEIGHT = 0;

/* support for jpeg transmit */
unsigned char *gpucJpgDest = NULL;//[1024*1022];
int giJpgSize=1024*1022;
#define RGB24_DEPTH  (3)

/* 线程 */
#define MaxThreadNum      (4)
pthread_t gPthreadId[MaxThreadNum] = {0};
unsigned int gPthreadMaxNum = ARRAY_SIZE(gPthreadId);
pthread_mutex_t gPthreadMutex_h264;

extern void Destory_sock(void);

void __stdcall EventCallback(unsigned nEvent, void* pCallbackCtx)
{
    if (TOUPCAM_EVENT_IMAGE == nEvent)
    {
        ToupcamFrameInfoV2 info = { 0 };
        memset(g_pImageData, 0, sizeof(g_pstTouPcam->m_header.biSizeImage));
        HRESULT hr = Toupcam_PullImageV2(g_hcam, g_pImageData, 24, &info);
#ifdef H264
        if (FAILED(hr))
            printf("failed to pull image, hr = %08x\n", hr);
        else
        {
            /* After we get the image data, we can do anything for the data we want to do */
            printf("pull image ok, total = %u, resolution = %u x %u\n", ++g_total, info.width, info.height);
            if(frame_num == 0)
            {
                encode_yuv((unsigned char *)g_pImageData);
                frame_num = 0;
            }else
            {
                frame_num++;
            }
        }
#else
        encode_jpeg((unsigned char *)g_pImageData);
#endif        
    }
    else if(TOUPCAM_EVENT_STILLIMAGE == nEvent)
    {
        ToupcamFrameInfoV2 info = { 0 };
        memset(g_pStaticImageData, 0, sizeof(g_pstTouPcam->m_header.biSizeImage));
        HRESULT hr = Toupcam_PullImageV2(g_hcam, g_pStaticImageData, 24, &info);
        if (FAILED(hr))
            printf("failed to pull image, hr = %08x\n", hr);
        else
        {
            /* After we get the image data, we can do anything for the data we want to do */
            printf("pull image ok, total = %u, resolution = %u x %u\n", ++g_total, info.width, info.height);
            encode_jpeg((unsigned char *)g_pStaticImageData);
        }
    }
    else
    {
        printf("event callback: %d\n", nEvent);
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
			printf("select time out.\n");
			continue;
		}
		else if(iRet < 0)
		{
			printf("select fail.\n");
			continue;
		}
		for(i = 0; i < iServerFd+1; i++)
		{
			if(FD_ISSET(i, &tmprdfs))
			{
				if(iServerFd == i)
				{
					iClientFd = accept(iServerFd, &stClientaddr, &addrlen);
					if(iClientFd < 0)
					{
						perror("stream accept: ");
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
						usSize = *(pcBuffData+5) | *(pcBuffData+6);
						g_ReqResFlag = *(pcBuffData+7);
						common_hander(i, pcBuffData+8, usSize-1);
					}
					/* close */
				}
				FD_CLR(i, &tmprdfs);
			}
		}
	}

	return NULL;
}

void *pthread_health_monitor(void *pdata)
{
	
	return NULL;
}


int init_sock()
{
    int iRet = ERROR_SUCCESS;
    
    /* step 1 init dgram */
    (void)socket_dgram_init();
    
    /* step 2 init stream */
    (void)socket_stream_init();

	return iRet;
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

    /* tcp server thread */
    iRet = pthread_create(&gPthreadId[0], NULL, pthread_server, (void *)&iPthredArg);

	/* toupcam health's monitor thread */
    iRet = pthread_create(&gPthreadId[1], NULL, pthread_health_monitor, (void *)&iPthredArg);

    iRet = init_Toupcam();
    if(ERROR_FAILED == iRet)
    {
        goto exit1_;
    }
    
    /* 安装一个Toupcam摄像头设备 */
    iRet = SetupToupcam(g_pstTouPcam);
    if(ERROR_FAILED == iRet)
    {
        goto exit1_;
    }

    /* cleanup */
    Toupcam_Close(g_hcam);
    if(g_pImageData)
        free(g_pImageData);
        g_pImageData = NULL;
    if(g_pStaticImageData)
        free(g_pStaticImageData);
        g_pStaticImageData = NULL;

exit1_:
    //Destory_sock();
    //Destory_Toupcam();
exit0_:
    //Destory_sock();

    return 0;
}
