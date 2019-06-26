#ifndef _COMMON_TOUPCAM_H_
#define _COMMON_TOUPCAM_H_
#include <sys/prctl.h>
#include "toupcam.h"
#include "mpp_encode_data.h"
#include "rockchip/mpp_err.h"
#include "toupcam_log.h"

#define REQUES_SIZE  (128)
#define ARRAY_SIZE(a) sizeof(a)/sizeof(a[0])

#undef SOFT_ENCODE_H264
#define MPP_FRAME_MAXNUM 1000000000000
#define ROI_IMAGE
#undef TOUPCAM_RELEASE
#undef PICTURE_ARITHMETIC

#define MINIMUM_RESOLUTION

#define FIX_480P

#if defined(FIX_480P)
#define ROI_INWIDTH  720  /* (x,y)=(150,250) */
#define ROI_INHEIGHT 480
#elif defined(FIX_720P)
#define ROI_INWIDTH  1280
#define ROI_INHEIGHT 720
#elif defined(FIX_1080P)
#define ROI_INWIDTH  1920
#define ROI_INHEIGHT 1080
#endif

#define BIT_DEPTH BIT_DEPTH8
#define BIT_DEPTH8 8
#define FIX_FRAMERATE 24

#define TOUPCAM_POWER_AC_60HZ       0
#define TOUPCAM_POWER_AC_50HZ       1
#define TOUPCAM_POWER_DC            2

#define RELOADERBEFORE   0
#define RELOADERAFTER    1

#define TOUPCAM_CFG_PATH  "/var/opt/toupcam_cfg"
#define TOUCPAM_CFG_NAME  "/toupcam.cfg"

#define END_BUFF_SIZE         (0)
#define INVAILD_BUFF_SIZE     (1)
#define COMMON_BUFF_SIZE      (5)
#define TOUPCAM_COMMON_RESPON_HEADER_SIZE (18)
#define IPV4
#define BIGLITTLESWAP32(A) ((A&0xff)<<24 | (A&0xff00)<<8 | (A&0xff0000)>>8 | (A&0xff000000)>>24)
#define BIGLITTLESWAP16(A) ((A&0xff)<<8 | (A&0xff00)>>8)

enum TOUPCAM_CMD_E{
    /* toupcam cfg */
    CMD_FIRST,
    CMD_SNAPPICTURE,        /* 拍照 */
    CMD_BLACKRORATOR,       /* 黑白反转 */          
    CMD_ZONEEXPO,           /* 区域曝光 */
    CMD_BRIGHTNESSTYPE,     /* 亮度方式 */
    CMD_BRIGHTNESS,         /* 亮度调节 */
    CMD_EXPOTYPE,           /* 曝光方式 */
    CMD_EXPO,               /* 曝光调节 */
    CMD_CONTRASTTYPE,       /* 对比度方式 */
    CMD_CONTRAST,           /* 对比度调节 */
    CMD_HISTOGRAMTYPE,      /* 直方图方式 */
    CMD_HISTOGRAM,          /* 直方图调节 */
    CMD_TOTALDATA,          /* 获取服务器当前配置参数 */
    CMD_DATATSM,            /* TCP UDP直传 */
    CMD_RESETSERVER,        /* 重启服务器 */
    /* wifi cfg */
    CMD_SETUDPADDR = 128,         /* UDP 设置客户端地址 */
};

enum TOUPCAM_CC_CODE_E{
    ERROR_SUCCESS,
    ERROR_FAILED,
    ERROR_WIFI_ADDR_EXIST = 2,
    ERROR_WIFI_ADDR_HOLD = 3,
    ERROR_TBRIGHTNESS_MANU = 4,
    NOTSUPPORT=255,
};

enum COMMON_CMD_E{
    COMCMD_FIRST,
    COMCMD_WIFICFG,
    COMCMD_TOUPCAMCFG,
};

typedef struct Toupcam_common_header
{
    char proto[5];
    unsigned int size[2];
    char type;
    char cmd;
    unsigned short subcmd;
}__attribute__((packed))TOUPCAM_COMMON_HEADER_S;

typedef struct{                
    int left;
    int top;
    int right;
    int bottom;
}__attribute__((packed))EXPO_RECT_S;

typedef struct  Historam_data
{
    unsigned short aLow[4];     /* R,G,B和灰度(Y) */
    unsigned short aHigh[4];    /* R,G,B和灰度(Y) */
}__attribute__((packed))HISTORAM_DATA_S;

typedef struct Total_Data{
    unsigned char brightnesstype;
    unsigned short brightness;
    unsigned char expotype;
    unsigned short expo;
    unsigned char contrasttype;
    unsigned int contrast;
    unsigned char historamtype;
    HISTORAM_DATA_S historam;
}__attribute__((packed))TOTAL_DATA_S;

