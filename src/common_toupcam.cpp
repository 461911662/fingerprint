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
static unsigned int Set_WhiteBalanceToupcam();
static unsigned int Set_ColorToupcam();
static unsigned int Set_ToupcamOrientation();


COM_ENTRY_S g_Comm_Entry[] = {
    {COMCMD_WIFICFG, common_wifi_cmd},
    {COMCMD_TOUPCAMCFG, common_toupcam_cmd},
};
unsigned int g_Comm_Entry_Size = ARRAY_SIZE(g_Comm_Entry);

#define MANUEXPOTIME 1
#define AUTOEXPOTIME 1
COM_TOUPCAM_ENTRY_S g_ComToupcam_Entry[] = {
    {MANUEXPOTIME, .handle=NULL},
    {AUTOEXPOTIME, .handle=NULL},
};

/*
* Toupcam 提前初始化函数
*/
unsigned int PreInitialDevice(void *pvoid)
{
    if(NULL == pvoid)
    {
        printf("%s: pvoid is null.\n", __func__);
        return ERROR_FAILED;
    }
    TOUPCAM_S *pTmpToupcam = (TOUPCAM_S *)pvoid;
    HRESULT hr;
    hr = Toupcam_get_SerialNumber(pTmpToupcam->m_hcam, pTmpToupcam->m_ins.sn);
    if(FAILED(hr))
    {
        toupcamerrinfo(hr);
    }
    
    hr = Toupcam_get_FwVersion(pTmpToupcam->m_hcam, pTmpToupcam->m_ins.fwver);
    if(FAILED(hr))
    {
        toupcamerrinfo(hr);
    }

    hr = Toupcam_get_HwVersion(pTmpToupcam->m_hcam, pTmpToupcam->m_ins.hwver);
    if(FAILED(hr))
    {
        toupcamerrinfo(hr);
    }

    hr = Toupcam_get_ProductionDate(pTmpToupcam->m_hcam, pTmpToupcam->m_ins.pdate);
    if(FAILED(hr))
    {
        toupcamerrinfo(hr);
    }

    hr = Toupcam_get_Revision(pTmpToupcam->m_hcam, &pTmpToupcam->m_ins.pRevision);
    if(FAILED(hr))
    {
        toupcamerrinfo(hr);
    }

    printf("#######################################################\n");
    printf("############ \033[40;31mlaplat camera program start \033[0m##############\n");
    printf("#######################################################\n\n");

    printf("%17s\033[40;31%s\033[0m\n", "Toupcam sn:", pTmpToupcam->m_ins.sn[0]?pTmpToupcam->m_ins.sn:"null");
    printf("%17s\033[40;31%s\033[0m\n", "Toupcam fwver:", pTmpToupcam->m_ins.fwver[0]?pTmpToupcam->m_ins.fwver:"null");
    printf("%17s\033[40;31%s\033[0m\n", "Toupcam hwver:", pTmpToupcam->m_ins.hwver[0]?pTmpToupcam->m_ins.hwver:"null");
    printf("%17s\033[40;31%s\033[0m\n", "Toupcam date:", pTmpToupcam->m_ins.pdate[0]?pTmpToupcam->m_ins.pdate:"null");
    printf("%17s\033[40;31%d\033[0m\n", "Toupcam Revision:", pTmpToupcam->m_ins.pRevision?pTmpToupcam->m_ins.pRevision:0);

    if(!pTmpToupcam->m_ins.sn[0] || !pTmpToupcam->m_ins.fwver[0] || !pTmpToupcam->m_ins.hwver[0]
        || !pTmpToupcam->m_ins.pdate[0] || !pTmpToupcam->m_ins.pRevision)
    {
        return ERROR_FAILED;
    }

    return ERROR_SUCCESS;
}


/*
* 打开一个摄像头设备
*/
unsigned int OpenDevice()
{ 
    g_hcam = Toupcam_Open(NULL);
    if(NULL == g_hcam)
    {
        printf("no Toupcam device.\n");
        return ERROR_FAILED;
    }
    g_pstTouPcam->m_hcam = g_hcam;

    return ERROR_SUCCESS;
}

