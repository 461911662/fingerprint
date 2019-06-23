//#include "opencv2/core.hpp"
//#include "opencv2/imgproc.hpp"
//#include "opencv2/highgui.hpp"
//#include "opencv2/videoio.hpp"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include "../include/socket.h"
#include "../include/x264.h"
#include "../include/codeimg.h"
#include "../include/rtp.h"
#include "../include/toupcam.h"
#include "../include/toupcam_log.h"
#include "../include/common_toupcam.h"
#include "../include/mpp_encode_data.h"
#include "../include/toupcam_data.h"
#include "../include/toupcam_parse.h"
#include "../include/fixframerate.h"

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
HashTable *g_pstToupcamHashTable = NULL;

unsigned g_total = 0;

int frame_num = 0;
int frame_size = 0;
int nWidth = 0, nHeight = 0;
int WIDTH = 0, HEIGHT = 0;

/* support for jpeg transmit */
unsigned char *g_pucJpgDest = NULL;//[1024*1022];
unsigned int giJpgSize=1024*1022;

/* 控制帧率 */
FixFrameRate *pstFixFrameRate = NULL;

/* 大小端标志 */
unsigned int iEndianness = 0;

/* 线程 */
#define MaxThreadNum      (4)
pthread_t g_PthreadId[MaxThreadNum] = {0};
unsigned int g_PthreadMaxNum = ARRAY_SIZE(g_PthreadId);
pthread_mutex_t g_PthreadMutex_h264;
pthread_mutex_t g_PthreadMutexJpgDest;
pthread_mutex_t g_PthreadMutexMonitor;
pthread_mutex_t g_PthreadMutexUDP;
sem_t g_SemaphoreHistoram;

unsigned int *g_puiProcess_task = NULL;
unsigned int g_uiProcess_num = 0;

extern void Destory_sock(void);
extern MPP_RET mpp_ctx_deinit(MPP_ENC_DATA_S **data);

union { 
    char c[4]; 
    unsigned long l; 
} endian_test = { 'l', '?', '?', 'b' };

#define ENDIANNESS ((char)endian_test.l)

void signal_brokenpipe(int event) { 
}

pthread_t *distribute_thread()
{
    pthread_t * pid = NULL;
    for(int i = 0; i < MaxThreadNum; i++)
    {
        if(g_PthreadId[i]>0)
        {
            continue;
        }
        pid = &g_PthreadId[i];
        break;
    }
    if(NULL == pid)
    {
        toupcam_log_f(LOG_ERROR, "unable to distribute thread.");
    }
    return pid;
}

int distribute_process()
{
    int i;
    for (i = 0; i < g_uiProcess_num; i++)
    {
        if(g_puiProcess_task[i] > 0)
        {
            continue;
        }
        g_puiProcess_task[i] = 1;
        break;
    }

    if(i == g_uiProcess_num)
    {
        toupcam_log_f(LOG_WARNNING, "unable to distribute process, because of Max process:%d", i);
        i = -1;
    }
    
    return i;
}

void signal_interrupt(int event)
{
    if(SIGINT == event)
    {
        sleep(5);
        toupcam_log(LOG_INFO, "main exit, os reboot in the future.");
        sleep(10);
        system("/sbin/reboot");
        //exit(0);
    }
}

