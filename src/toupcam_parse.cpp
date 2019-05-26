#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "../include/common_toupcam.h"
#include "../include/toupcam_parse.h"
#include "../include/toupcam_log.h"
extern TOUPCAM_S stTouPcam;

TOUPCAM_DATA_CFG_S g_astToupcamDataCfg[] = {
    {TOUPCAM_DATACFG_INS_SN,                "ins_sn",                           stTouPcam.m_ins.sn,  STRING},
    {TOUPCAM_DATACFG_INS_FWVER,             "ins_fwver",                        stTouPcam.m_ins.fwver,  STRING},
    {TOUPCAM_DATACFG_INS_HWVER,             "ins_hwver",                        stTouPcam.m_ins.hwver,  STRING},
    {TOUPCAM_DATACFG_INS_DATE,              "ins_date",                         stTouPcam.m_ins.pdate,  STRING},
    {TOUPCAM_DATACFG_INS_REVISION,          "ins_revision",                     &stTouPcam.m_ins.pRevision,  SHORT16},
    {TOUPCAM_DATACFG_INWIDTH,               "inWidth",                          &stTouPcam.inWidth,  INTEGRATE},
    {TOUPCAM_DATACFG_INHEIGHT,              "inHeight",                         &stTouPcam.inHeight,  INTEGRATE},
    {TOUPCAM_DATACFG_INMAXWIDTH,            "inMaxWidth",                       &stTouPcam.inMaxWidth,  INTEGRATE},
    {TOUPCAM_DATACFG_INMAXHEIGHT,           "inMaxHeight",                      &stTouPcam.inMaxHeight,  INTEGRATE},
    {TOUPCAM_DATACFG_ISNAPSIZE,             "iSnapSize",                        &stTouPcam.iSnapSize,  INTEGRATE},
    {TOUPCAM_DATACFG_HEADER_BISIZE,         "Header_BiSize",                    &stTouPcam.m_header.biSize,  INTEGRATE},
    {TOUPCAM_DATACFG_HEADER_BIWIDTH,         "Header_BiWidth",                  &stTouPcam.m_header.biWidth,  INTEGRATE},
    {TOUPCAM_DATACFG_HEADER_BIHEIGHT,        "Header_BiHeight",                 &stTouPcam.m_header.biHeight,  INTEGRATE},
    {TOUPCAM_DATACFG_HEADER_BIPLANES,        "Header_BiPlanes",                 &stTouPcam.m_header.biPlanes,  SHORT16},
    {TOUPCAM_DATACFG_HEADER_BIBITCOUNT,      "Header_BiBitCount",               &stTouPcam.m_header.biBitCount,  SHORT16},
    {TOUPCAM_DATACFG_HEADER_BICOMPRESSION,   "Header_BiCompression",            &stTouPcam.m_header.biCompression,  INTEGRATE},
    {TOUPCAM_DATACFG_HEADER_BISIZEIMAGE,     "Header_BiSizeImage",              &stTouPcam.m_header.biSizeImage,  INTEGRATE},
    {TOUPCAM_DATACFG_HEADER_BIXPELSPERMETER, "Header_BiXPelsPerMeter",          &stTouPcam.m_header.biXPelsPerMeter,  INTEGRATE},
    {TOUPCAM_DATACFG_HEADER_BIYPELSPERMETER, "Header_BiYPelsPerMeter",          &stTouPcam.m_header.biYPelsPerMeter,  INTEGRATE},
    {TOUPCAM_DATACFG_HEADER_BICLRUSED,       "Header_BiClrUsed",                &stTouPcam.m_header.biClrUsed,  INTEGRATE},
    {TOUPCAM_DATACFG_HEADER_BICLRIMPORTANT,  "Header_BiClrImportant",           &stTouPcam.m_header.biClrImportant,  INTEGRATE},
    {TOUPCAM_DATACFG_BNEGATIVE,               "bNegative",                      &stTouPcam.bNegative,  INTEGRATE},
    {TOUPCAM_DATACFG_IFRAMERATE,              "iFrameRate",                     &stTouPcam.iFrameRate,  INTEGRATE},
    {TOUPCAM_DATACFG_IVFLIP,                  "iVFlip",                         &stTouPcam.iVFlip,  INTEGRATE},
    {TOUPCAM_DATACFG_IHFLIP,                  "iHFlip",                         &stTouPcam.iHFlip,  INTEGRATE},
    {TOUPCAM_DATACFG_EXPO_CHANGED,            "Expo_Changed",                   &stTouPcam.stTexpo.changed,  CHAR8},
    {TOUPCAM_DATACFG_EXPO_BAUTOEXPOSURE,      "Expo_bAutoExposure",             &stTouPcam.stTexpo.bAutoExposure,  INTEGRATE},
    {TOUPCAM_DATACFG_EXPO_BAUTOAGAIN,         "Expo_bAutoAGain",                &stTouPcam.stTexpo.bAutoAGain,  INTEGRATE},
    {TOUPCAM_DATACFG_EXPO_AUTOTARGET,         "Expo_AutoTarget",                &stTouPcam.stTexpo.AutoTarget,  SHORT16},
    {TOUPCAM_DATACFG_EXPO_AUTOMAXTIME,        "Expo_AutoMaxTime",               &stTouPcam.stTexpo.AutoMaxTime,  INTEGRATE},
    {TOUPCAM_DATACFG_EXPO_AUTOMAXAGAIN,       "Expo_AutoMaxAGain",              &stTouPcam.stTexpo.AutoMaxAGain,  SHORT16},
    {TOUPCAM_DATACFG_EXPO_TIME,               "Expo_Time",                      &stTouPcam.stTexpo.Time,  INTEGRATE},
    {TOUPCAM_DATACFG_EXPO_NMIN,               "Expo_nMin",                      &stTouPcam.stTexpo.nMin,  INTEGRATE},
    {TOUPCAM_DATACFG_EXPO_NMAX,               "Expo_nMax",                      &stTouPcam.stTexpo.nMax,  INTEGRATE},
    {TOUPCAM_DATACFG_EXPO_NDEF,               "Expo_nDef",                      &stTouPcam.stTexpo.nDef,  INTEGRATE},
    {TOUPCAM_DATACFG_EXPO_AGAIN,              "Expo_AGain",                     &stTouPcam.stTexpo.AGain,  SHORT16},
    {TOUPCAM_DATACFG_EXPO_ANMIN,              "Expo_AnMin",                     &stTouPcam.stTexpo.AnMin,  SHORT16},
    {TOUPCAM_DATACFG_EXPO_ANMAX,              "Expo_AnMax",                     &stTouPcam.stTexpo.AnMax,  SHORT16},
    {TOUPCAM_DATACFG_EXPO_ANDEF,              "Expo_AnDef",                     &stTouPcam.stTexpo.AnDef,  SHORT16},
    {TOUPCAM_DATACFG_EXPO_SCALE,              "Expo_Scale",                     &stTouPcam.stTexpo.scale,  FLOAT},
    {TOUPCAM_DATACFG_WHITEBLC_IAUTO,           "WhiteBlc_iAuto",                &stTouPcam.stWhiteBlc.iauto,   INTEGRATE},
    {TOUPCAM_DATACFG_WHITEBLC_TEMP,            "WhiteBlc_Temp",                 &stTouPcam.stWhiteBlc.Temp,   INTEGRATE},
    {TOUPCAM_DATACFG_WHITEBLC_TINT,            "WhiteBlc_Tint",                 &stTouPcam.stWhiteBlc.Tint,   INTEGRATE},
    {TOUPCAM_DATACFG_WHITEBLC_RGAIN,           "WhiteBlc_RGain",                &stTouPcam.stWhiteBlc.RGain,   INTEGRATE},
    {TOUPCAM_DATACFG_WHITEBLC_GGAIN,           "WhiteBlc_GGain",                &stTouPcam.stWhiteBlc.GGain,   INTEGRATE},
    {TOUPCAM_DATACFG_WHITEBLC_BGAIN,           "WhiteBlc_BGain",                &stTouPcam.stWhiteBlc.BGain,   INTEGRATE},
    {TOUPCAM_DATACFG_BLACKBLC_IAUTO,            "BlackBlc_iAuto",               &stTouPcam.stBlackBlc.iauto,   INTEGRATE},
    {TOUPCAM_DATACFG_BLACKBLC_ROFF,             "BlackBlc_ROff",                &stTouPcam.stBlackBlc.OffVal.ROff,   SHORT16},
    {TOUPCAM_DATACFG_BLACKBLC_GOFF,             "BlackBlc_GOff",                &stTouPcam.stBlackBlc.OffVal.GOff,   SHORT16},
    {TOUPCAM_DATACFG_BLACKBLC_BOFF,             "BlackBlc_BOff",                &stTouPcam.stBlackBlc.OffVal.BOff,   SHORT16},
    {TOUPCAM_DATACFG_COLOR_BAUTOCOLOR,          "Color_bAutoColor",             &stTouPcam.stTcolor.bAutoColor,   INTEGRATE},
    {TOUPCAM_DATACFG_COLOR_HUE,                 "Color_Hue",                    &stTouPcam.stTcolor.Hue,   INTEGRATE},
    {TOUPCAM_DATACFG_COLOR_SATURATION,          "Color_Saturation",             &stTouPcam.stTcolor.Saturation,   INTEGRATE},
    {TOUPCAM_DATACFG_COLOR_BRIGHTNESS,          "Color_Brightness",             &stTouPcam.stTcolor.Brightness,   INTEGRATE},
    {TOUPCAM_DATACFG_COLOR_CONTRAST,            "Color_Contrast",               &stTouPcam.stTcolor.Contrast,   INTEGRATE},
    {TOUPCAM_DATACFG_COLOR_GAMMA,               "Color_Gamma",                  &stTouPcam.stTcolor.Gamma,   INTEGRATE},
    {TOUPCAM_DATACFG_HISTORAM_BAUTOHIS,          "Historam_bAutoHis",           &stTouPcam.stHistoram.bAutoHis,   CHAR8},
    {TOUPCAM_DATACFG_HISTORAM_ALOWR,             "Historam_ALowR",              &stTouPcam.stHistoram.aLow[0],   SHORT16},
    {TOUPCAM_DATACFG_HISTORAM_ALOWG,             "Historam_ALowG",              &stTouPcam.stHistoram.aLow[1],   SHORT16},
    {TOUPCAM_DATACFG_HISTORAM_ALOWB,             "Historam_ALowB",              &stTouPcam.stHistoram.aLow[2],   SHORT16},
    {TOUPCAM_DATACFG_HISTORAM_ALOWY,             "Historam_ALowY",              &stTouPcam.stHistoram.aLow[3],   SHORT16},
    {TOUPCAM_DATACFG_HISTORAM_AHIGHR,            "Historam_AHighR",             &stTouPcam.stHistoram.aHigh[0],   SHORT16},
    {TOUPCAM_DATACFG_HISTORAM_AHIGHG,            "Historam_AHighG",             &stTouPcam.stHistoram.aHigh[1],   SHORT16},  
    {TOUPCAM_DATACFG_HISTORAM_AHIGHB,            "Historam_AHighB",             &stTouPcam.stHistoram.aHigh[2],   SHORT16},
    {TOUPCAM_DATACFG_HISTORAM_AHIGHY,            "Historam_AHighY",             &stTouPcam.stHistoram.aHigh[3],   SHORT16},
    {TOUPCAM_DATACFG_COLOR,                      "Color",                       &stTouPcam.m_color,   INTEGRATE},
    {TOUPCAM_DATACFG_SAMPLE,                     "Sample",                      &stTouPcam.m_sample,   INTEGRATE},
    {TOUPCAM_DATACFG_NHZ,                        "nHz",                         &stTouPcam.m_nHz,   INTEGRATE},
    {TOUPCAM_DATACFG_CFG,                        "cfg",                         &stTouPcam.cfg,   INTEGRATE},
    {TOUPCAM_DATACFG_UDPIP,                      "udpip",                       &stTouPcam.udp_ipv4,  INTEGRATE},
};

