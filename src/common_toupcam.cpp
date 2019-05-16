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
extern MPP_ENC_DATA_S *g_pstmpp_enc_data;
extern MPP_RET mpp_ctx_deinit(MPP_ENC_DATA_S **data);

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
static unsigned int Set_HistogramToupcam();
static int init_Toupcam_button();


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
        toupcam_log_f(LOG_ERROR, "pvoid is null.\n");
        return ERROR_FAILED;
    }
    TOUPCAM_S *pTmpToupcam = (TOUPCAM_S *)pvoid;
    HRESULT hr;
    hr = Toupcam_get_SerialNumber(pTmpToupcam->m_hcam, pTmpToupcam->m_ins.sn);
    if(FAILED(hr))
    {
        toupcamerrinfo(hr);
    }
    int i = 0;
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
    printf("#############\033[40;31mlaplat camera program start \033[0m##############\n");
    printf("#######################################################\n\n");

    printf("%17s\033[40;31m%s\033[0m\n", "Toupcam sn:", pTmpToupcam->m_ins.sn[0]?pTmpToupcam->m_ins.sn:" null");
    printf("%17s\033[40;31m%s\033[0m\n", "Toupcam fwver:", pTmpToupcam->m_ins.fwver[0]?pTmpToupcam->m_ins.fwver:" null");
    printf("%17s\033[40;31m%s\033[0m\n", "Toupcam hwver:", pTmpToupcam->m_ins.hwver[0]?pTmpToupcam->m_ins.hwver:" null");
    printf("%17s\033[40;31m%s\033[0m\n", "Toupcam date:", pTmpToupcam->m_ins.pdate[0]?pTmpToupcam->m_ins.pdate:" null");
    printf("%17s\033[40;31m%d\033[0m\n", "Toupcam Revision:", pTmpToupcam->m_ins.pRevision?pTmpToupcam->m_ins.pRevision:0);

    if(!pTmpToupcam->m_ins.sn[0] || !pTmpToupcam->m_ins.fwver[0] || !pTmpToupcam->m_ins.hwver[0]
        || !pTmpToupcam->m_ins.pdate[0] || !pTmpToupcam->m_ins.pRevision)
    {
        return ERROR_FAILED;
    }

    return ERROR_SUCCESS;
}

static void *pthread_running(void *_val)
{
    int iRet = ERROR_SUCCESS;
    pthread_detach(pthread_self());
    toupcam_log_f(LOG_INFO, "task thread is running");
    sleep(5); /* 等待摄像机的启动 */

    /* 设置Toupcam自动曝光 */
    iRet = SetAutoExpo_Toupcam();
    if(ERROR_FAILED == iRet)
    {
        toupcam_log_f(LOG_INFO, "set auto Expo failed");
    }

    /* 白平衡设置 */
    /*
    * 白平衡只能在相机启动后push/pull使用
    */
    iRet = Set_WhiteBalanceToupcam();
    if(ERROR_FAILED == iRet)
    {
        toupcam_log_f(LOG_INFO, "set white balance failed");
    }

    /*  初始化直方图 */
    iRet = Set_HistogramToupcam();/* 同上 */
    if(ERROR_FAILED == iRet)
    {
        toupcam_log_f(LOG_INFO, "set histogram failed");
    }    

    pthread_exit(_val);
    return NULL;
}

