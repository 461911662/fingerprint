#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../include/codeimg.h"
#include "../include/wificfg.h"
#include "../include/toupcamcfg.h"
#include "../include/libavutil/frame.h"
#include "../include/common_toupcam.h"


using namespace std;

extern "C"
{
#include "../include/x264.h"

#include "../include/libavcodec/avcodec.h"
#include "../include/libavformat/avformat.h"
#include "../include/libswscale/swscale.h"
#include "../include/libavutil/imgutils.h"

#include "../include/rtp.h"
#include "../include/Error.h"
#include "../include/socket.h"


#include "../include/toupcam.h"
#include "../include/jpeglib.h"


};

#define H264  /* 使用h264软件编码 */

#define CALLBAK_TOUPCAM_KIT 1
//#define LOWRESOLUTION 1

#define TOUPCAM_MIN_ID 0

TOUPCAM_S* g_pstTouPcam = NULL;

void OnEventError();
void OnEventDisconnected();
void OnEventImage();
void OnEventExpo();
void OnEventTempTint();
void OnEventStillImage();
static unsigned int SetAutoExpo_Toupcam();

COM_ENTRY_S g_Comm_Entry[] = {
    {COMCMD_WIFICFG, common_wifi_cmd},
    {COMCMD_TOUPCAMCFG, common_toupcam_cmd},
};

#define MANUEXPOTIME 1
#define AUTOEXPOTIME 1
COM_TOUPCAM_ENTRY_S g_ComToupcam_Entry[] = {
    {MANUEXPOTIME, .handle=NULL},
    {AUTOEXPOTIME, .handle=NULL},
};

/*
* Toupcam 通用回调函数集
*/
//unsigned int PreInitialDevice(TOUPCAM_S *pTmpToupcam)
unsigned int PreInitialDevice(void *pvoid)
{
    if(NULL == pvoid)
    {
        printf("%s: pvoid is null.\n", __func__);
        return ERROR_FAILED;
    }
    TOUPCAM_S *pTmpToupcam = (TOUPCAM_S *)pvoid;
    
    unsigned int iRSL_Num = 0; /* RSL is equal 'RESOLUTION' */
    int i, iWidth, iHeight, iMulResult, iRet = 0;
    int iMaxWidth, iMaxHeight;

    /* 设置分辨率 */
    iRSL_Num = (int)Toupcam_get_ResolutionNumber(pTmpToupcam->m_hcam);
    if(iRSL_Num > 0)
    {
        iRet = Toupcam_put_eSize(pTmpToupcam->m_hcam, iRSL_Num-1);  /* 预览适配最小分辨率 */
        iRet += Toupcam_get_Resolution(pTmpToupcam->m_hcam, iRSL_Num-1, &iWidth, &iHeight);
        iRet += Toupcam_get_Resolution(pTmpToupcam->m_hcam, 0, &iMaxWidth, &iMaxHeight);  /* 抓拍适配最大分辨率 */

        nWidth = iWidth;
        nHeight = iHeight;        
        pTmpToupcam->inWidth = iWidth;
        pTmpToupcam->inHeight = iHeight;
        pTmpToupcam->inMaxWidth = iMaxWidth;
        pTmpToupcam->inMaxHeight = iMaxHeight;
		pTmpToupcam->iSnapSize = TDIBWIDTHBYTES(iMaxWidth * 24) * iMaxHeight;
        pTmpToupcam->m_header.biSizeImage = TDIBWIDTHBYTES(iWidth * 24) * iHeight;
    }
    else
    {
        printf("%s: Toupcam get resolution number failed!\n", __func__);
        return ERROR_FAILED;
    }

    if(FAILED(iRet))
    {
        printf("%s: Toupcam failed.\n", __func__);
        return ERROR_FAILED;
    }

    /* 设置Toupcam自动曝光 */
    //iRet = SetAutoExpo_Toupcam();
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }

    return ERROR_SUCCESS;
}