int iToupcamDataCfgSize = ARRAY_SIZE(g_astToupcamDataCfg);

int lock_file(FILE* fp, int flag)
{
    if(NULL == fp)
    {
        toupcam_log_f(LOG_ERROR, "input ptr is null");
        return ERROR_FAILED;
    }
    int fd = fileno(fp);
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    if(fcntl(fd, F_GETLK, &fl)<0)
    {
        toupcam_log_f(LOG_ERROR, "F_GETLK failed!");
        return ERROR_FAILED;
    }
    if(F_UNLCK != fl.l_type)
    {
        toupcam_log_f(LOG_ERROR, "file lock is locked");
        return ERROR_FAILED;
    }
    fl.l_type = flag;

    if(fcntl(fd, F_SETLK, &fl)<0)
    {
        toupcam_log_f(LOG_ERROR, "F_SETLK %s failed!", flag==F_WRLCK?"F_WRLCK":"F_RDLCK");
        return ERROR_FAILED;
    }

    return ERROR_SUCCESS;
}

int unlock_file(FILE* fp, int flag)
{
    if(NULL == fp)
    {
        toupcam_log_f(LOG_ERROR, "input ptr is null");
        return ERROR_FAILED;
    }
    int fd = fileno(fp);
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    if(fcntl(fd, F_GETLK, &fl)<0)
    {
        toupcam_log_f(LOG_ERROR, "F_GETLK failed!");
        return ERROR_FAILED;
    }
    fl.l_type = F_UNLCK;
    
    if(fcntl(fd, F_SETLK, &fl)<0)
    {
        toupcam_log_f(LOG_ERROR, "F_SETLK F_UNLCK failed!");
        return ERROR_FAILED;
    }

    return ERROR_SUCCESS;
}

