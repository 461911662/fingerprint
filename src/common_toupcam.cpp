#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include "../include/codeimg.h"
#include "../include/wificfg.h"
#include "../include/toupcamcfg.h"
#include "../include/libavutil/frame.h"
#include "../include/common_toupcam.h"
#include "../include/toupcam_data.h"
#include "../include/toupcam_parse.h"
#include "../include/fixframerate.h"

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
extern HashTable *g_pstToupcamHashTable;
extern FixFrameRate *pstFixFrameRate;
extern int frame_num;


#define CALLBAK_TOUPCAM_KIT 1
//#define LOWRESOLUTION 1

#define TOUPCAM_MIN_ID 0

TOUPCAM_S* g_pstTouPcam = NULL;
TOUPCAM_S  stTouPcam;

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
static unsigned int Set_BlackBalanceToupcam();
unsigned int putReloadercfg(void *pvoid, int method);
static unsigned int Set_ReToupcamOrientation();
static unsigned int SetReAutoExpo_Toupcam();
static unsigned int Set_ReWhiteBalanceToupcam();
static unsigned int Set_ReBlackBalanceToupcam();
static unsigned int Set_ReHistogramToupcam();
static unsigned int Set_ReColorToupcam();

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
* 白平衡回调 TEMP/TINT
*/
void __stdcall _PITOUPCAM_TEMPTINT_CALLBACK(const int nTemp, const int nTint, void* pCtx)
{
    return ;
}

/*
* 白平衡回调 aGain: R G B
*/
void __stdcall _PITOUPCAM_WHITEBALANCE_CALLBACK(const int aGain[3], void* pCtx)
{
    return;
}


/*
* 黑平衡回调
*/
void __stdcall _PITOUPCAM_BLACKBALANCE_CALLBACK(const unsigned short aSub[3], void* pCtx)
{
    return ;
}

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

    /* 黑平衡设置 */
    iRet = Set_BlackBalanceToupcam();
    if(ERROR_FAILED == iRet)
    {
        toupcam_log_f(LOG_INFO, "set black balance failed");
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

unsigned int SaveConfigure(void *pvoid)
{
    if(NULL == pvoid)
    {
        toupcam_log_f(LOG_ERROR, "pvoid is null.\n");
        return ERROR_FAILED;
    }
    TOUPCAM_S *pTmpToupcam = (TOUPCAM_S *)pvoid;
    HRESULT hr;
    char cPathCfg[128] = {0};
    DIR *mDir = opendir(TOUPCAM_CFG_PATH);
    if(NULL == mDir)
    {
        toupcam_log_f(LOG_WARNNING, "%s", strerror(errno));
        if(ERROR_SUCCESS != mkdir(TOUPCAM_CFG_PATH, 755))
        {
            toupcam_log_f(LOG_ERROR, "%s", strerror(errno));
            return ERROR_FAILED;    
        }
    }

    snprintf(cPathCfg, 128, "%s%s", TOUPCAM_CFG_PATH, TOUCPAM_CFG_NAME);
    FILE* pFile = fopen(cPathCfg, "wb");
    if(NULL == pFile)
    {
        toupcam_log_f(LOG_WARNNING, "%s", strerror(errno));
        return ERROR_FAILED; 
    }
    hr = toupcam_parse_cfg(g_pstToupcamHashTable, CFG_TO_FILE_DATA);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "parse cfg failed!\n");
        return ERROR_FAILED;
    }

    hr = toupcam_cfg_save(cPathCfg, g_pstToupcamHashTable, CFG_TO_FILE_DATA);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "save cfg failed!\n");
        return ERROR_FAILED;
    }

    return ERROR_SUCCESS;   
}