void timer_handler(int signo)
{
    static char c = '\\';
    static unsigned char ucbreak = 0;
    static unsigned char ucframenum = 0;
	double dSize = 0;
	char cmb = 'k';
    if('\\' == c)
    {
        c = '|';
    }
    else if('/' == c)
    {
        c = '\\';
    }
    else if('|' == c)
    {
        c = '/';
    }

    ucbreak++;
    ucframenum += frame_num;
    if(60 == ucbreak)
    {
        if(0 == ucframenum)
        {
            raise(SIGINT);
        }
        ucframenum = 0;
        ucbreak = 0;
    }
    
   	if(frame_size > 0)
    {
        pthread_mutex_trylock(&g_PthreadMutexUDP);
        dSize = (double)frame_size;
		cmb = frame_size > 1024*1024*1024?'G':
			  frame_size >      1024*1024?'M':
			  frame_size >           1024?'K':'B';

        dSize = frame_size > 1024*1024*1024?frame_size/(1024*1024*1024):
			    frame_size >      1024*1024?frame_size/(1024*1024):
			    frame_size >           1024?frame_size/(1024):frame_size;
        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\033[40;32mfps:   size:        \033[0m\n");
        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\033[40;32mfps:%02d size:%03.2lf%c%c\033[0m\n", frame_num, dSize, cmb, c);
        pthread_mutex_unlock(&g_PthreadMutexUDP);
    }
    frame_num = 0;
    return ;
}

int init_sigaction_timer(void)
{
    int iRet = ERROR_SUCCESS;

    /* 初始化型号处理函数 */
    struct sigaction stNewAction;
    stNewAction.sa_handler = timer_handler;
    stNewAction.sa_flags = 0;
    sigemptyset(&stNewAction.sa_mask);
    iRet = sigaction(SIGALRM, &stNewAction, NULL);
    if(ERROR_SUCCESS != iRet)
    {
        toupcam_log_f(LOG_ERROR, "%s", strerror(errno));
        iRet = ERROR_FAILED;
        goto exit0_;
    }

    /* 初始化定时器 */
    struct itimerval stValue;
    stValue.it_value.tv_sec = 5;
    stValue.it_value.tv_usec = 0;
    struct itimerval stNextValue;
    stNextValue.it_value.tv_sec = 1;
    stNextValue.it_value.tv_usec = 0;    
    stValue.it_interval = stNextValue.it_value;
    iRet = setitimer(ITIMER_REAL, &stValue, NULL);
    if(ERROR_SUCCESS != iRet)
    {
        toupcam_log_f(LOG_ERROR, "%s", strerror(errno));
        iRet = ERROR_FAILED;
        goto exit0_;
    }    

exit0_:

    return iRet;
}

int init_signals(void)
{
    int iRet = ERROR_SUCCESS;

    if(SIG_ERR == signal(SIGPIPE, signal_brokenpipe))
    {
        toupcam_log_f(LOG_ERROR, "%s", strerror(errno));
        iRet = ERROR_FAILED;
        goto exit0_;
    }

    if(SIG_ERR == signal(SIGINT, signal_interrupt))
    {
        toupcam_log_f(LOG_ERROR, "%s", strerror(errno));
        iRet = ERROR_FAILED;
        goto exit0_;
    }

    iRet = init_sigaction_timer();
    if(ERROR_SUCCESS != iRet)
    {
        iRet = ERROR_FAILED;
        goto exit0_;
    }

exit0_:

    return iRet;       
}


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
            hr = Toupcam_PullImageV2(g_hcam, g_pImageData, BIT_DEPTH, &info);

            /* 算法处理 */
#ifdef PICTURE_ARITHMETIC
            hr += image_arithmetic_handle_rgb((unsigned char *)g_pImageData,  g_pstTouPcam->inHeight, g_pstTouPcam->inWidth, 24);
#endif
            if (FAILED(hr))
                toupcam_log_f(LOG_INFO, "failed to pull image, hr = %08x\n", hr);
            else
            {                
                /* After we get the image data, we can do anything for the data we want to do */
                /* toupcam_log_f(LOG_INFO, "pull image ok, total = %u, resolution = %u x %u\n", ++g_total, info.width, info.height); */
                /*
                FILE *fp = fopen("mRGB.dat", "ab");
                if(NULL == fp)
                {
                    toupcam_log_f(LOG_INFO, "mGRB.dat:%s", strerror(errno));
                }
                else
                {
                    fwrite(g_pImageData, g_pstTouPcam->inHeight*g_pstTouPcam->inWidth*3, 1, fp);
                    fclose(fp);
                }
                */
                pstFixFrameRate->PushQueue((char *)g_pImageData);
            }
            break;
        case TOUPCAM_EVENT_STILLIMAGE:
            toupcam_log_f(LOG_INFO, "toupcam event TOUPCAM_EVENT_STILLIMAGE(%d).\n", nEvent);
            memset(g_pStaticImageData, 0, sizeof(g_pstTouPcam->m_header.biSizeImage));
            hr = Toupcam_PullStillImageV2(g_hcam, NULL, 24, &info);
            if (FAILED(hr))
                toupcam_log_f(LOG_INFO, "failed to pull image, hr = %08x\n", hr);
            else
            {
                /* After we get the image data, we can do anything for the data we want to do */
                /* toupcam_log_f(LOG_INFO, "pull static image ok, total = %u, resolution = %u x %u\n", ++g_total, info.width, info.height); */
                
                hr = Toupcam_PullStillImage(g_hcam, g_pStaticImageData, 24, NULL, NULL);
                if (SUCCEEDED(hr))
                {
                    toupcam_log_f(LOG_INFO, "encode.\n");
                    encode_jpeg((unsigned char *)g_pStaticImageData);
                }
                else
                {
                    toupcam_log_f(LOG_INFO, "miss static image, total = %u, resolution = %u x %u\n", ++g_total, info.width, info.height);
                }
            }
            break;
        case TOUPCAM_EVENT_TEMPTINT:     /* white balance changed, Temp/Tint mode */
            toupcam_log_f(LOG_INFO, "toupcam event TOUPCAM_EVENT_TEMPTINT(%d).\n", nEvent);
            pthread_mutex_trylock(&g_pstTouPcam->stWhiteBlc.mutex);
            if(g_pstTouPcam->stWhiteBlc.iauto)
            {
                hr = Toupcam_get_TempTint(g_pstTouPcam->m_hcam, &g_pstTouPcam->stWhiteBlc.Temp, &g_pstTouPcam->stWhiteBlc.Tint);
                if(FAILED(hr))
                {
                    toupcam_log_f(LOG_INFO, "init Temp,Tint failed!\n");
                }
                toupcam_log_f(LOG_INFO, "White balance Temp:%d, Tint:%d\n", g_pstTouPcam->stWhiteBlc.Temp, g_pstTouPcam->stWhiteBlc.Tint);
            }
            pthread_mutex_unlock(&g_pstTouPcam->stWhiteBlc.mutex);
            break;
        case TOUPCAM_EVENT_WBGAIN:       /* white balance changed, RGB Gain mode */
            /* not support it yet */
            break;
        case TOUPCAM_EVENT_EXPOSURE:     /* exposure time changed */
            /* toupcam_log_f(LOG_INFO, "toupcam event TOUPCAM_EVENT_EXPOSURE(%d).\n", nEvent); */
            if(g_pstTouPcam->OnEventExpo)
            {
                g_pstTouPcam->OnEventExpo();
            }
            else
            {
                toupcam_log_f(LOG_INFO, "TOUPCAM_EVENT_EXPOSURE: not register it!!!\n");
            }
            break;
        case TOUPCAM_EVENT_TRIGGERFAIL:  /* trigger failed */
        case TOUPCAM_EVENT_BLACK:        /* black balance changed */
        case TOUPCAM_EVENT_FFC:          /* flat field correction status changed */
        case TOUPCAM_EVENT_DFC:          /* dark field correction status changed */
            //toupcam_log_f(LOG_INFO, "toupcam event: %d\n", nEvent);
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
            toupcam_log_f(LOG_INFO, "unknown event: %d\n", nEvent);
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
    memcpy(g_pstTouPcam->stHistoram.aHistR, aHistR, sizeof(g_pstTouPcam->stHistoram.aHistR)); 
    memcpy(g_pstTouPcam->stHistoram.aHistG, aHistG, sizeof(g_pstTouPcam->stHistoram.aHistG)); 
    memcpy(g_pstTouPcam->stHistoram.aHistB, aHistB, sizeof(g_pstTouPcam->stHistoram.aHistB));
    sem_post(&g_SemaphoreHistoram);
    return;
}



void *pthread_link_task1(void *argv)
{
    if(NULL == argv)
    {
        toupcam_log_f(LOG_ERROR, "input error");
        *(int *)argv = -1;
        pthread_exit(argv);
    }
    int iRet;
    int iNum = 0;
    int iLen = 0;
    int fd = *(int *)argv;
    int pCtx = 0;
    char *pcdes = NULL;
    char *pcfinger = NULL;
    HRESULT hr;
    int pHistoramCtx = 0;
    TOUPCAM_COMMON_RESPON_S stToupcamRespon;

    char cname[16] = {0};
    cpu_set_t mask;

    int processid = distribute_process();
    if(-1 != processid)
    {
        CPU_ZERO(&mask);
        CPU_SET(processid, &mask);
        if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
        {
            toupcam_log_f(LOG_WARNNING, "%s", strerror(errno));
        }
        toupcam_log_f(LOG_INFO, "thread id(%d) use process(%d)", (int)pthread_self(), processid);
    }
    
    if(-1 != processid)
    {
        sprintf(cname, "link_task1_p%d", processid);
    }
    else
    {
        sprintf(cname, "link_task1_p__");
    }
    
    iRet = prctl(PR_SET_NAME, cname);
    if(0 != iRet)
    {
        toupcam_log_f(LOG_WARNNING, "%s", strerror(errno));
        goto exit0;
    }

    stToupcamRespon.com.cmd = COMCMD_TOUPCAMCFG;
    sprintf(stToupcamRespon.com.proto, "%s", "proto");
    stToupcamRespon.com.proto[4] = 'o';
    stToupcamRespon.com.type = TCP_RESPONSE;
    stToupcamRespon.com.size[0] = INVAILD_BUFF_SIZE;
    stToupcamRespon.com.size[1] = 0;
    stToupcamRespon.cc = ERROR_SUCCESS;
    memset(stToupcamRespon.data.reserve, 0, ARRAY_SIZE(stToupcamRespon.data.reserve));

    stToupcamRespon.com.type = UDP_DATATSM;
    stToupcamRespon.com.cmd = COMCMD_TOUPCAMCFG;
    stToupcamRespon.com.subcmd = CMD_DATATSM;

    pcdes = (char *)malloc(sizeof(g_pstTouPcam->stHistoram.aHistY)*4+TOUPCAM_COMMON_RESPON_HEADER_SIZE);
    if(NULL == pcdes)
    {
        toupcam_log_f(LOG_ERROR, "%s.", strerror(errno));
        goto exit0;
    }

    while(1)
    {
        if(!g_pstTouPcam->m_hcam)
        {
            continue;
        }

        /* 更新自动直方图 */
        pthread_mutex_trylock(&g_PthreadMutexMonitor);
        if(g_pstTouPcam->stHistoram.bAutoHis)
        {
            hr = Toupcam_LevelRangeAuto(g_pstTouPcam->m_hcam);
            if(FAILED(hr))
            {
                /* goto _exit0; */
            }
            hr = Toupcam_get_LevelRange(g_pstTouPcam->m_hcam, g_pstTouPcam->stHistoram.aLow, g_pstTouPcam->stHistoram.aHigh);
            if(FAILED(hr))
            {
                /* goto _exit0; */
            }
        }
        pthread_mutex_unlock(&g_PthreadMutexMonitor);
    
        pcfinger = pcdes;
        void *pstr = &pCtx;
        memset(pcfinger, 0, sizeof(g_pstTouPcam->stHistoram.aHistY)*4);
        hr = Toupcam_GetHistogram(g_pstTouPcam->m_hcam, pHistramCallback, (void*)&pHistoramCtx);
        if(FAILED(hr))
        {
            toupcam_dbg_f(LOG_DEBUG, "get Historam data failed(%lld)\n", hr);
            continue;
        }

        stToupcamRespon.com.size[0] = ARRAY_SIZE(g_pstTouPcam->stHistoram.aHistY)*4;
        stToupcamRespon.cc = ERROR_SUCCESS;

        sem_wait(&g_SemaphoreHistoram);
        memcpy(pcfinger, &stToupcamRespon, TOUPCAM_COMMON_RESPON_HEADER_SIZE);
        pcfinger+=TOUPCAM_COMMON_RESPON_HEADER_SIZE;
        memcpy(pcfinger, g_pstTouPcam->stHistoram.aHistY, sizeof(g_pstTouPcam->stHistoram.aHistY)); 
#ifdef HISTOGRAM_RGB
        pcfinger+=sizeof(g_pstTouPcam->stHistoram.aHistY);
        memcpy(pcfinger, g_pstTouPcam->stHistoram.aHistR, sizeof(g_pstTouPcam->stHistoram.aHistR)); 
        pcfinger+=sizeof(g_pstTouPcam->stHistoram.aHistR);
        memcpy(pcfinger, g_pstTouPcam->stHistoram.aHistG, sizeof(g_pstTouPcam->stHistoram.aHistG)); 
        pcfinger+=sizeof(g_pstTouPcam->stHistoram.aHistG);
        memcpy(pcfinger, g_pstTouPcam->stHistoram.aHistB, sizeof(g_pstTouPcam->stHistoram.aHistB)); 
#endif
        sem_post(&g_SemaphoreHistoram);
        sleep(1);

        /* iLen = sizeof(g_pstTouPcam->stHistoram.aHistY)*4+TOUPCAM_COMMON_RESPON_HEADER_SIZE; */
        iLen = sizeof(g_pstTouPcam->stHistoram.aHistY)+TOUPCAM_COMMON_RESPON_HEADER_SIZE;

        /* pthread_mutex_lock(&g_PthreadMutexUDP); */
        if((iNum=sendto(sock->local, pcdes, iLen, 0, (struct sockaddr *)sock->cliaddr[1], sizeof(struct sockaddr_in)))==-1)
        {
            fail("socket: %s\n", strerror(errno));
        }
        /* pthread_mutex_unlock(&g_PthreadMutexUDP); */

        toupcam_dbg_f(LOG_DEBUG, "send: sock->local:%d iLen:%d, ip:%s.\n", sock->local, iLen, inet_ntoa(sock->cliaddr[1]->sin_addr));

    }

    
    free(pcdes);
    pcdes = NULL;

exit0:
    g_puiProcess_task[processid] = 0;
    *(int *)argv = -1;
    pthread_exit(argv);
    return NULL;

}

void link_task()
{
    int ifd = 0;

    pthread_t  *pid = NULL;
    pthread_attr_t attr;

    int s = pthread_attr_init(&attr);
    if (0 != s)
    {
        toupcam_log_f(LOG_INFO, "pthread_attr_init(%d)", s);
    }
    s = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (0 != s)
    {
        toupcam_log_f(LOG_INFO, "pthread_attr_setdetachstate(%d)", s);
    }

    /* create link task */
    pid = distribute_thread();
    if(NULL == pid)
    {
        return;
    }
    pthread_create(pid, &attr, pthread_link_task1, (void *)&ifd);

    sleep(2);
    if(-1 == ifd)
    {
        *pid = 0;
        toupcam_log_f(LOG_ERROR, "abort pthread_link_task1 exit");
    }
    //pthread_join(pt, NULL);

    return ;
}

void *pthread_server(void *pdata)
{
    int iClientFd, iServerFd, iRet, i;
    unsigned int uiSize = 0;
    TOUPCAM_COMMON_REQUES_S stToupcam_common_req;
    struct sockaddr stClientaddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    cpu_set_t mask;
    int id;
    pthread_t pid;
    char cname[16] = {0};

    int processid = distribute_process();
    if(-1 != processid)
    {
        CPU_ZERO(&mask);
        CPU_SET(processid, &mask);
        if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
        {
            toupcam_log_f(LOG_WARNNING, "%s", strerror(errno));
        }
        toupcam_log_f(LOG_INFO, "thread id(%d) use process(%d)", (int)pthread_self(), processid);
    }
    
    if(sock1->local < 0)
    {
        toupcam_log_f(LOG_INFO, "socket fd(%d).\n", sock1->local);
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
			//toupcam_log_f(LOG_INFO, "select time out.\n");
			continue;
		}
		else if(iRet < 0)
		{
			//toupcam_log_f(LOG_INFO, "select fail.\n");
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
						toupcam_log_f(LOG_ERROR, "%s.\n", strerror(errno));
						continue;
					}

				    if(1 == g_pstTouPcam->iconnect)
                    {
                        toupcam_log_f(LOG_WARNNING, "tcp connection has been establishe...\n", i, iClientFd);
                        close(iClientFd);
                        continue;
                    }

					if(iClientFd > iMaxfd)
					{
						iMaxfd = iClientFd;
					}
					FD_SET(iClientFd, &rdfs);
					toupcam_log_f(LOG_INFO, "tcp(%d) listen(%d)...\n", i, iClientFd);
                    link_task();
                    g_pstTouPcam->iconnect = 1;
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
							toupcam_log_f(LOG_INFO, "no proto.\n");
							FD_CLR(i, &tmprdfs);
							continue;
						}

                        if(iRet != sizeof(TOUPCAM_COMMON_REQUES_S))
                        {
                            toupcam_log_f(LOG_INFO, "length too shorter(%d)%d.\n", iRet, sizeof(TOUPCAM_COMMON_REQUES_S));
                            FD_CLR(i, &tmprdfs);
                            continue;
                        }

						/*
						*debug
						int k;
						for(k=0; k<11; k++)
						{
							toupcam_log_f(LOG_INFO, "%2x ", g_cBuffData[k]);
						}
						toupcam_log_f(LOG_INFO, "\n");
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
                        memset(cname, 0, 128);
                        for (id = 0; id < MaxThreadNum; ++id)
                        {
                            pid = g_PthreadId[id];
                            if(pid)
                            {
                                pthread_getname_np(pid, cname, 16);
                                char *pc = cname+12;
                                int iProcessid = strtol(pc, NULL, 10);
                                if(!strncmp(cname, "link_task1", 10))
                                {
                                    if(iProcessid>=0 && iProcessid<g_uiProcess_num && *pc != '_' && *(pc+1) != '_')
                                    {
                                        g_puiProcess_task[iProcessid] = 0;
                                        toupcam_log_f(LOG_INFO, "processor id(%d) cancel...\n", iProcessid);
                                    }
                                    else
                                    {
                                        toupcam_log_f(LOG_WARNNING, "abort processor id(%d) cancel...\n", iProcessid);
                                    }
                                    
                                    toupcam_log_f(LOG_INFO, "pthread_cancel(%s)...\n", cname);
                                    pthread_cancel(pid);
                                    g_PthreadId[id] = 0;
                                }
                            }
                        }
                        g_pstTouPcam->iconnect = 0;
                        toupcam_log_f(LOG_INFO, "sock unconnect...\n");
                        g_pstTouPcam->SaveConfigure(g_pstTouPcam);
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
    HRESULT hr;
    cpu_set_t mask;

    int processid = distribute_process();
    if(-1 != processid)
    {
        CPU_ZERO(&mask);
        CPU_SET(processid, &mask);
        if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
        {
            toupcam_log_f(LOG_WARNNING, "%s", strerror(errno));
        }
        toupcam_log_f(LOG_INFO, "thread id(%d) use process(%d)", (int)pthread_self(), processid);
    }

    while(1)
    {
        sleep(5*60);
        pthread_mutex_trylock(&g_PthreadMutexMonitor);
        /* 同步自动增益 */
        g_pstTouPcam->stTexpo.bAutoAGain = g_pstTouPcam->stTexpo.bAutoAGain;

        hr = Toupcam_get_ExpoAGain(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.AGain);
        if (FAILED(hr))
        {
            toupcam_log_f(LOG_WARNNING, "get brightness failly(%lld)!\n", hr);
        }

        /* 同步曝光 */
        hr = Toupcam_get_AutoExpoEnable(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.bAutoExposure);
        if (FAILED(hr))
        {
            toupcam_log_f(LOG_WARNNING, "get auto expo failly(%lld)!\n", hr);
        }
        hr = Toupcam_get_AutoExpoTarget(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.AutoTarget);
        if (FAILED(hr))
        {
            toupcam_log_f(LOG_WARNNING, "get brightness failly(%lld)!\n", hr);
        }

        /* 同步对比度 */
        g_pstTouPcam->stTcolor.bAutoColor = g_pstTouPcam->stTcolor.bAutoColor;
        hr = Toupcam_get_Contrast(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTcolor.Contrast);
        if (FAILED(hr))
        {
            toupcam_log_f(LOG_WARNNING, "get contrast failly(%lld)!\n", hr);
        }

        /* 同步直方图 */
        g_pstTouPcam->stHistoram.bAutoHis = g_pstTouPcam->stHistoram.bAutoHis;
        hr = Toupcam_get_LevelRange(g_pstTouPcam->m_hcam, g_pstTouPcam->stHistoram.aLow, g_pstTouPcam->stHistoram.aHigh);
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_WARNNING, "get histogram failly(%lld)!\n", hr);
        } 

        pthread_mutex_unlock(&g_PthreadMutexMonitor);
    }

	return NULL;
}