unsigned int SufInitialDevice(void *pvoid)
{
    if(NULL == pvoid)
    {
        toupcam_log_f(LOG_ERROR, "pvoid is null.\n");
        return ERROR_FAILED;
    }
    TOUPCAM_S *pTmpToupcam = (TOUPCAM_S *)pvoid;
    HRESULT hr;
    void **retval;
    pthread_t pdonce;
    hr = pthread_create(&pdonce, NULL, pthread_running, *retval);
    if(hr != 0)
    {
        toupcam_log_f(LOG_WARNNING, "%s", strerror(errno));
        return ERROR_FAILED;
    }
    pthread_join(pdonce, retval);
    toupcam_log_f(LOG_INFO, "task thread is exit");
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
        toupcam_log_f(LOG_ERROR, "no Toupcam device.\n");
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
        toupcam_log_f(LOG_ERROR, "pvoid is null.\n");
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
#ifdef SOFT_ENCODE_H264
    av_register_all();

    initX264Encoder(x264Encoder,"myCamera.h264");
#else
    toupcam_log_f(LOG_WARNNING, "not soft_encode_h264.\n");
    if(g_pstmpp_enc_data && g_pstmpp_enc_data->frame_count >= MPP_FRAME_MAXNUM)
    {
        mpp_ctx_deinit(&g_pstmpp_enc_data);
    }
    init_mpp();
#endif
    toupcam_dbg_f(LOG_DEBUG, "pTmpToupcam->inWidth:%d, pTmpToupcam->inHeight:%d\n", g_pstTouPcam->inWidth, g_pstTouPcam->inHeight);
    toupcam_dbg_f(LOG_DEBUG, "pTmpToupcam->inMaxWidth:%d, pTmpToupcam->inMaxHeight:%d\n", g_pstTouPcam->inMaxWidth, g_pstTouPcam->inMaxHeight);

    sleep(10);
    /* 开启拉模式视频 */
    iRet = Toupcam_StartPullModeWithCallback(pToupcam->m_hcam, EventCallback, &uiCallbackContext);
    if (FAILED(iRet))
    {
        toupcam_log_f(LOG_ERROR, "failed to start camera, hr = %08x\n", iRet);
    }
    else if(uiCallbackContext)
    {
        toupcam_log_f(LOG_ERROR, "Toupcam Event(%d):", uiCallbackContext);
        return toupcamerrinfo(uiCallbackContext)?ERROR_FAILED:ERROR_SUCCESS;
    }
    else
    {
        SufInitialDevice(g_pstTouPcam);
        toupcam_log_f(LOG_WARNNING, "press any B/b to exit\n");
        while(1)
        {
            if('b' == getc(stdin) || 'B' == getc(stdin))
            {
                break;
            }
        }
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
        toupcam_log_f(LOG_ERROR, "pvoid is null.\n");
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
        //iRet = Toupcam_put_eSize(pTmpToupcam->m_hcam, 0);  /* 预览适配最大分辨率 */
        //iRet += Toupcam_get_Resolution(pTmpToupcam->m_hcam, 0, &iWidth, &iHeight);

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
        toupcam_log_f(LOG_ERROR, "Toupcam get resolution number failed!\n");
        return ERROR_FAILED;
    }

    if(FAILED(iRet))
    {
        toupcam_log_f(LOG_ERROR, "Toupcam failed.\n", __func__);
        return ERROR_FAILED;
    }

    /* 颜色设置 */
    iRet = Set_ColorToupcam();
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }

    /* 帧率设置 */
    g_pstTouPcam->iFrameRate = 10;
    iRet = Toupcam_put_Option(g_pstTouPcam->m_hcam, TOUPCAM_OPTION_FRAMERATE, g_pstTouPcam->iFrameRate);
    if(FAILED(iRet))
    {
        toupcam_log_f(LOG_ERROR, "set frame rate failed.\n");
        return ERROR_FAILED;
    }
    toupcam_dbg_f(LOG_DEBUG, "get frame rate:%d.", g_pstTouPcam->iFrameRate);

    /* 色彩模式设置 */
    iRet = Toupcam_get_Chrome(g_pstTouPcam->m_hcam, &g_pstTouPcam->m_color);
    if(FAILED(iRet))
    {
        toupcam_log_f(LOG_ERROR, "get chrome failed.\n");
        return ERROR_FAILED;
    }
    toupcam_dbg_f(LOG_DEBUG, "get chrome:%s.", g_pstTouPcam->m_color?"color":"gray");
  
    /* 翻转设置 */
    iRet = Set_ToupcamOrientation();
    if(ERROR_FAILED == iRet)
    {
        return ERROR_FAILED;
    }

    /* 采样方式设置 */
    /*
    iRet = Toupcam_get_Mode(g_pstTouPcam->m_hcam, &g_pstTouPcam->m_sample);
    if(FAILED(iRet))
    {
        toupcam_log_f(LOG_ERROR, "get sample failed.\n");
        //return ERROR_FAILED;
    }
    toupcam_dbg_f(LOG_DEBUG, "get sample:%s.", g_pstTouPcam->m_sample?"skip":"bin");
    */

    /* 光源频率设置 */
    iRet = Toupcam_get_HZ(g_pstTouPcam->m_hcam, &g_pstTouPcam->m_nHz);
    if(FAILED(iRet))
    {
        toupcam_log_f(LOG_ERROR, "get power source Hz failed.\n");
        return ERROR_FAILED;
    }
    toupcam_dbg_f(LOG_DEBUG, "power source :%s.", g_pstTouPcam->m_nHz?(g_pstTouPcam->m_nHz == 1?"50Hz":"direct-currnet"):"60Hz");

    /* 初始化toupcam Button */
    iRet = init_Toupcam_button();
    if(FAILED(iRet))
    {
        toupcam_log_f(LOG_ERROR, "init button failed.\n");
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
        toupcam_log_f(LOG_ERROR, "pvoid is null.\n");
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
    HRESULT hr;
    if(g_pstTouPcam->stTexpo.bAutoExposure)
    {
        hr = Toupcam_get_ExpoTime(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.Time);
        if(FAILED(hr))
        {
            //printf("%s: get expo time failed!\n", __func__);
        }
    }
    else
    {
        //printf("%s: cur expo mode:%s.\n", __func__, g_pstTouPcam->stTexpo.bAutoExposure?"auto":"manu");
    }
    return;
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
    g_pstTouPcam->SufInitialDevice = SufInitialDevice;

    g_pstTouPcam->OnEventImage = OnEventImage;
    g_pstTouPcam->OnEventExpo = OnEventExpo;

    return iRet;
}