/*
* 开启摄像头设备
*/
unsigned int StartDevice(void *pvoid)
{
    if(NULL == pvoid)
    {
        printf("%s: pvoid is null.\n", __func__);
        return ERROR_FAILED;
    }
    TOUPCAM_S *pToupcam = (TOUPCAM_S *)pvoid;
    unsigned int uiCallbackContext = 0;
    int iRet = ERROR_SUCCESS;
    
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
* 配置一个摄像头设备
*/
unsigned int ConfigDevice(void *pvoid)
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
        /*
        pTmpToupcam->inMaxWidth = iMaxWidth;
        pTmpToupcam->inMaxHeight = iMaxHeight;
		pTmpToupcam->iSnapSize = TDIBWIDTHBYTES(iMaxWidth * 24) * iMaxHeight;
		*/
        pTmpToupcam->inMaxWidth = iWidth;
        pTmpToupcam->inMaxHeight = iHeight;
		pTmpToupcam->iSnapSize = TDIBWIDTHBYTES(iWidth * 24) * iHeight;
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
    iRet = SetAutoExpo_Toupcam();
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }

    /* 白平衡设置 */
    iRet = Set_WhiteBalanceToupcam();
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }
   
    /* 颜色设置 */
    iRet = Set_ColorToupcam();
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }

    /* 帧率设置 */
    g_pstTouPcam->iFrameRate = 30;
    iRet = Toupcam_put_Option(g_pstTouPcam->m_hcam, TOUPCAM_OPTION_FRAMERATE, g_pstTouPcam->iFrameRate);
    if(FAILED(iRet))
    {
        printf("%s: set frame rate failed.\n", __func__);
        return ERROR_FAILED;
    }

    /* 色彩模式设置 */
    iRet = Toupcam_get_Chrome(g_pstTouPcam->m_hcam, &g_pstTouPcam->m_color);
    if(FAILED(iRet))
    {
        printf("%s: get chrome failed.\n", __func__);
        return ERROR_FAILED;
    }
  
    /* 翻转设置 */
    iRet = Set_ToupcamOrientation();
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }

    /* 采样方式设置 */
    iRet = Toupcam_get_Mode(g_pstTouPcam->m_hcam, &g_pstTouPcam->m_sample);
    if(FAILED(iRet))
    {
        printf("%s: get sample failed.\n", __func__);
        //return ERROR_FAILED;
    }

    /* 光源频率设置 */
    iRet = Toupcam_get_HZ(g_pstTouPcam->m_hcam, &g_pstTouPcam->m_nHz);
    if(FAILED(iRet))
    {
        printf("%s: get power source Hz failed.\n", __func__);
        return ERROR_FAILED;
    }

    return ERROR_SUCCESS;


}

/*
* 关掉一个摄像头设备
*/
unsigned int CloseDevice(void *pvoid)
{
    if(NULL == pvoid)
    {
        printf("%s: pvoid is null.\n", __func__);
        return ERROR_FAILED;
    }
    TOUPCAM_S *pTmpToupcam = (TOUPCAM_S *)pvoid;
    
    Toupcam_Close(pTmpToupcam->m_hcam);
    pTmpToupcam->m_hcam = NULL;
    g_hcam = NULL;

    return ERROR_SUCCESS;
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

    g_pstTouPcam->bNegative = 0;   /* 设置相机图像不反转 */
    g_pstTouPcam->m_header.biSize = sizeof(g_pstTouPcam->m_header);
    g_pstTouPcam->m_header.biPlanes = 1;
    g_pstTouPcam->m_header.biBitCount = 24;

    /*初始化Toupcam函数注册*/
    g_pstTouPcam->OpenDevice = OpenDevice;
    g_pstTouPcam->PreInitialDevice = PreInitialDevice;
    g_pstTouPcam->StartDevice = StartDevice;
    g_pstTouPcam->ConfigDevice = ConfigDevice;
    g_pstTouPcam->CloseDevice = CloseDevice;

    g_pstTouPcam->OnEventImage = OnEventImage;

    return iRet;
}