unsigned int getReloadercfg(void *pvoid, int method)
{
#if 0
    if(NULL == pvoid)
    {
        toupcam_log_f(LOG_ERROR, "pvoid is null.\n");
        return ERROR_FAILED;
    }
    TOUPCAM_S *pTmpToupcam = (TOUPCAM_S *)pvoid;
    /* toupcam固有属性获取 */
    memcpy(g_pstTouPcam->m_ins, pTmpToupcam->m_ins, sizeof(pTmpToupcam->m_ins));
    /* 分辨率获取 */
    g_pstTouPcam->inWidth = pTmpToupcam->inWidth;
    g_pstTouPcam->inHeight = pTmpToupcam->inHeight;
    g_pstTouPcam->inMaxHeight = pTmpToupcam->inMaxHeight;
    g_pstTouPcam->inMaxWidth = pTmpToupcam->inMaxWidth;
    g_pstTouPcam->iSnapSize = pTmpToupcam->iSnapSize;
    /* 初始化相机数据 */
    g_pstTouPcam->m_pImageData = NULL;
    g_pstTouPcam->m_PStaticImageData = NULL;
    /* 初始化相机设置 */
    g_pstTouPcam->bNegative = pTmpToupcam->bNegative;
    g_pstTouPcam->iFrameRate = pTmpToupcam->iFrameRate;
    g_pstTouPcam->iVFlip = pTmpToupcam->iVFlip;
    g_pstTouPcam->iHFlip = pTmpToupcam->iHFlip;
    memcpy(g_pstTouPcam->stTexpo, pTmpToupcam->stTexpo, sizeof(pTmpToupcam->stTexpo));
    memcpy(g_pstTouPcam->stWhiteBlc, pTmpToupcam->stWhiteBlc, sizeof(pTmpToupcam->stWhiteBlc));
    memcpy(g_pstTouPcam->stTcolor, pTmpToupcam->stTcolor, sizeof(pTmpToupcam->stTcolor));
    memcpy(g_pstTouPcam->stHistoram, pTmpToupcam->stHistoram, sizeof(pTmpToupcam->stHistoram));
    g_pstTouPcam->m_color = pTmpToupcam->m_color;
    g_pstTouPcam->m_sample = pTmpToupcam->m_sample;
    g_pstTouPcam->m_nHz = pTmpToupcam->m_nHz;
    g_pstTouPcam->cfg = 1;

    return ERROR_SUCCESS;
#endif
}


unsigned int putReloadercfg(void *pvoid, int method)
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

    if(RELOADERBEFORE == method) /* toupcam before */
    {
        /* 设置分辨率 */
        iRSL_Num = (int)Toupcam_get_ResolutionNumber(pTmpToupcam->m_hcam);
        iRet = Toupcam_put_eSize(pTmpToupcam->m_hcam, iRSL_Num-1);  /* 预览适配最小分辨率 */
        if(FAILED(iRet))
        {
            toupcam_log_f(LOG_ERROR, "Toupcam failed.\n", __func__);
            return ERROR_FAILED;
        }

        nWidth = pTmpToupcam->inWidth;
        nHeight = pTmpToupcam->inHeight; 
        
        toupcam_dbg_f(LOG_INFO, "reloader cfg inWidth:%d", pTmpToupcam->inWidth);
        toupcam_dbg_f(LOG_INFO, "reloader cfg inHeight:%d", pTmpToupcam->inHeight);
        toupcam_dbg_f(LOG_INFO, "reloader cfg inMaxWidth:%d", pTmpToupcam->inMaxWidth);
        toupcam_dbg_f(LOG_INFO, "reloader cfg inMaxHeight:%d", pTmpToupcam->inMaxHeight);
        toupcam_dbg_f(LOG_INFO, "reloader cfg iSnapSize:%d", pTmpToupcam->iSnapSize);
        toupcam_dbg_f(LOG_INFO, "reloader cfg m_header.biSizeImage:%d", pTmpToupcam->m_header.biSizeImage);

        /* 颜色设置 */
        iRet = Set_ReColorToupcam();
        if(ERROR_FAILED == iRet)
        {
            return ERROR_FAILED;
        }

        /* 帧率设置 */
        iRet = Toupcam_put_Option(g_pstTouPcam->m_hcam, TOUPCAM_OPTION_FRAMERATE, g_pstTouPcam->iFrameRate);
        if(FAILED(iRet))
        {
            toupcam_log_f(LOG_ERROR, "set frame rate failed.\n");
            return ERROR_FAILED;
        }
        toupcam_dbg_f(LOG_INFO, "reloader cfg iFrameRate:%d", g_pstTouPcam->iFrameRate);
        
        /* 色彩模式设置 */
        toupcam_dbg_f(LOG_INFO, "reloader cfg m_color:%s", g_pstTouPcam->m_color?"color(1)":"gray(0)");

        /* 翻转设置 */
        iRet = Set_ReToupcamOrientation();
        if(ERROR_FAILED == iRet)
        {
            return ERROR_FAILED;
        }

        /* 更新手机端ip */
        sock->cliaddr[0]->sin_addr.s_addr = g_pstTouPcam->udp_ipv4;
        sock->cliaddr[1]->sin_addr.s_addr = g_pstTouPcam->udp_ipv4;
        toupcam_dbg_f(LOG_INFO, "send: sock->local:%d ip:%s.\n", sock->local, inet_ntoa(sock->cliaddr[1]->sin_addr));
        toupcam_dbg_f(LOG_INFO, "send: sock->local:%d ip:%s.\n", sock->local, inet_ntoa(sock->cliaddr[1]->sin_addr));
    }
    else if(RELOADERAFTER == method) /* toupcam after */
    {
        sleep(5);
        /* 设置Toupcam自动曝光 */
        iRet = SetReAutoExpo_Toupcam();
        if(ERROR_FAILED == iRet)
        {
            toupcam_log_f(LOG_INFO, "set auto Expo failed");
            return iRet;
        }
        
        /* 白平衡设置 */
        /*
        * 白平衡只能在相机启动后push/pull使用
        */
        iRet = Set_ReWhiteBalanceToupcam();
        if(ERROR_FAILED == iRet)
        {
            toupcam_log_f(LOG_INFO, "set white balance failed");
            return iRet;
        }
        
        /* 黑平衡设置 */
        iRet = Set_ReBlackBalanceToupcam();
        if(ERROR_FAILED == iRet)
        {
            toupcam_log_f(LOG_INFO, "set black balance failed");
            return iRet;
        }
        
        /*  初始化直方图 */
        iRet = Set_ReHistogramToupcam();
        if(ERROR_FAILED == iRet)
        {
            toupcam_log_f(LOG_INFO, "set histogram failed");
            return iRet;
        }
    }

    return ERROR_SUCCESS;
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

    /* 创建固定帧循环队列 */
    pstFixFrameRate = new FixFrameRate(g_pstTouPcam->inHeight, g_pstTouPcam->inWidth, BIT_DEPTH, FIX_FRAMERATE);

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
        if(g_pstTouPcam->cfg)
        {
            putReloadercfg(g_pstTouPcam, RELOADERAFTER);
        }
        else
        {
            SufInitialDevice(g_pstTouPcam);
        }
        toupcam_log_f(LOG_WARNNING, "press any B/b to exit\n");

        for(int i = 0; i < g_PthreadMaxNum; i++)
        {
            void** pret = NULL;
            pthread_join(g_PthreadId[i], pret);    
        }