/*
* 定制toupcam配置
*/
int setToupcamDefaultCfg()
{
    
}

/*
* 设置Toupcam曝光参数为自动
*/
static unsigned int SetAutoExpo_Toupcam()
{
    if(NULL == g_pstTouPcam->m_hcam)
    {
        toupcam_log_f(LOG_ERROR, "m_hcam is null.\n");
        return ERROR_FAILED;
    }

    /*初始化自动曝光锁*/
    pthread_mutex_init(&g_pstTouPcam->stTexpo.mutex, NULL);
    
    pthread_mutex_lock(&g_pstTouPcam->stTexpo.mutex);
    Toupcam_get_AutoExpoEnable(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.bAutoExposure);
    if(g_pstTouPcam->stTexpo.bAutoExposure)
    {
        g_pstTouPcam->stTexpo.bAutoExposure = 1;
        Toupcam_put_AutoExpoEnable(g_pstTouPcam->m_hcam, g_pstTouPcam->stTexpo.bAutoExposure);
    }
    toupcam_dbg_f(LOG_DEBUG, "auto expo is %s.\n", g_pstTouPcam->stTexpo.bAutoExposure?"enable":"disable");

    Toupcam_get_AutoExpoTarget(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.AutoTarget);


    if(120 != g_pstTouPcam->stTexpo.AutoTarget) /* default expo 120 */
    {
        g_pstTouPcam->stTexpo.AutoTarget = 120;
        Toupcam_put_AutoExpoTarget(g_pstTouPcam->m_hcam, g_pstTouPcam->stTexpo.AutoTarget);
    }

    toupcam_dbg_f(LOG_DEBUG, "auto expo target is %d.\n", g_pstTouPcam->stTexpo.AutoTarget);

    /*Toupcam_put_MaxAutoExpoTimeAGain();*/
    Toupcam_get_MaxAutoExpoTimeAGain(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.AutoMaxTime, &g_pstTouPcam->stTexpo.AutoMaxAGain);
    Toupcam_get_RealExpoTime(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.Time);
    Toupcam_get_ExpTimeRange(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.nMin, &g_pstTouPcam->stTexpo.nMax, &g_pstTouPcam->stTexpo.nDef);
    Toupcam_get_ExpoAGain(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.AGain);
    Toupcam_get_ExpoAGainRange(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTexpo.AnMin, &g_pstTouPcam->stTexpo.AnMax, &g_pstTouPcam->stTexpo.AnDef);
    g_pstTouPcam->stTexpo.scale = 9.375;
    
    pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);

    toupcam_dbg_f(LOG_DEBUG, "expo param:\n");
    toupcam_dbg_f(LOG_DEBUG, "cur expo real time:%uus.\n", g_pstTouPcam->stTexpo.Time);
    toupcam_dbg_f(LOG_DEBUG, "cur max expo time Min:%uus,Max:%uus,Def:%uus.\n", g_pstTouPcam->stTexpo.nMin, g_pstTouPcam->stTexpo.nMax, g_pstTouPcam->stTexpo.nDef);
    toupcam_dbg_f(LOG_DEBUG, "cur expo AGain:%d.\n", g_pstTouPcam->stTexpo.AGain);
    toupcam_dbg_f(LOG_DEBUG, "cur max expo Value AMin:%u,AMax:%u,ADef:%u.\n", g_pstTouPcam->stTexpo.AnMin, g_pstTouPcam->stTexpo.AnMax, g_pstTouPcam->stTexpo.AnDef);
    toupcam_dbg_f(LOG_DEBUG, "cur max expo time:%u, max AGain time:%u.\n", g_pstTouPcam->stTexpo.AutoMaxTime, g_pstTouPcam->stTexpo.AutoMaxAGain);
    
    return ERROR_SUCCESS;
}