void *pthread_udp_distribute(void *pdata)
{
    cpu_set_t mask;

    int processid = distribute_process();
    if(-1 != processid)
    {
        CPU_ZERO(&mask);
        CPU_SET(processid, &mask);
        if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
        {
            toupcam_log_f(LOG_WARNNING, "%s", strerror(errno));
        }
        toupcam_log_f(LOG_INFO, "thread id(%d) use process(%d)", (int)pthread_self(), processid);
    }

    while(1)
    {
        if(NULL == pstFixFrameRate || 0 == pstFixFrameRate->getQueueNum())
        {
            sleep(1);
            continue;
        }
        
        if(pstFixFrameRate->getFixnum() < frame_num)
        {
            usleep(100000);
            continue;
        }
        
        unsigned char *ndata = (unsigned char *)pstFixFrameRate->PopQueue();
        if(NULL == ndata)
        {
            continue;
        }
            
        
#ifdef SOFT_ENCODE_H264
        encode_yuv((unsigned char *)ndata);
#else
        if(g_pstmpp_enc_data && g_pstmpp_enc_data->frame_count >= MPP_FRAME_MAXNUM)
        {
            mpp_ctx_deinit(&g_pstmpp_enc_data);
            init_mpp();
        }
        encode2hardware((unsigned char *)ndata);
#endif
        frame_num++;
        usleep(10000);
    }

}