unsigned int OpenDevice(void *pvoid)
{
    if(NULL == pvoid)
    {
        printf("%s: pvoid is null.\n", __func__);
        return ERROR_FAILED;
    }
    TOUPCAM_S *pTmpToupcam = (TOUPCAM_S *)pvoid;
    
    g_hcam = Toupcam_Open(NULL);
    if(NULL == g_hcam)
    {
        printf("no Toupcam device.\n");
        return ERROR_FAILED;
    }
    pTmpToupcam->m_hcam = g_hcam;

    return ERROR_SUCCESS;
}
unsigned int StartDevice(void)
{
}
unsigned int ConfigDevice(void)
{
}
unsigned int CloseDevice(void)
{
}

/*
* Toupcam 事件回调函数集
*/
#ifdef CALLBAK_TOUPCAM_KIT
void OnEventError()
{
    ;
}

void OnEventDisconnected()
{
    ;
}

void OnEventImage()
{
    ;
}

void OnEventExpo()
{
    ;
}

void OnEventTempTint()
{
    ;
}

void OnEventStillImage()
{
    ;
}

#else
__weak void OnEventError(){};
__weak void OnEventDisconnected(){};
__weak void OnEventImage(){};
__weak void OnEventExpo(){};
__weak void OnEventTempTint(){};
__weak void OnEventStillImage(){};
#endif
/*
* 初始化必要的数据结构
*/
static unsigned int _init(void)
{
    int iRet = ERROR_SUCCESS;
    /*申请Toupcam数据*/
    if(NULL != g_pstTouPcam)
    {
        free(g_pstTouPcam);
        g_pstTouPcam = NULL;
    }

    g_pstTouPcam = (TOUPCAM_S *)malloc(sizeof(TOUPCAM_S));
    if(NULL == g_pstTouPcam)
    {
        perror("_init");
        return ERROR_FAILED;
    }
    memset(g_pstTouPcam, 0, sizeof(TOUPCAM_S));

    g_pstTouPcam->m_header.biSize = sizeof(g_pstTouPcam->m_header);
    g_pstTouPcam->m_header.biPlanes = 1;
    g_pstTouPcam->m_header.biBitCount = 24;

    /*初始化Toupcam函数注册*/
    g_pstTouPcam->PreInitialDevice = PreInitialDevice;
    g_pstTouPcam->OpenDevice = OpenDevice;
    g_pstTouPcam->StartDevice = StartDevice;
    g_pstTouPcam->ConfigDevice = ConfigDevice;
    g_pstTouPcam->CloseDevice = CloseDevice;

    g_pstTouPcam->OnEventImage = OnEventImage;

    return iRet;
}

/*
* 打开一个Toupcam摄像头设备
*/

void* getToupcam()
{
    if(NULL != g_pstTouPcam)
    {
        return g_pstTouPcam;
    }

    return NULL;
}

/*
* 定制toupcam配置
*/
int setToupcamDefaultCfg()
{
    
}