/*
* 设置Toupcam白平衡参数为自动
*/
static unsigned int Set_WhiteBalanceToupcam()
{
    if(NULL == g_pstTouPcam)
    {
        toupcam_log_f(LOG_ERROR, "g_pstToupcam is null\n");
        return ERROR_FAILED;
    }
    HRESULT hr = 0;
    int iTTCtx = 0;
    PITOUPCAM_TEMPTINT_CALLBACK fnTTProc = NULL;

    /* 初始化锁 */
    pthread_mutex_init(&g_pstTouPcam->stWhiteBlc.mutex, NULL);
#if 1
    /* set auto white balance mode as Temp/Tint */
    /* auto white balance "one push". This function must be called AFTER Toupcam_StartXXXX */
    hr = Toupcam_AwbOnePush(g_pstTouPcam->m_hcam, fnTTProc, (void *)&iTTCtx);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "set white balance fnTTProc failed!\n");
        //return ERROR_FAILED;
    }
    
    pthread_mutex_lock(&g_pstTouPcam->stWhiteBlc.mutex);
    g_pstTouPcam->stWhiteBlc.iauto = 1;

    hr = Toupcam_get_TempTint(g_pstTouPcam->m_hcam, &g_pstTouPcam->stWhiteBlc.Temp, &g_pstTouPcam->stWhiteBlc.Tint);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "init Temp,Tint failed!\n");
        pthread_mutex_unlock(&g_pstTouPcam->stWhiteBlc.mutex);
        //return ERROR_FAILED;
    }
    toupcam_dbg_f(LOG_DEBUG, "White balance Temp:%d, Tint:%d\n", g_pstTouPcam->stWhiteBlc.Temp, g_pstTouPcam->stWhiteBlc.Tint);
    pthread_mutex_unlock(&g_pstTouPcam->stWhiteBlc.mutex);
#endif
    return ERROR_SUCCESS;
}

/*
* 设置Toupcam颜色衡参数
*/
static unsigned int Set_ColorToupcam()
{
    if(NULL == g_pstTouPcam)
    {
        toupcam_log_f(LOG_ERROR, "g_pstToupcam is null\n");
        return ERROR_FAILED;
    }
    /*初始化自动曝光锁*/
    pthread_mutex_init(&g_pstTouPcam->stTcolor.mutex, NULL);
    pthread_mutex_lock(&g_pstTouPcam->stTcolor.mutex);
    g_pstTouPcam->stTcolor.Contrast = 0;
    HRESULT hr;
    hr = Toupcam_get_Contrast(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTcolor.Contrast);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "get Contrast failed!\n");
        pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);
        return ERROR_FAILED;
    }

    if(0 != g_pstTouPcam->stTcolor.Contrast)
    {
        hr = Toupcam_put_Contrast(g_pstTouPcam->m_hcam, g_pstTouPcam->stTcolor.Contrast);
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_ERROR, "put Contrast failed!\n");
            pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);
            return ERROR_FAILED;
        }
    }
    toupcam_dbg_f(LOG_DEBUG, "Toupcam get Contrast:%d.\n", g_pstTouPcam->stTcolor.Contrast);

    g_pstTouPcam->stTcolor.Gamma = 100;
    hr = Toupcam_get_Gamma(g_pstTouPcam->m_hcam, &g_pstTouPcam->stTcolor.Gamma);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "get Gamma failed!\n");
        pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);
        return ERROR_FAILED;
    }
    
    if(100 != g_pstTouPcam->stTcolor.Gamma)
    {
        hr = Toupcam_put_Gamma(g_pstTouPcam->m_hcam, g_pstTouPcam->stTcolor.Gamma);
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_ERROR, "init Gamma failed!\n");
            pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);
            return ERROR_FAILED;
        }
    }
    toupcam_dbg_f(LOG_DEBUG, "Toupcam get Gamma:%d.\n", g_pstTouPcam->stTcolor.Gamma);

    pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);
    return ERROR_SUCCESS;
}