/* cpu is high
        while(1)
        {
            if('b' == getc(stdin) || 'B' == getc(stdin))
            {
                break;
            }
            sleep(10);          
        }
*/
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
#ifdef MINIMUM_RESOLUTION
        iRet = Toupcam_put_eSize(pTmpToupcam->m_hcam, iRSL_Num-1);  /* 预览适配最小分辨率 */
        iRet += Toupcam_get_Resolution(pTmpToupcam->m_hcam, iRSL_Num-1, &iWidth, &iHeight);
#else
        iRet = Toupcam_put_eSize(pTmpToupcam->m_hcam, 0);  /* 预览适配最大分辨率 */
        iRet += Toupcam_get_Resolution(pTmpToupcam->m_hcam, 0, &iWidth, &iHeight);
#endif
        iRet += Toupcam_get_Resolution(pTmpToupcam->m_hcam, 0, &iMaxWidth, &iMaxHeight);  /* 抓拍适配最大分辨率 */

        nWidth = iWidth;
        nHeight = iHeight;        
        pTmpToupcam->inWidth = iWidth;
        pTmpToupcam->inHeight = iHeight;

        pTmpToupcam->inMaxWidth = iWidth;
        pTmpToupcam->inMaxHeight = iHeight;
		pTmpToupcam->iSnapSize = TDIBWIDTHBYTES(iWidth * BIT_DEPTH) * iHeight;
        pTmpToupcam->m_header.biSizeImage = TDIBWIDTHBYTES(iWidth * BIT_DEPTH) * iHeight;
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
    g_pstTouPcam->iFrameRate = 30;
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

    if(g_pstTouPcam->m_nHz != TOUPCAM_POWER_AC_50HZ)
    {
        toupcam_dbg_f(LOG_INFO, "power source is not AC 50Hz(%dHz)", g_pstTouPcam->m_nHz);
        g_pstTouPcam->m_nHz = TOUPCAM_POWER_AC_50HZ;
        iRet = Toupcam_put_HZ(g_pstTouPcam->m_hcam, g_pstTouPcam->m_nHz);
        if(SUCCEEDED(iRet))
        {
            toupcam_dbg_f(LOG_INFO, "current power source is AC 50Hz");
        }
    }
    else
    {
        toupcam_dbg_f(LOG_DEBUG, "power source :%s.", g_pstTouPcam->m_nHz?(g_pstTouPcam->m_nHz == 1?"50Hz":"direct-currnet"):"60Hz");
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
#if 0
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
#endif
    g_pstTouPcam = &stTouPcam;
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
    g_pstTouPcam->SaveConfigure = SaveConfigure;
    g_pstTouPcam->getReloadercfg = getReloadercfg;
    g_pstTouPcam->putReloadercfg = putReloadercfg;

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
* 重载Toupcam曝光参数
*/
static unsigned int SetReAutoExpo_Toupcam()
{
    if(NULL == g_pstTouPcam->m_hcam)
    {
        toupcam_log_f(LOG_ERROR, "m_hcam is null.\n");
        return ERROR_FAILED;
    }

    /*初始化自动曝光锁*/
    pthread_mutex_init(&g_pstTouPcam->stTexpo.mutex, NULL);
    
    pthread_mutex_lock(&g_pstTouPcam->stTexpo.mutex);

    Toupcam_put_AutoExpoEnable(g_pstTouPcam->m_hcam, g_pstTouPcam->stTexpo.bAutoExposure);

    toupcam_dbg_f(LOG_INFO, "reloader cfg stTexpo.bAutoExposure:%s\n", g_pstTouPcam->stTexpo.bAutoExposure?"enable":"disable");
    if(!g_pstTouPcam->stTexpo.bAutoExposure)
    {
        if(!g_pstTouPcam->stTexpo.bAutoAGain)
        {
            g_pstTouPcam->stTexpo.AGain = (g_pstTouPcam->stTexpo.AnMin + g_pstTouPcam->stTexpo.AnMax) / 2 ;
            Toupcam_put_ExpoAGain(g_pstTouPcam->m_hcam, g_pstTouPcam->stTexpo.AGain);
        }
    }
    else
    {
        Toupcam_put_AutoExpoTarget(g_pstTouPcam->m_hcam, g_pstTouPcam->stTexpo.AutoTarget);
        toupcam_dbg_f(LOG_INFO, "reloader cfg stTexpo.AutoTarget:%d.\n", g_pstTouPcam->stTexpo.AutoTarget); 
    }
   
    pthread_mutex_unlock(&g_pstTouPcam->stTexpo.mutex);

    toupcam_dbg_f(LOG_INFO, "reloader cfg stTexpo.Time:%uus.\n", g_pstTouPcam->stTexpo.Time);
    toupcam_dbg_f(LOG_INFO, "reloader cfg stTexpo.nMin:%uus,stTexpo.nMax:%uus,stTexpo.nDef:%uus\n", g_pstTouPcam->stTexpo.nMin, g_pstTouPcam->stTexpo.nMax, g_pstTouPcam->stTexpo.nDef);
    toupcam_dbg_f(LOG_INFO, "reloader cfg stTexpo.AGain:%d\n", g_pstTouPcam->stTexpo.AGain);
    toupcam_dbg_f(LOG_INFO, "reloader cfg stTexpo.AnMin:%u,stTexpo.AnMax:%u,stTexpo.AnDef:%u\n", g_pstTouPcam->stTexpo.AnMin, g_pstTouPcam->stTexpo.AnMax, g_pstTouPcam->stTexpo.AnDef);
    toupcam_dbg_f(LOG_INFO, "reloader cfg stTexpo.AutoMaxTime:%u,stTexpo.AutoMaxAGain:%u\n", g_pstTouPcam->stTexpo.AutoMaxTime, g_pstTouPcam->stTexpo.AutoMaxAGain);
    
    return ERROR_SUCCESS;
}




/*
* 设置toupcam黑平衡参数
*/
static unsigned int Set_BlackBalanceToupcam()
{
    if(NULL == g_pstTouPcam)
    {
        toupcam_log_f(LOG_ERROR, "g_pstToupcam is null\n");
        return ERROR_FAILED;
    }
    HRESULT hr = 0;
    unsigned short BBlcOff[3] = {0};
    PITOUPCAM_BLACKBALANCE_CALLBACK fnBBProc = NULL;

    /* 更新黑平衡 */
    fnBBProc = _PITOUPCAM_BLACKBALANCE_CALLBACK;

    pthread_mutex_init(&g_pstTouPcam->stBlackBlc.mutex, NULL);
    pthread_mutex_lock(&g_pstTouPcam->stBlackBlc.mutex);
#if 0
    /* special RGB offset */
    /*
    * 自动黑平衡设置
    */
    /* auto black balance "one push". This function must be called AFTER Toupcam_StartXXXX */
    hr = Toupcam_AbbOnePush(g_pstTouPcam->m_hcam, fnBBProc, NULL); 
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "set black balance failed(%d)\n", hr);
        g_pstTouPcam->stBlackBlc.iauto = 0;
    }
    else
    {
        g_pstTouPcam->stBlackBlc.iauto = 1;
    }
