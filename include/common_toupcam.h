#ifndef _COMMON_TOUPCAM_H_
#define _COMMON_TOUPCAM_H_
#include "toupcam.h"

#define ARRAY_SIZE(a) sizeof(a)/sizeof(a[0])

#define END_BUFF_SIZE         (0)
#define INVAILD_BUFF_SIZE     (1)
#define TOUPCAM_COMMON_RESPON_HEADER_SIZE (18)
#define IPV4
#define BIGLITTLESWAP32(A) ((A&0xff)<<24 | (A&0xff00)<<8 | (A&0xff0000)>>8 | (A&0xff000000)>>24)

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
    /* wifi cfg */
    CMD_SETUDPADDR,         /* UDP 设置客户端地址 */
};

enum TOUPCAM_CC_CODE_E{
    ERROR_SUCCESS,
    ERROR_FAILED,
    ERROR_WIFI_ADDR_EXIST = 2,
    ERROR_WIFI_ADDR_HOLD = 3,
    NOTSUPPORT,
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

typedef struct Toupcam_common_respon
{
    TOUPCAM_COMMON_HEADER_S com;
    char cc;
    char *pdata;
}__attribute__((packed))TOUPCAM_COMMON_RESPON_S;

typedef struct Toupcam_common_reques
{
    TOUPCAM_COMMON_HEADER_S com;
    union Data {
        unsigned int ipv4;
        unsigned long long ipv6;
        char resever[128];
    }data;
}__attribute__((packed))TOUPCAM_COMMON_REQUES_S;

typedef struct Toupcam_ExpoTime
{

}TOUPCAM_EXPOTIME_S;

typedef struct Texpo
{
    /* Toupcam相机曝光参数 */
    char changed;                   /* 描述曝光时间发生变化的标志 */
    int bAutoExposure;              /* 设置自动曝光，True        or False */
    unsigned short AutoTarget;      /* 自动曝光目标值 */
    unsigned AutoMaxTime;           /* 设置自动曝光的最大时间 */
    unsigned short AutoMaxAGain;    /* 设置自动曝光的最大增益            */
    unsigned Time;                  /* 设置曝光时间，单位为微秒 */
    unsigned nMin;                  /* 曝光的最小时间 */
    unsigned nMax;                  /* 曝光的最大时间 */
    unsigned nDef;                  /* 曝光的默认时间 */
    unsigned short AGain;           /* 模拟增益，百分比，如200表示增益200% */
    unsigned short AnMin;           /* 模拟增益的最小时间 */
    unsigned short AnMax;            /* 模拟增益的最大时间 */
    unsigned short AnDef;            /* 模拟增益的默认时间 */
}TEXPO_S;



typedef struct Toupcam
{
    int inWidth;    /* 当前图片的有效宽度 */
    int inHeight;   /* 当前图片的有效高度 */
    int inMaxWidth;    /* 当前图片的支持最大有效宽度 */
    int inMaxHeight;   /* 当前图片的支持最大有效高度 */
	int iSnapSize;
	HToupCam m_hcam; /* 当前Toupcam的有效句柄 */
	void*	m_pImageData; /* 当前Toupcam的数据Buffer */
    void*   m_PStaticImageData;  /* 快速抓拍使用的数据Buffer */
	BITMAPINFOHEADER m_header;
    int bNegative;               /* 照片是否反转 */
    TEXPO_S stTexpo;

    /* Toupcam wifi cmd */
    struct Toupcam_wifi{
        TOUPCAM_COMMON_HEADER_S mCommon;
        
        TOUPCAM_EXPOTIME_S mExpoTime;

        unsigned int cmd_id;
        void *private_data;
#if separate
        /* tcp request */
        union{
            struct ExpoTime{
                unsigned int i;
            };
        
        }ToupcamReq_U;
        
        /* tcp respon */
        union{
            int cc;            
        }ToupcamRes_U;  

        void *private_data;
#endif
    };

    
  

    /* opreation */
    unsigned int (*PreInitialDevice)(void *);
    unsigned int (*OpenDevice)(void *);
    unsigned int (*StartDevice)(void);
    unsigned int (*ConfigDevice)(void);
    unsigned int (*CloseDevice)(void);

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

extern struct sockets *sock; /* udp potr:5004 */
extern struct sockets *sock1; /* tcp potr:5005 */

extern unsigned int init_Toupcam(void);
extern unsigned int SetupToupcam(TOUPCAM_S* pToupcam);
extern void __stdcall EventCallback(unsigned nEvent, void* pCallbackCtx);
extern void* getToupcam(void);


#endif