typedef struct Toupcam_common_respon
{
    TOUPCAM_COMMON_HEADER_S com;
    char cc;
    union Data{
        unsigned char brightnesstype;
        unsigned short brightness;
        unsigned char expotype;
        unsigned short expo;
        unsigned char contrasttype;
        unsigned int contrast;
        RECT expozone;
        unsigned char historamtype;
        HISTORAM_DATA_S historam;
        TOTAL_DATA_S totaldata;
        char reserve[2048];
    }data;
}__attribute__((packed))TOUPCAM_COMMON_RESPON_S;

typedef struct Toupcam_common_reques
{
    TOUPCAM_COMMON_HEADER_S com;
    union Data {
        unsigned int ipv4;
        unsigned long long ipv6;
        unsigned char brightnesstype;   /* 0x00:手动   0x01:自动   */
        unsigned short brightness;                 /* 亮度值 */
        unsigned char expotype;        /* 0x00:手动   0x01:自动 */
        unsigned short expo;                       /* 曝光值 */
        unsigned char contrasttype;    /* 0x00:手动 0x01:自动     */
        RECT expozone;            /* 区域曝光参数 */
        int contrast;                              /* 对比度 */
        unsigned char historamtype;    /* 0x00:手动 0x01:自动     */
        HISTORAM_DATA_S historam;           /* 直方图数据 */
        char reserve[REQUES_SIZE];
    }data;
}__attribute__((packed))TOUPCAM_COMMON_REQUES_S;

typedef struct Texpo
{
    /* Toupcam相机曝光参数 */
    char changed;                   /* 描述曝光时间发生变化的标志 */
    int bAutoExposure;              /* 设置自动曝光，True        or False */
    int bAutoAGain;                 /* 设置自动曝光增益 */
    unsigned short AutoTarget;      /* 自动曝光目标值 */
    unsigned AutoMaxTime;           /* 设置自动曝光的最大时间 */
    unsigned short AutoMaxAGain;    /* 设置自动曝光的最大增益            */
    unsigned Time;                  /* 设置曝光时间，单位为微秒 */
    unsigned nMin;                  /* 曝光的最小时间 */
    unsigned nMax;                  /* 曝光的最大时间 */
    unsigned nDef;                  /* 曝光的默认时间 */
    unsigned short AGain;           /* 模拟增益，百分比，如200表示增益200% */
    unsigned short AnMin;           /* 模拟增益的最小值 */
    unsigned short AnMax;           /* 模拟增益的最大值 */
    unsigned short AnDef;           /* 模拟增益的默认值 */
    float scale;                    /* 最小刻度值 */
    pthread_mutex_t mutex;          /* 保护曝光AutoTarget,亮度AGain */   
}TEXPO_S;

typedef struct Tcolor
{
    int bAutoColor;              /* 设置自动调色，True        or False */
    int Hue;                        /* 色度 */
    int Saturation;                 /* 饱和度 */
    int Brightness;                 /* 亮度 */
    int Contrast;                   /* 对比度 */
    int Gamma;                      /* 伽玛 */
    pthread_mutex_t mutex;          /* 设置相机保护锁 */  
}TCOLOR_S;

typedef struct TWhiteBalance{
    int iauto;                      /* 白平衡自动使能 */
    int Temp;                       /* 白平衡色温 */
    int Tint;                       /* 白平衡微调 */
    int RGain;                      /* 白平衡Red    Gain */
    int GGain;                      /* 白平衡Green    Gain */
    int BGain;                      /* 白平衡Blue    Gain */
    pthread_mutex_t mutex;          /* 设置白平衡保护锁 */
}TWHITEBALANCE_S;

typedef struct BlackBalanace{
    int iauto;                      /* 黑平衡自动使能 */
    union{
        unsigned short ROff;        /* 黑平衡Red 偏移 */
        unsigned short GOff;        /* 黑平衡Green 偏移 */
        unsigned short BOff;        /* 黑平衡Blue 偏移 */
        unsigned short aSub[3];
    }OffVal;
    pthread_mutex_t mutex;          /* 设置黑平衡保护锁 */
}BLACKBALANCE_S;

typedef struct  Historam
{
    unsigned char bAutoHis;               /* 设置直方图自动手动模式 */
    unsigned short aLow[4];     /* R,G,B和灰度(Y) */
    unsigned short aHigh[4];    /* R,G,B和灰度(Y) */
    float aHistY[256];
    float aHistR[256];
    float aHistG[256];
    float aHistB[256];
    pthread_mutex_t mutex;          /* 设置直方图数据保护锁 */
}HISTORAM_S;

typedef struct Toupcam_instance{
    char sn[32];
    char fwver[16];
    char hwver[16];
    char pdate[10];
    unsigned short pRevision;
}TOUPCAM_INS_S;