/*
* 在系统上安装一个摄像头
*/
unsigned int SetupToupcam(TOUPCAM_S* pToupcam)
{
    int iRet = ERROR_SUCCESS;
    unsigned int uiCallbackContext = 0;
    if(NULL == pToupcam)
    {
        printf("%s: pTmpToupcam is null.\n", __func__);
        return ERROR_FAILED;
    }

    /* 打开Toupcam设备 */
    iRet = pToupcam->OpenDevice(pToupcam);
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }

    /* 设置Toupcam分辨率 */
    iRet = pToupcam->PreInitialDevice(pToupcam);
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }

    /* 申请足够大的数据Buffer1 preview         */
    if(pToupcam->m_header.biSizeImage > 0)
    {
        if(NULL == g_pImageData)
        {
            free(g_pImageData);
            g_pImageData = NULL;
        }
        g_pImageData = malloc(pToupcam->m_header.biSizeImage);
        if(NULL == g_pImageData)
        {
            perror("SetupToupcam g_pImageData");
            return ERROR_FAILED;
        }
    }
    pToupcam->m_pImageData = g_pImageData;

    /* buffer2 static picture */
    if(pToupcam->iSnapSize > 0)
    {
        if(NULL == g_pStaticImageData)
        {
            free(g_pStaticImageData);
            g_pStaticImageData = NULL;
        }
        g_pStaticImageData = malloc(pToupcam->iSnapSize);
        if(NULL == g_pStaticImageData)
        {
            perror("SetupToupcam g_pStaticImageData");
            return ERROR_FAILED;
        }
    }
    pToupcam->m_PStaticImageData = g_pStaticImageData;

    av_register_all();

    initX264Encoder(x264Encoder,"myCamera.h264");

    printf("pTmpToupcam->inWidth:%d, pTmpToupcam->inHeight:%d\n", g_pstTouPcam->inWidth, g_pstTouPcam->inHeight);
    printf("pTmpToupcam->inMaxWidth:%d, pTmpToupcam->inMaxHeight:%d\n", g_pstTouPcam->inMaxWidth, g_pstTouPcam->inMaxHeight);

    /* 开启拉模式视频 */
    iRet = Toupcam_StartPullModeWithCallback(pToupcam->m_hcam, EventCallback, &uiCallbackContext);
    if (FAILED(iRet))
    {
        printf("failed to start camera, hr = %08x\n", iRet);
    }
    else if(uiCallbackContext)
    {
        printf("Toupcam Event(%d):", uiCallbackContext);
        return toupcamerrinfo(uiCallbackContext)?ERROR_FAILED:ERROR_SUCCESS;
    }
    else
    {
        printf("press any key to exit\n");
        getc(stdin);
        return ERROR_FAILED;
    }
    
    return ERROR_SUCCESS;
}


/*
* 设置Toupcam曝光参数为自动
*/
static unsigned int SetAutoExpo_Toupcam()
{
    int  bEnableAutoExpo = 1;
    unsigned short  bEnableAutoExpo1 = 1;
    unsigned int nMinExpTime, nMaxExpTime, nDefExpTime;
    unsigned short nMaxAGain;
    if(NULL != g_pstTouPcam->m_hcam)
    {
        Toupcam_get_AutoExpoEnable(g_pstTouPcam->m_hcam, &bEnableAutoExpo);
    }
    else
    {
        printf("%s: m_hcam is null.\n", __func__);
        return ERROR_FAILED;
    }

    if(!bEnableAutoExpo)
    {
        bEnableAutoExpo = 1;
        Toupcam_put_AutoExpoEnable(g_pstTouPcam->m_hcam, bEnableAutoExpo);
        printf("auto expo is enable.\n");
    }

    Toupcam_get_AutoExpoTarget(g_pstTouPcam->m_hcam, &bEnableAutoExpo1);
    if(!bEnableAutoExpo1)
    {
        bEnableAutoExpo = 1;
        Toupcam_put_AutoExpoTarget(g_pstTouPcam->m_hcam, bEnableAutoExpo1);
        printf("auto expo target is enable.\n");
    }

    /*Toupcam_put_MaxAutoExpoTimeAGain();*/
    Toupcam_get_MaxAutoExpoTimeAGain(g_pstTouPcam->m_hcam, &nMaxExpTime, &nMaxAGain);
     printf("expo time max:%u, default:%u.\n", nMaxExpTime, nMaxAGain);
    
}

/*
* 设置Toupcam曝光参数为手动，并设置对应参数
*/
static unsigned int SetManuExpo_Toupcam()
{

}

/*
* 初始化Toupcam数据结构
*/
unsigned int init_Toupcam(void)
{
    int iRet = 0;
    TOUPCAM_S* pToupcam = g_pstTouPcam;

    /*初始化Toupcam数据结构中必要的函数*/
    iRet = _init();
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }
    
    return ERROR_SUCCESS;
}