/*
* 定制toupcam配置
*/
int setToupcamDefaultCfg()
{
    
}

#if 0
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
#endif

/*
* 设置Toupcam曝光参数为自动
*/
static unsigned int SetAutoExpo_Toupcam()
{
    if(NULL == g_pstTouPcam->m_hcam)
    {
        printf("%s: m_hcam is null.\n", __func__);
        return ERROR_FAILED;
    }

    /*初始化自动曝光锁*/
    pthread_mutex_init(&g_pstTouPcam->stTexpo.mutex, NULL);
    
    pthread_mutex_lock(&g_pstTouPcam->stTexpo.mutex);
    Toupcam_get_AutoExpoEnable(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.bAutoExposure);
    if(!g_pstTouPcam->stTexpo.bAutoExposure)
    {
        g_pstTouPcam->stTexpo.bAutoExposure = 1;
        Toupcam_put_AutoExpoEnable(g_pstTouPcam->m_hcam, g_pstTouPcam->stTexpo.bAutoExposure);
        printf("auto expo is enable.\n");
    }

    Toupcam_get_AutoExpoTarget(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.AutoTarget);
    if(120 != g_pstTouPcam->stTexpo.AutoTarget) /* default expo 120 */
    {
        g_pstTouPcam->stTexpo.AutoTarget = 120;
        Toupcam_put_AutoExpoTarget(g_pstTouPcam->m_hcam, g_pstTouPcam->stTexpo.AutoTarget);
        printf("auto expo target is %d.\n", g_pstTouPcam->stTexpo.AutoTarget);
    }

    /*Toupcam_put_MaxAutoExpoTimeAGain();*/
    Toupcam_get_MaxAutoExpoTimeAGain(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.AutoMaxTime, &g_pstTouPcam->stTexpo.AutoMaxAGain);
    Toupcam_get_RealExpoTime(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.Time);
    Toupcam_get_ExpTimeRange(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.nMin, &g_pstTouPcam->stTexpo.nMax, &g_pstTouPcam->stTexpo.nDef);
    Toupcam_get_ExpoAGainRange(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.AnMin, &g_pstTouPcam->stTexpo.AnMax, &g_pstTouPcam->stTexpo.AnDef);
    g_pstTouPcam->stTexpo.scale = 9.375;
    pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);

    printf("expo param:\n");
    printf("cur expo real time:%u.\n", g_pstTouPcam->stTexpo.Time);
    printf("cur max expo time Min:%u,Max:%u,Def:%u.\n", g_pstTouPcam->stTexpo.nMin, g_pstTouPcam->stTexpo.nMax, g_pstTouPcam->stTexpo.nDef);
    printf("cur max expo Value AMin:%u,AMax:%u,ADef:%u.\n", g_pstTouPcam->stTexpo.AnMin, g_pstTouPcam->stTexpo.AnMax, g_pstTouPcam->stTexpo.AnDef);
    printf("cur max expo time:%u, max AGain time:%us.\n", g_pstTouPcam->stTexpo.AutoMaxTime, g_pstTouPcam->stTexpo.AutoMaxAGain);
    
    return ERROR_SUCCESS;
}