typedef struct Toupcam
{
    TOUPCAM_INS_S m_ins; /* Toupcam实例 */
    int iconnect;   /* 一个手机连接 0:未连接 1:连接 */
    int inWidth;    /* 当前图片的有效宽度 */
    int inHeight;   /* 当前图片的有效高度 */
    int inMaxWidth;    /* 当前图片的支持最大有效宽度 */
    int inMaxHeight;   /* 当前图片的支持最大有效高度 */
	int iSnapSize;
	HToupCam m_hcam; /* 当前Toupcam的有效句柄 */
	void*	m_pImageData; /* 当前Toupcam的数据Buffer */
    void*   m_PStaticImageData;  /* 快速抓拍使用的数据Buffer */
	BITMAPINFOHEADER m_header;
    int bNegative;                      /* 图像是否反转 */
    int iFrameRate;                     /* 相机的帧率 */
    int iVFlip;                         /* 垂直翻转 */
    int iHFlip;                         /* 水平翻转 */
    TEXPO_S stTexpo;                    /* 相机曝光参数设置 */
    TWHITEBALANCE_S stWhiteBlc;     /* 相机白平衡参数 */
    BLACKBALANCE_S stBlackBlc;      /* 相机黑平衡参数 */
    TCOLOR_S stTcolor;                  /* 相机颜色参数设置 */
    HISTORAM_S stHistoram;              /* 相机直方图参数 */

    int m_color;                    /* 0:gray 1:color */
    int m_sample;                   /* skip or bin */
    int m_nHz;                      /* 交流50Hz,交流60Hz,直流 */

    int cfg;                        /* cfg enable:disable */

    unsigned int udp_ipv4;          /* mobile ip */
#if 0    
    union mode_color{                   /* 色彩模式 */   
        struct mode{
            unsigned char resever:6;
            unsigned char color:1;     /* 多彩模式 */
            unsigned char gray:1;      /* 黑白模式 */
        }stMode;
        unsigned char ucMode;
    }m_color;

    union mode_sample{           /* 采样模式 */
        struct mode{
            unsigned char resever:6;
            unsigned char bin:1;       /* 邻域采样 */ 
            unsigned char skip:1;      /* 抽样提取 */
        }stMode;
        unsigned char ucMode;
    }m_sample;
        
    union mode_powersource{           /* 电源模式 */
        struct mode{
            unsigned int resever:29;
            unsigned int ac50:1;        /* 交流50Hz */    
            unsigned int ac60:1;        /* 交流60Hz */
            unsigned int dc:1;          /* 直流 */
        }stMode;
        unsigned int ucMode;
    }m_powersource;
#endif

    /* opreation */
    unsigned int (*OpenDevice)();
    unsigned int (*PreInitialDevice)(void *);
    unsigned int (*ConfigDevice)(void *);
    unsigned int (*StartDevice)(void *);
    unsigned int (*CloseDevice)(void *);
    unsigned int (*SufInitialDevice)(void*);
    unsigned int (*SaveConfigure)(void*);
    unsigned int (*getReloadercfg)(void*, int);
    unsigned int (*putReloadercfg)(void*, int);

    /* event */
    void (*OnEventError)(void);
    void (*OnEventDisconnected)(void);
    void (*OnEventImage)(void);
    void (*OnEventExpo)(void);
    void (*OnEventTempTint)(void);
    void (*OnEventStillImage)(void);
}TOUPCAM_S;

typedef struct Com_Toupcam_Entry
{
    unsigned int cmd_id;
    unsigned (*handle)(TOUPCAM_S *pstToupcam, unsigned int cmd_id, void *pprivate);
}COM_TOUPCAM_ENTRY_S;

typedef struct Com_Entry
{
    char cmd_id;
    int (*handle)(int fd, void *pdata, unsigned short usSize);
}COM_ENTRY_S;

extern COM_ENTRY_S g_Comm_Entry[];
extern unsigned int g_Comm_Entry_Size;

extern TOUPCAM_S* g_pstTouPcam;
extern HToupCam g_hcam;
extern int nWidth, nHeight;
extern void* g_pImageData;
extern void* g_pStaticImageData;
extern int g_pStaticImageDataFlag;
extern unsigned int iEndianness;
extern pthread_mutex_t g_PthreadMutexMonitor;

extern pthread_t g_PthreadId[];
extern unsigned int g_PthreadMaxNum;

extern struct sockets *sock; /* udp potr:5004 */
extern struct sockets *sock1; /* tcp potr:5005 */

extern unsigned int init_Toupcam(void *pdata);
extern unsigned int SetupToupcam(TOUPCAM_S* pToupcam);
extern void __stdcall EventCallback(unsigned nEvent, void* pCallbackCtx);
extern void* getToupcam(void);
extern void init_mpp();


#endif