/*
* 初始化直方图数据
*/
static unsigned int Set_HistogramToupcam()
{
    HRESULT hr;
    /* 初始化直方图数据锁 */
    pthread_mutex_init(&g_pstTouPcam->stHistoram.mutex, NULL);

    pthread_mutex_lock(&g_pstTouPcam->stHistoram.mutex);    
    /* 设置直方图为自动         */
    g_pstTouPcam->stHistoram.bAutoHis = TRUE;
    hr = Toupcam_LevelRangeAuto(g_pstTouPcam->m_hcam);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "set auto histogram failed.\n");
        return ERROR_FAILED;
    }
    
    /* 更新直方图范围 */
    hr = Toupcam_put_LevelRange(g_pstTouPcam->m_hcam, g_pstTouPcam->stHistoram.aLow, g_pstTouPcam->stHistoram.aHigh);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "updated histogram data failed.\n");
        pthread_mutex_unlock(&g_pstTouPcam->stHistoram.mutex);
        return ERROR_FAILED;
    }
    toupcam_log_f(LOG_DEBUG, "cur auto Histogram data:LR:[%d] LG:[%d] LB:[%d] LY:[%d]\n", 
                                g_pstTouPcam->stHistoram.aLow[0],
                                g_pstTouPcam->stHistoram.aLow[1],
                                g_pstTouPcam->stHistoram.aLow[2],
                                g_pstTouPcam->stHistoram.aLow[3]);
    toupcam_log_f(LOG_DEBUG, "cur auto Histogram data:HR:[%d] HG:[%d] HB:[%d] HY:[%d]\n", 
                                g_pstTouPcam->stHistoram.aHigh[0],
                                g_pstTouPcam->stHistoram.aHigh[1],
                                g_pstTouPcam->stHistoram.aHigh[2],
                                g_pstTouPcam->stHistoram.aHigh[3]);    
    pthread_mutex_unlock(&g_pstTouPcam->stHistoram.mutex);

    return ERROR_SUCCESS;    
}

/*
* 设置Toupcam图像方向
*/
static unsigned int Set_ToupcamOrientation()
{
    if(NULL == g_pstTouPcam)
    {
        toupcam_log_f(LOG_ERROR, "g_pstToupcam is null\n");
        return ERROR_FAILED;
    }
    /*
    HRESULT hr = Toupcam_put_VFlip(g_pstTouPcam->m_hcam, 0);
    if(FAILED(hr))
    {
        printf("init vFlip failed!\n");
        return ERROR_FAILED;
    }*/
    HRESULT hr = Toupcam_get_VFlip(g_pstTouPcam->m_hcam, &g_pstTouPcam->iVFlip);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "init vFlip failed!\n");
        return ERROR_FAILED;
    }
    toupcam_dbg_f(LOG_DEBUG, "Toupcam VFlip picture:%s\n", g_pstTouPcam->iVFlip?"negative":"positive");

    hr = Toupcam_put_HFlip(g_pstTouPcam->m_hcam, 1);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "init Hflip failed!\n");
        return ERROR_FAILED;
    }    
    hr = Toupcam_get_HFlip(g_pstTouPcam->m_hcam, &g_pstTouPcam->iHFlip);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "init Hflip failed!\n");
        return ERROR_FAILED;
    }
    toupcam_dbg_f(LOG_DEBUG, "Toupcam HFlip picture:%s\n", g_pstTouPcam->iHFlip?"positive":"negative");

    g_pstTouPcam->iVFlip = 0;
    g_pstTouPcam->iHFlip = 0;


    /* 图像倒置 */
    hr = Toupcam_put_Option(g_pstTouPcam->m_hcam, TOUPCAM_OPTION_UPSIDE_DOWN, 0);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "set frame rate failed.\n");
        return ERROR_FAILED;
    }

#ifdef ROI_IMAGE
    nWidth = 640, nHeight = 480;
    //WIDTH = nWidth, HEIGHT = nHeight;
    g_pstTouPcam->inHeight = g_pstTouPcam->inMaxHeight = 480;
    g_pstTouPcam->inWidth = g_pstTouPcam->inMaxWidth = 640;
    Toupcam_put_Roi(g_hcam, 200, 200, 640, 480);
#endif

    return ERROR_SUCCESS;
}

/*
* 设置Toupcam对应开关值
*/
static int init_Toupcam_button()
{
    g_pstTouPcam->stTexpo.bAutoExposure = 1;
    g_pstTouPcam->stTexpo.bAutoAGain = 0;
    g_pstTouPcam->stTcolor.bAutoColor = 0;
    g_pstTouPcam->stHistoram.bAutoHis = 0;
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
        toupcam_log_f(LOG_ERROR, "input is invaild.\n");
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