#endif

    hr = Toupcam_get_BlackBalance(g_pstTouPcam->m_hcam, g_pstTouPcam->stBlackBlc.OffVal.aSub);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "get cur value failed(%d)\n", hr);
    }
  
    /*
    g_pstTouPcam->stBlackBlc.OffVal.aSub[0] = 80;
    g_pstTouPcam->stBlackBlc.OffVal.aSub[1] = 80;
    g_pstTouPcam->stBlackBlc.OffVal.aSub[2] = 80;
    
    hr = Toupcam_put_BlackBalance(g_pstTouPcam->m_hcam, g_pstTouPcam->stBlackBlc.OffVal.aSub);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "put default value failed(%d)\n", hr);
    }
    */

    toupcam_log_f(LOG_INFO, "blackbalance R:%d", g_pstTouPcam->stBlackBlc.OffVal.aSub[0]);
    toupcam_log_f(LOG_INFO, "blackbalance G:%d", g_pstTouPcam->stBlackBlc.OffVal.aSub[1]);
    toupcam_log_f(LOG_INFO, "blackbalance B:%d", g_pstTouPcam->stBlackBlc.OffVal.aSub[2]);

    pthread_mutex_unlock(&g_pstTouPcam->stBlackBlc.mutex);

    return ERROR_SUCCESS;    
}