int toupcam_cfg_read(const char *pname, void *ptable, int method)
{
    if(NULL == pname || NULL == ptable)
    {
        toupcam_log_f(LOG_ERROR, "pname is %s, ptable is %s", pname?"Yes":"NULL", ptable?"Yes":"NULL");
        return ERROR_FAILED;
    }
    char cIcon[12] = {0};
    char key[128] = {0};
    char val[128] = {0};
    int i=0,j=0;
    int iRet = ERROR_SUCCESS;

    /* init Hash Table */
    HashTable *pstToupcamHashTable = (HashTable *)ptable;

    FILE *pFile = fopen(pname, "rb");
    if(NULL == pFile)
    {
        toupcam_log_f(LOG_ERROR, "fopen:%s", strerror(errno));
        return ERROR_FAILED;
    }

    if(ERROR_FAILED == lock_file(pFile, F_RDLCK))
    {
        return ERROR_FAILED;
    }

    /* read icon 'toupcamcfg:' */
    iRet = fread(cIcon, 1, 11, pFile);
    if(strcmp(cIcon, "toupcamcfg:"))
    {
        toupcam_log_f(LOG_ERROR, "parse cfg file error: not found icon(%s)", cIcon);
        fclose(pFile);
        return ERROR_FAILED;
    }

    while(!feof(pFile))
    {
        unsigned char c = fgetc(pFile);
        if(c == ':')
        {
            j = 255;
            memset(val, 0, sizeof(val)/sizeof(*val));
            continue;
        }
        else if(0 == c)
        {
            val[j] = c;
            DIRECTORY_S *pnode = createnode(key, val);
            toupcam_dbg_f(LOG_WARNNING, "read node:%s,val:%s", pnode->pcname, pnode->pcval);
            if(findnnode(pstToupcamHashTable, pnode))
            {
                updatenode(pstToupcamHashTable, pnode);
                destorynode(pnode);
            }
            else
            {
                insertnode(pstToupcamHashTable, pnode);                
            }
            i = 0;
            j = 0;
            memset(key, 0, sizeof(key)/sizeof(*key));
            memset(val, 0, sizeof(val)/sizeof(*val));
            continue;
        }

        if(0 == j)
        {
            key[i++] = c;
        }
        else
        {
            if(255 == j)
            {
                j = 0;
            }
            val[j++] = c;
        }
    }

    if(ERROR_FAILED == unlock_file(pFile, F_UNLCK))
    {
        return ERROR_FAILED;
    }

    fclose(pFile);
    return ERROR_SUCCESS;
}