void Destory_sock(void)
{
    if(sock)
    {
        close(sock->local);
        free(sock);
        sock = NULL;
        toupcam_log_f(LOG_INFO, "destory dgram sock...\n");
    }
    if(sock1)
    {
        close(sock1->local);
        free(sock1);
        sock1 = NULL;
        toupcam_log_f(LOG_INFO, "destory stream sock...\n");
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

    if(g_pstToupcamHashTable)
    {
        destoryHash(g_pstToupcamHashTable);
    }

    if(g_puiProcess_task)
    {
        free(g_puiProcess_task);
        g_puiProcess_task = NULL;
    }
#if 0
    if(g_pstTouPcam)
    {
        free(g_pstTouPcam);
        g_pstTouPcam = NULL;
    }
#endif   
}

int init_sock(void)
{
    int iRet = ERROR_SUCCESS;

    /* step 1 init dgram */
    iRet = socket_dgram_init();
    if(ERROR_SUCCESS == iRet)
    {
        toupcam_log_f(LOG_INFO, "dgram sock init is ok.\n");
    }
    else
    {
        toupcam_log_f(LOG_INFO, "dgram sock init is not ok.\n");
    }
    /* step 2 init stream */
    iRet = socket_stream_init();
    if(ERROR_SUCCESS == iRet)
    {
        toupcam_log_f(LOG_INFO, "stream sock init is ok.\n");
    }
    else
    {
        toupcam_log_f(LOG_INFO, "stream sock init is not ok.\n");
    }

	return iRet;
}

int init_cpu(void)
{
    int iRet = ERROR_SUCCESS;
    int num = sysconf(_SC_NPROCESSORS_CONF);
    if(num <= 0)
    {
        toupcam_log_f(LOG_ERROR, "there is no cpu");
        return ERROR_FAILED;
    }

    unsigned int *puiProcessTask = (unsigned int*)malloc(sizeof(unsigned int)*num);
    if(NULL == puiProcessTask)
    {
        toupcam_log_f(LOG_ERROR, "%s", strerror(errno));
        return ERROR_FAILED;
    }
    memset(puiProcessTask, 0, sizeof(unsigned int)*num);
    g_uiProcess_num = num;
    g_puiProcess_task = puiProcessTask;

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
    pthread_mutex_init(&g_PthreadMutexUDP, NULL); 
    return;
}

void pthread_mutex_destroys()
{
    /* 销毁线程锁 */
    pthread_mutex_destroy(&g_PthreadMutexJpgDest);
    pthread_mutex_destroy(&g_PthreadMutexMonitor);
    pthread_mutex_destroy(&g_PthreadMutexUDP);
    return;
}

static int SetupToupcam(void)
{
    unsigned int iRet = ERROR_SUCCESS;
    TOUPCAM_S *pstTouPcam = NULL;
    FILE * pFile = NULL;
    HRESULT hr;
    TOUPCAM_S stToupcam;
    char cPathCfg[128] = {0};
    snprintf(cPathCfg, 128, "%s%s", TOUPCAM_CFG_PATH, TOUCPAM_CFG_NAME);
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
    /*
    * 增加配置导入
    */
    if(NULL != (pFile = fopen(cPathCfg, "r")))
    {
        fclose(pFile);
        hr = toupcam_cfg_read(cPathCfg, g_pstToupcamHashTable, CFG_TO_NVR_DATA);
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_ERROR, "read cfg failed!\n");
            return ERROR_FAILED;
        }
        
        hr = toupcam_parse_cfg(g_pstToupcamHashTable, CFG_TO_NVR_DATA);
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_ERROR, "parse cfg failed!\n");
            return ERROR_FAILED;
        }

        hr = g_pstTouPcam->putReloadercfg(g_pstTouPcam, RELOADERBEFORE);
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_ERROR, "reloader cfg failed!\n");
            return ERROR_FAILED;
        }
    }
    else
    {
        /* disable cfg */
        g_pstTouPcam->cfg = 0;
        iRet = g_pstTouPcam->ConfigDevice((void *)g_pstTouPcam);
        if(ERROR_FAILED == iRet)
        {
            return ERROR_FAILED;
        }
    }
    iRet = g_pstTouPcam->StartDevice((void *)g_pstTouPcam);
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }    

    g_pstTouPcam->CloseDevice((void *)g_pstTouPcam);

    return ERROR_SUCCESS;
}