/*
* 设置toupcam黑平衡重载参数
*/
static unsigned int Set_ReBlackBalanceToupcam()
{
    if(NULL == g_pstTouPcam)
    {
        toupcam_log_f(LOG_ERROR, "g_pstToupcam is null\n");
        return ERROR_FAILED;
    }
    HRESULT hr = 0;
    PITOUPCAM_BLACKBALANCE_CALLBACK fnBBProc = NULL;

    /* 更新黑平衡 */
    fnBBProc = _PITOUPCAM_BLACKBALANCE_CALLBACK;

    pthread_mutex_init(&g_pstTouPcam->stBlackBlc.mutex, NULL);
    pthread_mutex_lock(&g_pstTouPcam->stBlackBlc.mutex);

    if(g_pstTouPcam->stBlackBlc.iauto)
    {
        hr = Toupcam_AbbOnePush(g_pstTouPcam->m_hcam, fnBBProc, NULL); 
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_ERROR, "set black balance failed(%d)\n", hr);
        }
    }
    else
    {
#if 0   /* cancle special class */
        /* fresh data */
        g_pstTouPcam->stBlackBlc.OffVal.aSub[0] = g_pstTouPcam->stBlackBlc.OffVal.ROff;
        g_pstTouPcam->stBlackBlc.OffVal.aSub[1] = g_pstTouPcam->stBlackBlc.OffVal.GOff;
        g_pstTouPcam->stBlackBlc.OffVal.aSub[2] = g_pstTouPcam->stBlackBlc.OffVal.BOff;

        /* special RGB offset */
        hr = Toupcam_put_BlackBalance(g_pstTouPcam->m_hcam, g_pstTouPcam->stBlackBlc.OffVal.aSub);
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_ERROR, "put default value failed(%d)\n", hr);
        }
#endif
    }

    hr = Toupcam_get_BlackBalance(g_pstTouPcam->m_hcam, g_pstTouPcam->stBlackBlc.OffVal.aSub);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "get default value failed(%d)\n", hr);
    }
    
    toupcam_dbg_f(LOG_INFO, "reloader cfg stBlackBlc.OffVal.ROff:%d", g_pstTouPcam->stBlackBlc.OffVal.ROff);
    toupcam_dbg_f(LOG_INFO, "reloader cfg stBlackBlc.OffVal.GOff:%d", g_pstTouPcam->stBlackBlc.OffVal.GOff);
    toupcam_dbg_f(LOG_INFO, "reloader cfg stBlackBlc.OffVal.BOff:%d", g_pstTouPcam->stBlackBlc.OffVal.BOff);

    pthread_mutex_unlock(&g_pstTouPcam->stBlackBlc.mutex);

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
    int aGain[3] = {0};
    PITOUPCAM_WHITEBALANCE_CALLBACK fnTTProc = NULL;

    /* 更新白平衡回调函数 */
    fnTTProc = _PITOUPCAM_WHITEBALANCE_CALLBACK;
    
    /* 初始化锁 */
    pthread_mutex_init(&g_pstTouPcam->stWhiteBlc.mutex, NULL);
    pthread_mutex_lock(&g_pstTouPcam->stWhiteBlc.mutex);