int toupcam_cfg_save(const char *pname, void *ptable, int method)
{
    if(NULL == pname || NULL == ptable)
    {
        toupcam_log_f(LOG_ERROR, "pname is %s, ptable is %s", pname?"Yes":"NULL", ptable?"Yes":"NULL");
        return ERROR_FAILED;
    }

    /* init Hash Table */
    int i = 0;
    int iRet = 0;
    char cbreak = 0;
    HashTable *pstToupcamHashTable = (HashTable *)ptable;
    DIRECTORY_S *pDirection = NULL;

    FILE *pFile = fopen(pname, "wb");
    if(NULL == pFile)
    {
        toupcam_log_f(LOG_ERROR, "fopen:%s", strerror(errno));
        return ERROR_FAILED;
    }

    if(ERROR_FAILED == lock_file(pFile, F_WRLCK))
    {
        return ERROR_FAILED;
    }

    iRet = fwrite("toupcamcfg:", 1, 11, pFile);
    if(11 != iRet)
    {
        toupcam_log_f(LOG_ERROR, "fwrite:%s(%d)", strerror(errno), iRet);
        fclose(pFile);
        return ERROR_FAILED;
    }

    for (i = 0; i < HASHTABLESIZE; ++i)
    {
        if(pstToupcamHashTable->Table[i])
        {
            pDirection = pstToupcamHashTable->Table[i];
            while(1)
            {
                if(pDirection)
                {
                    /* key */
                    iRet = fwrite(pDirection->pcname, 1, strlen(pDirection->pcname), pFile);
                    toupcam_dbg_f(LOG_INFO, "name: %s", pDirection->pcname);
                    if(strlen(pDirection->pcname) != iRet)
                    {
                        toupcam_log_f(LOG_ERROR, "fwrite key:%s", strerror(errno));
                        goto exit0_;
                    }

                    /* : */
                    iRet = fwrite(":", 1, 1, pFile);
                    if(1 != iRet)
                    {
                        toupcam_log_f(LOG_ERROR, "fwrite ':':%s", strerror(errno));
                        goto exit0_;
                    }

                    /* val */
                    iRet = fwrite(pDirection->pcval, 1, strlen(pDirection->pcval), pFile);
                    toupcam_dbg_f(LOG_INFO, "val: %s", pDirection->pcval);
                    if(strlen(pDirection->pcval) != iRet)
                    {
                        toupcam_log_f(LOG_ERROR, "fwrite val:%s", strerror(errno));
                        goto exit0_;
                    }                    

                    iRet = fwrite(&cbreak, 1, 1, pFile);
                    if(1 != iRet)
                    {
                        toupcam_log_f(LOG_ERROR, "fwrite 0:%s", strerror(errno));
                        goto exit0_;
                    }
                }
                else
                {
                    break;
                }
                pDirection = pDirection->pdirectory;
            }
        }
    }

    if(ERROR_FAILED == lock_file(pFile, F_UNLCK))
    {
        return ERROR_FAILED;
    }

    fclose(pFile);
    return ERROR_SUCCESS;

exit0_:
    if(ERROR_FAILED == lock_file(pFile, F_UNLCK))
    {
        return ERROR_FAILED;
    }

    fclose(pFile);
    system("mv /var/opt/toupcam_cfg/toupcam.cfg /var/opt/toupcam_cfg/strace");
    return ERROR_SUCCESS;
}

