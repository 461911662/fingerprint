/*
 * Error.c
 *
 *  Created on: Jun 1, 2013
 *      Author: Administrator
 */
#include <stdio.h> 
#include "../include/Error.h"
#include "../include/toupcam.h"
#include "../include/common_toupcam.h"

ERRORINFO_S g_astERRINFO[] = {
    {TOUPCAM_EVENT_EXPOSURE,        NULL,                       "exposure time changed"                                                     },
    {TOUPCAM_EVENT_TEMPTINT,        NULL,                       "white balance changed, Temp/Tint mode"                                     },
    {TOUPCAM_EVENT_IMAGE,           NULL,                       "live image arrived, use Toupcam_PullImage to get this image"               },
    {TOUPCAM_EVENT_STILLIMAGE,      NULL,                       "snap (still) frame arrived, use Toupcam_PullStillImage to get this frame"  },
    {TOUPCAM_EVENT_WBGAIN,          NULL,                       "white balance changed, RGB Gain mode"                                      },
    {TOUPCAM_EVENT_TRIGGERFAIL,     NULL,                       "trigger failed"                                                            },
    {TOUPCAM_EVENT_BLACK,           NULL,                       "black balance changed"                                                     },
    {TOUPCAM_EVENT_FFC,             NULL,                       "flat field correction status changed"                                      },
    {TOUPCAM_EVENT_DFC,             NULL,                       "dark field correction status changed"                                      },
    {TOUPCAM_EVENT_ERROR,           "generic error",            "generic error"                                                             },
    {TOUPCAM_EVENT_DISCONNECTED,    "camera disconnected",      "camera disconnected"                                                       },
    {TOUPCAM_EVENT_TIMEOUT,         NULL,                       "timeout error"                                                             },
    {TOUPCAM_EVENT_FACTORY,         NULL,                       "restore factory settings"                                                  },
};
unsigned int g_astERRINFOSIZE = ARRAY_SIZE(g_astERRINFO);

char *toupcamerrinfo(unsigned int uiCallbackContext)
{
    int i;
    for(i=0; i<g_astERRINFOSIZE; i++)
    {
        if(uiCallbackContext == g_astERRINFO[i].errorid)
        {
#if 0
            if(NULL == g_astERRINFO[i].info)
                return g_astERRINFO[i].definfo;
            else
#endif
                printf("%s\n", g_astERRINFO[i].info);
                return g_astERRINFO[i].info;
        }
    }
    return NULL;
}

int debug(char *fmt, ...) {
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return r;
}
 
int warn(char *fmt, ...) {
    int r;
    va_list ap;
    va_start(ap, fmt);
    r = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return r;
}
int fail(char *fmt, ...) {
    int r;
    va_list ap;
    va_start(ap, fmt);
    r = vfprintf(stderr, fmt, ap);
    exit(1);
    /* notreached */
    va_end(ap);
    return r;
}