#if 0 /* not support */
    /* set auto white balance mode as Temp/Tint */
    /* auto white balance "one push". This function must be called AFTER Toupcam_StartXXXX */
    hr = Toupcam_AwbOnePush(g_pstTouPcam->m_hcam, fnTTProc, (void *)&iTTCtx);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "set white balance fnTTProc failed!\n");
        g_pstTouPcam->stWhiteBlc.iauto = 0;
        //return ERROR_FAILED;
    }
    else
    {
        g_pstTouPcam->stWhiteBlc.iauto = 1;
    }

    hr = Toupcam_get_TempTint(g_pstTouPcam->m_hcam, &g_pstTouPcam->stWhiteBlc.Temp, &g_pstTouPcam->stWhiteBlc.Tint);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "init Temp,Tint failed!\n");
        //pthread_mutex_unlock(&g_pstTouPcam->stWhiteBlc.mutex);
        //return ERROR_FAILED;
    }
    toupcam_dbg_f(LOG_DEBUG, "White balance Temp:%d, Tint:%d\n", g_pstTouPcam->stWhiteBlc.Temp, g_pstTouPcam->stWhiteBlc.Tint);
#endif
    hr = Toupcam_AwbInit(g_pstTouPcam->m_hcam, fnTTProc, NULL);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "set white balance fnTTProc failed(%d)!\n", hr);
        g_pstTouPcam->stWhiteBlc.iauto = 0;
        //return ERROR_FAILED;
    }
    else
    {
        g_pstTouPcam->stWhiteBlc.iauto = 1;
    }
    
    hr = Toupcam_get_WhiteBalanceGain(g_pstTouPcam->m_hcam, aGain);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "get white balance aGain failed(%d)!\n", hr);
        //return ERROR_FAILED;
    }
    else
    {
        g_pstTouPcam->stWhiteBlc.RGain = aGain[0];
        g_pstTouPcam->stWhiteBlc.GGain = aGain[1];
        g_pstTouPcam->stWhiteBlc.BGain = aGain[2];
    }

    toupcam_log_f(LOG_INFO, "cur white balance RGain[%d],GGain[%d],BGain[%d]\n", aGain[0], aGain[1], aGain[2]);
    pthread_mutex_unlock(&g_pstTouPcam->stWhiteBlc.mutex);

    return ERROR_SUCCESS;
}

/*
* 设置Toupcam白平衡重载参数
*/
static unsigned int Set_ReWhiteBalanceToupcam()
{
    if(NULL == g_pstTouPcam)
    {
        toupcam_log_f(LOG_ERROR, "g_pstToupcam is null\n");
        return ERROR_FAILED;
    }
    HRESULT hr = 0;
    int iTTCtx = 0;
    int aGain[3] = {0};
    PITOUPCAM_WHITEBALANCE_CALLBACK fnTTProc = NULL;

    /* 更新白平衡 */
    fnTTProc = _PITOUPCAM_WHITEBALANCE_CALLBACK;

    /* 初始化锁 */
    pthread_mutex_init(&g_pstTouPcam->stWhiteBlc.mutex, NULL);

    /* set auto white balance mode as Temp/Tint */
    /* auto white balance "one push". This function must be called AFTER Toupcam_StartXXXX */

    /*
    * 白平衡功能,不支持自动白平衡模式，支持Temp
    */
    if(g_pstTouPcam->stWhiteBlc.iauto)
    {
#if 0        
        hr = Toupcam_AwbOnePush(g_pstTouPcam->m_hcam, fnTTProc, (void *)&iTTCtx);
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_ERROR, "set white balance fnTTProc failed(%d)!\n", hr);
            //return ERROR_FAILED;
        }
#endif
        hr = Toupcam_AwbInit(g_pstTouPcam->m_hcam, fnTTProc, NULL);
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_ERROR, "reloader cfg set white balance fnTTProc failed(%d)!\n", hr);
            //return ERROR_FAILED;
        }
    }
    else
    {
#if 0
        hr = Toupcam_get_TempTint(g_pstTouPcam->m_hcam, &g_pstTouPcam->stWhiteBlc.Temp, &g_pstTouPcam->stWhiteBlc.Tint);
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_ERROR, "init Temp,Tint failed!\n");
            //pthread_mutex_unlock(&g_pstTouPcam->stWhiteBlc.mutex);
            //return ERROR_FAILED;
        }