int daemon(int nochdir,int noclose)
{
	pid_t pid;
 
	/* 父进程产生子进程 */
	pid = fork();
	if (pid < 0)
	{
		perror("fork");
		return -1;
	}
    
	/* 父进程退出运行 */
	if (pid != 0)
	{
		exit(0);
	}
    
	/* 创建新的会话 */
	pid = setsid();
	if (pid < -1)
	{
		perror("set sid");
		return -1;
	}
    
	/* 更改当前工作目录,将工作目录修改成根目录 */
	if (!nochdir)
	{
		chdir("/");
	}
	/* 关闭文件描述符，并重定向标准输入，输出合错误输出
	 * 将标准输入输出重定向到空设备
	 */
	if (!noclose)
	{
		int fd;
		fd = open("/dev/null",O_RDWR,0);
		if (fd != -1)
		{
			dup2(fd,STDIN_FILENO);
			dup2(fd,STDOUT_FILENO);
			dup2(fd,STDERR_FILENO);
			if (fd > 2)
			{
				close(fd);
			}
		}
	}
	/* 设置守护进程的文件权限创建掩码 */
	umask(0027);
 
	return 0;
}


/*
* 主函数main
*/
int main(int, char**)
{
	int inWidth = 0, inHeight = 0;
    int iPthredArg = 0;
    int iRet = 0;
    pthread_t *pid = NULL;

    /* 开启一个守护进程 */
#if 0
    daemon(0, 0);
#endif

    iRet = init_toupcam_log();
    if(ERROR_FAILED == iRet)
    {
        goto exit0_;
    }

    /* 初始化sock数据,用于wifi网络视频帧传输和控制协议传输 */
    iRet = init_sock();
    if(ERROR_FAILED == iRet)
    {
        goto exit0_;
    }

    /* 检查CPU核数,初始化资源管理器             */
    iRet = init_cpu();
    if(ERROR_FAILED == iRet)
    {
        goto exit1_;
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
    pid = distribute_thread();
    if(NULL == pid)
    {
        goto exit0_;
    }
    iRet = pthread_create(pid, NULL, pthread_server, (void *)&iPthredArg);

	/* toupcam health's monitor thread */
    pid = distribute_thread();
    if(NULL == pid)
    {
        goto exit0_;
    }
    iRet += pthread_create(pid, NULL, pthread_health_monitor, (void *)&iPthredArg);
    if(ERROR_FAILED == iRet)
    {
        goto exit0_;
    }

	/* distribute udp's data */
    pid = distribute_thread();
    if(NULL == pid)
    {
        goto exit0_;
    }
    iRet += pthread_create(pid, NULL, pthread_udp_distribute, (void *)&iPthredArg);
    if(ERROR_FAILED == iRet)
    {
        goto exit0_;
    }

    /* 初始化信号 */    
    iRet = init_signals();
    if(ERROR_FAILED == iRet)
    {
        goto exit0_;
    }

    /* 初始化配置保存 */
    g_pstToupcamHashTable = createHash();
    if(NULL == g_pstToupcamHashTable)
    {
        goto exit0_;
    }

    /* 启动摄像头 */
    iRet = SetupToupcam();
    if(ERROR_FAILED == iRet)
    {
        goto exit1_;
    }

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