/*
* 设置Toupcam白平衡参数为自动
*/
static unsigned int Set_WhiteBalanceToupcam()
{
    if(NULL == g_pstTouPcam)
    {
        printf("%s: g_pstToupcam is null\n", __func__);
        return ERROR_FAILED;
    }
    HRESULT hr = 0;
    int iTTCtx = 0;
    PITOUPCAM_TEMPTINT_CALLBACK fnTTProc = NULL;

    /* set auto white balance mode as Temp/Tint */
    /* auto white balance "one push". This function must be called AFTER Toupcam_StartXXXX */
    hr = Toupcam_AwbOnePush(g_pstTouPcam->m_hcam, fnTTProc, (void *)&iTTCtx);
    if(FAILED(hr))
    {
        printf("set white balance fnTTProc failed!\n");
        //return ERROR_FAILED;
    }

    /* 初始化锁 */
    pthread_mutex_init(&g_pstTouPcam->stWhiteBlc.mutex, NULL);
    
    pthread_mutex_lock(&g_pstTouPcam->stWhiteBlc.mutex);
    g_pstTouPcam->stWhiteBlc.iauto = 1;

    hr = Toupcam_get_TempTint(g_pstTouPcam->m_hcam, &g_pstTouPcam->stWhiteBlc.Temp, &g_pstTouPcam->stWhiteBlc.Tint);
    if(FAILED(hr))
    {
        printf("init Temp,Tint failed!\n");
        pthread_mutex_unlock(&g_pstTouPcam->stWhiteBlc.mutex);
        //return ERROR_FAILED;
    }
    printf("White balance Temp:%d, Tint:%d\n", g_pstTouPcam->stWhiteBlc.Temp, g_pstTouPcam->stWhiteBlc.Tint);
    pthread_mutex_unlock(&g_pstTouPcam->stWhiteBlc.mutex);

    return ERROR_SUCCESS;
}

/*
* 设置Toupcam颜色衡参数
*/
static unsigned int Set_ColorToupcam()
{
    if(NULL == g_pstTouPcam)
    {
        printf("%s: g_pstToupcam is null\n", __func__);
        return ERROR_FAILED;
    }
    /*初始化自动曝光锁*/
    pthread_mutex_init(&g_pstTouPcam->stTcolor.mutex, NULL);
    pthread_mutex_lock(&g_pstTouPcam->stTcolor.mutex);
    g_pstTouPcam->stTcolor.Contrast = 0;
    HRESULT hr = Toupcam_put_Contrast(g_pstTouPcam->m_hcam, g_pstTouPcam->stTcolor.Contrast);
    if(FAILED(hr))
    {
        printf("init Color failed!\n");
        pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);
        return ERROR_FAILED;
    }

    g_pstTouPcam->stTcolor.Gamma = 100;
    hr = Toupcam_put_Gamma(g_pstTouPcam->m_hcam, g_pstTouPcam->stTcolor.Gamma);
    if(FAILED(hr))
    {
        printf("init Color failed!\n");
        pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);
        return ERROR_FAILED;
    }    

    pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);
    return ERROR_SUCCESS;
}

/*
* 设置Toupcam图像方向
*/
static unsigned int Set_ToupcamOrientation()
{
    if(NULL == g_pstTouPcam)
    {
        printf("%s: g_pstToupcam is null\n", __func__);
        return ERROR_FAILED;
    }
    HRESULT hr = Toupcam_get_VFlip(g_pstTouPcam->m_hcam, &g_pstTouPcam->iVFlip);
    if(FAILED(hr))
    {
        printf("init vFlip failed!\n");
        return ERROR_FAILED;
    }
    hr = Toupcam_get_HFlip(g_pstTouPcam->m_hcam, &g_pstTouPcam->iHFlip);
    if(FAILED(hr))
    {
        printf("init Hflip failed!\n");
        return ERROR_FAILED;
    }

    /* 图像倒置 */
    hr = Toupcam_put_Option(g_pstTouPcam->m_hcam, TOUPCAM_OPTION_UPSIDE_DOWN, 1);
    if(FAILED(hr))
    {
        printf("%s: set frame rate failed.\n", __func__);
        return ERROR_FAILED;
    }

    return ERROR_SUCCESS;
}


/*
* 初始化Toupcam数据结构
*/
unsigned int init_Toupcam(void *pdata)
{
    if(NULL != pdata)
    {
        free(pdata);
        pdata = NULL;
    }
    int iRet = ERROR_SUCCESS;

    /*初始化Toupcam数据结构中必要的函数*/
    iRet = _init();
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }
    
    return ERROR_SUCCESS;
}

/*
* Toupcam 应答通用头
*/
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
    respon->com.size[0] = INVAILD_BUFF_SIZE;
    respon->com.size[1] = 0;
    respon->cc = ERROR_SUCCESS;
    memset(respon->data.reserve, 0, ARRAY_SIZE(respon->data.reserve));

    return ERROR_SUCCESS;
}