#endif
        aGain[0] = g_pstTouPcam->stWhiteBlc.RGain;
        aGain[1] = g_pstTouPcam->stWhiteBlc.GGain;
        aGain[2] = g_pstTouPcam->stWhiteBlc.BGain;

        hr = Toupcam_put_WhiteBalanceGain(g_pstTouPcam->m_hcam, aGain);
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_ERROR, "reloader cfg put white balance aGain failed(%d)!\n", hr);
            //return ERROR_FAILED;
        }
    }

    hr = Toupcam_get_WhiteBalanceGain(g_pstTouPcam->m_hcam, aGain);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "reloader cfg get white balance aGain failed(%d)!\n", hr);
        //return ERROR_FAILED;
    }
    else
    {
        pthread_mutex_lock(&g_pstTouPcam->stWhiteBlc.mutex);
        g_pstTouPcam->stWhiteBlc.RGain = aGain[0];
        g_pstTouPcam->stWhiteBlc.GGain = aGain[1];
        g_pstTouPcam->stWhiteBlc.BGain = aGain[2];
        pthread_mutex_unlock(&g_pstTouPcam->stWhiteBlc.mutex);
    }
  
    toupcam_dbg_f(LOG_INFO, "reloader cfg stWhiteBlc.Temp:%d,stWhiteBlc.Tint:%d\n", g_pstTouPcam->stWhiteBlc.Temp, 
                            g_pstTouPcam->stWhiteBlc.Tint);
    toupcam_log_f(LOG_INFO, "reloader cfg cur white balance RGain[%d],GGain[%d],BGain[%d]\n", g_pstTouPcam->stWhiteBlc.RGain, 
                            g_pstTouPcam->stWhiteBlc.GGain, g_pstTouPcam->stWhiteBlc.BGain);

    return ERROR_SUCCESS;
}


/*
* 设置Toupcam颜色参数
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
* 设置Toupcam重载颜色参数
*/
static unsigned int Set_ReColorToupcam()
{
    if(NULL == g_pstTouPcam)
    {
        toupcam_log_f(LOG_ERROR, "g_pstToupcam is null\n");
        return ERROR_FAILED;
    }

    HRESULT hr;
    /*初始化自动曝光锁*/
    pthread_mutex_init(&g_pstTouPcam->stTcolor.mutex, NULL);
    pthread_mutex_lock(&g_pstTouPcam->stTcolor.mutex);

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
    toupcam_dbg_f(LOG_INFO, "reloader cfg stTcolor.Contrast:%d", g_pstTouPcam->stTcolor.Contrast);
  
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
    toupcam_dbg_f(LOG_INFO, "reloader cfg stTcolor.Gamma:%d.\n", g_pstTouPcam->stTcolor.Gamma);

    pthread_mutex_unlock(&g_pstTouPcam->stTcolor.mutex);
    toupcam_dbg_f(LOG_INFO, "reloader cfg stTcolor.bAutoColor:%d", g_pstTouPcam->stTcolor.bAutoColor);
    toupcam_dbg_f(LOG_INFO, "reloader cfg stTcolor.Hue:%d", g_pstTouPcam->stTcolor.Hue);
    toupcam_dbg_f(LOG_INFO, "reloader cfg stTcolor.Brightness:%d", g_pstTouPcam->stTcolor.Brightness);
    toupcam_dbg_f(LOG_INFO, "reloader cfg stTcolor.Saturation:%d", g_pstTouPcam->stTcolor.Saturation);

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
    g_pstTouPcam->stHistoram.bAutoHis = 0;
#if 1
    /* 设置直方图为自动         */
    g_pstTouPcam->stHistoram.bAutoHis = TRUE;
    hr = Toupcam_LevelRangeAuto(g_pstTouPcam->m_hcam);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "set auto histogram failed.\n");
        return ERROR_FAILED;
    }
#endif
#if 0
    g_pstTouPcam->stHistoram.aLow[0] = 0;
    g_pstTouPcam->stHistoram.aLow[1] = 0;
    g_pstTouPcam->stHistoram.aLow[2] = 0;
    g_pstTouPcam->stHistoram.aLow[3] = 104;

    g_pstTouPcam->stHistoram.aHigh[0] = 255;
    g_pstTouPcam->stHistoram.aHigh[1] = 255;
    g_pstTouPcam->stHistoram.aHigh[2] = 255;
    g_pstTouPcam->stHistoram.aHigh[3] = 135;

    hr = Toupcam_put_LevelRange(g_pstTouPcam->m_hcam, g_pstTouPcam->stHistoram.aLow, g_pstTouPcam->stHistoram.aHigh);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "updated histogram data failed.\n");
        return ERROR_FAILED;
    }