int toupcam_parse_cfg(void *ptable, int method)
{
    if(NULL == ptable)
    {
        toupcam_log_f(LOG_ERROR, "ptable is %s", ptable?"Yes":"NULL");
        return ERROR_FAILED;
    }

    HASHTABLE_S *psttable = (HASHTABLE_S *)ptable;
    char cStr[128] = {0};
    DIRECTORY_S *pdirectory = NULL;
    DIRECTORY_S *pnode = NULL;
    int integrate = 0;
    long long llong = 0;
    int i = 0;

    for(i=0; i<iToupcamDataCfgSize; i++)
    {
        if(STRING == g_astToupcamDataCfg[i].attr)
        {
            pnode = createnode(g_astToupcamDataCfg[i].pname, (char *)g_astToupcamDataCfg[i].pvoid);
            if(NULL != pnode)
            {
                if(CFG_TO_FILE_DATA == method) /* toupcam data -> table */
                {
                    if(findnnode(psttable, pnode))
                    {
                        updatenode(psttable, pnode);
                        destorynode(pnode);
                    }
                    else
                    {
                        insertnode(psttable, pnode);
                    }
                }
                else if(CFG_TO_NVR_DATA == method) /* table -> toupcam data */
                {
                    pdirectory = findnnode(psttable, pnode);
                    if(NULL != pdirectory)
                    {
                        char *cpc = (char *)g_astToupcamDataCfg[i].pvoid;
                        int ilen = strlen(cpc);
                        memset((char *)g_astToupcamDataCfg[i].pvoid, 0, ilen);
                        strncpy((char *)g_astToupcamDataCfg[i].pvoid, pdirectory->pcval, ilen);
                    }
                }
            }
            else
            {
                toupcam_log_f(LOG_WARNNING, "create node %s:%s failed!", g_astToupcamDataCfg[i].pname, (char *)g_astToupcamDataCfg[i].pvoid);
            }
            toupcam_dbg_f(LOG_INFO, "node %s:%s", g_astToupcamDataCfg[i].pname, (char *)g_astToupcamDataCfg[i].pvoid);
        }
        else
        {
            memset(cStr, 0, ARRAY_SIZE(cStr));
            if(INTEGRATE == g_astToupcamDataCfg[i].attr)
            {
                snprintf(cStr, ARRAY_SIZE(cStr), "%d", *((int *)g_astToupcamDataCfg[i].pvoid));
            }
            else if(CHAR8 == g_astToupcamDataCfg[i].attr)
            {
                snprintf(cStr, ARRAY_SIZE(cStr), "%c", *((char *)g_astToupcamDataCfg[i].pvoid));
            }
            else if(SHORT16 == g_astToupcamDataCfg[i].attr)
            {
                snprintf(cStr, ARRAY_SIZE(cStr), "%d", *((short *)g_astToupcamDataCfg[i].pvoid));
            }
            else if(LONGLONG64 == g_astToupcamDataCfg[i].attr)
            {
                snprintf(cStr, ARRAY_SIZE(cStr), "%ld", *((long long *)g_astToupcamDataCfg[i].pvoid));
            }
            else if(FLOAT == g_astToupcamDataCfg[i].attr)
            {
                snprintf(cStr, ARRAY_SIZE(cStr), "%f", *((float *)g_astToupcamDataCfg[i].pvoid));
            }
            else if(DFLOAT == g_astToupcamDataCfg[i].attr)
            {
                snprintf(cStr, ARRAY_SIZE(cStr), "%lf", *((double *)g_astToupcamDataCfg[i].pvoid));
            }

            pnode = createnode(g_astToupcamDataCfg[i].pname, cStr);
            if(NULL != pnode)
            {
                if(CFG_TO_FILE_DATA == method) /* toupcam -> table */
                {
                    if(findnnode(psttable, pnode))
                    {
                        updatenode(psttable, pnode);
                        destorynode(pnode);
                    }
                    else
                    {
                        insertnode(psttable, pnode);
                    }
                }
                else if(CFG_TO_NVR_DATA == method) /* table -> toupcam */
                {
                    pdirectory = findnnode(psttable, pnode);
                    if(NULL != pdirectory)
                    {
                        if(INTEGRATE == g_astToupcamDataCfg[i].attr)
                        {
                            *((int *)g_astToupcamDataCfg[i].pvoid) = atoi(pdirectory->pcval);
                            toupcam_dbg_f(LOG_INFO, "node %s:%d", g_astToupcamDataCfg[i].pname, *((int *)g_astToupcamDataCfg[i].pvoid));
                        }
                        else if(CHAR8 == g_astToupcamDataCfg[i].attr)
                        {
                            *((char *)g_astToupcamDataCfg[i].pvoid) = atoi(pdirectory->pcval);
                            toupcam_dbg_f(LOG_INFO, "node %s:%d", g_astToupcamDataCfg[i].pname, *((char *)g_astToupcamDataCfg[i].pvoid));
                        }
                        else if(SHORT16 == g_astToupcamDataCfg[i].attr)
                        {
                            *((short *)g_astToupcamDataCfg[i].pvoid) = atoi(pdirectory->pcval);
                            toupcam_dbg_f(LOG_INFO, "node %s:%d", g_astToupcamDataCfg[i].pname, *((short *)g_astToupcamDataCfg[i].pvoid));
                        }
                        else if(LONGLONG64 == g_astToupcamDataCfg[i].attr)
                        {
                            *((long long *)g_astToupcamDataCfg[i].pvoid) = atoll(pdirectory->pcval);
                            toupcam_dbg_f(LOG_INFO, "node %s:%ld", g_astToupcamDataCfg[i].pname, *((long long *)g_astToupcamDataCfg[i].pvoid));
                        }
                        else if(FLOAT == g_astToupcamDataCfg[i].attr)
                        {
                            *((float *)g_astToupcamDataCfg[i].pvoid) = atof(pdirectory->pcval);
                            toupcam_dbg_f(LOG_INFO, "node %s:%f", g_astToupcamDataCfg[i].pname, *((float *)g_astToupcamDataCfg[i].pvoid));
                        }
                        else if(DFLOAT == g_astToupcamDataCfg[i].attr)
                        {
                            *((double *)g_astToupcamDataCfg[i].pvoid) = atof(pdirectory->pcval);
                            toupcam_dbg_f(LOG_INFO, "node %s:%lf", g_astToupcamDataCfg[i].pname, *((double *)g_astToupcamDataCfg[i].pvoid));
                        }
                    }
                }
            }
            else
            {
                toupcam_log_f(LOG_WARNNING, "create node %s:%s failed!", g_astToupcamDataCfg[i].pname, cStr);
            }
        }
    }

    /* cfg enable */
    stTouPcam.cfg = 1;

    return ERROR_SUCCESS;
}