#endif
    /* 更新直方图范围 */
    hr = Toupcam_get_LevelRange(g_pstTouPcam->m_hcam, g_pstTouPcam->stHistoram.aLow, g_pstTouPcam->stHistoram.aHigh);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "updated histogram data failed(%d).\n", hr);
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
* 重载直方图数据
*/
static unsigned int Set_ReHistogramToupcam()
{
    HRESULT hr;
    /* 初始化直方图数据锁 */
    pthread_mutex_init(&g_pstTouPcam->stHistoram.mutex, NULL);

    pthread_mutex_lock(&g_pstTouPcam->stHistoram.mutex);    
    /* 设置直方图为自动         */
    //if(g_pstTouPcam->stHistoram.bAutoHis == -1) /* 直方图自动不支持 */
    if(g_pstTouPcam->stHistoram.bAutoHis)
    {
        hr = Toupcam_LevelRangeAuto(g_pstTouPcam->m_hcam);
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_ERROR, "set auto histogram failed(%d).\n", hr);
            return ERROR_FAILED;
        }
    }
    else
    {
        /* 更新直方图范围 */
        hr = Toupcam_put_LevelRange(g_pstTouPcam->m_hcam, g_pstTouPcam->stHistoram.aLow, g_pstTouPcam->stHistoram.aHigh);
        if(FAILED(hr))
        {
            toupcam_log_f(LOG_ERROR, "updated histogram data failed.\n");
            pthread_mutex_unlock(&g_pstTouPcam->stHistoram.mutex);
            return ERROR_FAILED;
        }
    }

    toupcam_dbg_f(LOG_INFO, "reloader cfg stHistoram.aLow:LR:[%d] LG:[%d] LB:[%d] LY:[%d]\n", 
                                g_pstTouPcam->stHistoram.aLow[0],
                                g_pstTouPcam->stHistoram.aLow[1],
                                g_pstTouPcam->stHistoram.aLow[2],
                                g_pstTouPcam->stHistoram.aLow[3]);
    toupcam_dbg_f(LOG_INFO, "reloader cfg stHistoram.aHigh:HR:[%d] HG:[%d] HB:[%d] HY:[%d]\n", 
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


    /* 图像倒置 */
    hr = Toupcam_put_Option(g_pstTouPcam->m_hcam, TOUPCAM_OPTION_UPSIDE_DOWN, 0);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "set frame rate failed.\n");
        return ERROR_FAILED;
    }

#ifdef ROI_IMAGE
    nWidth = ROI_INWIDTH, nHeight = ROI_INHEIGHT;
    g_pstTouPcam->inHeight = g_pstTouPcam->inMaxHeight = ROI_INHEIGHT;
    g_pstTouPcam->inWidth = g_pstTouPcam->inMaxWidth = ROI_INWIDTH;
    hr = Toupcam_put_Roi(g_hcam, 150, 250, ROI_INWIDTH, ROI_INHEIGHT);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "set roi (%dx%d) failed at(150, 250).\n", ROI_INWIDTH, ROI_INHEIGHT);
        return ERROR_FAILED;
    }
#endif

    return ERROR_SUCCESS;
}

/*
* 重载Toupcam图像方向
*/
static unsigned int Set_ReToupcamOrientation()
{
    if(NULL == g_pstTouPcam)
    {
        toupcam_log_f(LOG_ERROR, "g_pstToupcam is null\n");
        return ERROR_FAILED;
    }

    HRESULT hr;   
    toupcam_dbg_f(LOG_INFO, "reloader cfg iVFlip:%s", g_pstTouPcam->iVFlip?"negative(1)":"positive(0)");

    hr = Toupcam_put_HFlip(g_pstTouPcam->m_hcam, g_pstTouPcam->iHFlip);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "init Hflip failed!\n");
        return ERROR_FAILED;
    }
    toupcam_dbg_f(LOG_INFO, "reloader cfg iHFlip:%s", g_pstTouPcam->iHFlip?"negative(1)":"positive(0)");

    /* 图像倒置 */
    hr = Toupcam_put_Option(g_pstTouPcam->m_hcam, TOUPCAM_OPTION_UPSIDE_DOWN, 0);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "set upside_down failed.\n");
        return ERROR_FAILED;
    }

#ifdef ROI_IMAGE
    nWidth = g_pstTouPcam->inWidth;
    nHeight = g_pstTouPcam->inHeight;
    hr = Toupcam_put_Roi(g_hcam, 150, 250, g_pstTouPcam->inWidth, g_pstTouPcam->inHeight);
    if(FAILED(hr))
    {
        toupcam_log_f(LOG_ERROR, "set frame roi failed.\n");
        return ERROR_FAILED;
    }
    toupcam_dbg_f(LOG_INFO, "reloader cfg roi w(%d),h(%d)", g_pstTouPcam->inWidth, g_pstTouPcam->inHeight);
#endif

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


