#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "../include/common_toupcam.h"
#include "../include/toupcam_parse.h"
#include "../include/toupcam_data.h"
#include "../include/toupcam_log.h"

/*
* 创建
*/
HASHTABLE_S* createHash()
{
    HASHTABLE_S *pstHashTable = (HASHTABLE_S *)malloc(sizeof(HASHTABLE_S));
    if(NULL == pstHashTable)
    {
        toupcam_log_f(LOG_ERROR, "%s", strerror(errno));
        return NULL;
    }
    memset(pstHashTable, 0, sizeof(HASHTABLE_S));

    pstHashTable->iHashVal = 0x12;

    return pstHashTable;
}

DIRECTORY_S* createnode(const char *key, const char*val)
{
    if(NULL == key || NULL == val)
    {
        toupcam_log_f(LOG_ERROR, "HASHTABLE_S is %s, DIRECTORY_S is %s", key?"Yes":"NULL", val?"Yes":"NULL");
        return NULL;
    }
    DIRECTORY_S *pstDirectory = (DIRECTORY_S *)malloc(sizeof(DIRECTORY_S));
    if(NULL == pstDirectory)
    {
        toupcam_log_f(LOG_ERROR, "%s", strerror(errno));
        return NULL;
    }
    memset(pstDirectory, 0, sizeof(DIRECTORY_S));

    pstDirectory->pcname = strdup(key);
    pstDirectory->pcval = strdup(val);
    pstDirectory->pdirectory = NULL;

    return pstDirectory;
}

void destorynode(DIRECTORY_S *pnode)
{
    if(NULL == pnode)
    {
        toupcam_log_f(LOG_ERROR, "DIRECTORY_S is %s", pnode?"Yes":"NULL");
        return;
    }

    if(pnode->pcname)
    {
        free(pnode->pcname);
    }
    if(pnode->pcval)
    {
        free(pnode->pcval);
    }
    pnode->pcname = NULL;
    pnode->pcval = NULL;
    
    free(pnode);
    pnode = NULL;

    return;
}

static int hashmapindex(int HashVal, DIRECTORY_S *pnode)
{
    int i = 0;
    int index = 0;
    int SumVal = 0;
    if(NULL == pnode)
    {
        return -1;
    }

    for(i = 0; i < strlen(pnode->pcname); i++)
    {
        SumVal += pnode->pcname[i];
    }

    SumVal %= HashVal;
    index = SumVal;
    
    return index;
}

/*
* 查
*/
DIRECTORY_S* findnnode(HASHTABLE_S *ptable, DIRECTORY_S *pnode)
{
    int iRet = 0;
    unsigned char ucindex = 0;
    DIRECTORY_S *pstDirectory = NULL;
    DIRECTORY_S *pstnext = NULL;
    if(NULL == ptable || NULL == pnode)
    {
        toupcam_log_f(LOG_ERROR, "HASHTABLE_S is %s, DIRECTORY_S is %s", ptable?"Yes":"NULL", pnode?"Yes":"NULL");
        return NULL;
    }

    iRet = hashmapindex(ptable->iHashVal, pnode);
    if(-1 == iRet)
    {
        toupcam_log_f(LOG_ERROR, "Hash Table index error");
        return NULL;
    }
    ucindex = (unsigned char)iRet;

    pstnext = ptable->Table[ucindex];
    while (pstnext)
    {
        if(!strcmp(pnode->pcname, pstnext->pcname))
        {
            pstDirectory = pstnext;
            break;
        }
        pstnext = pstnext->pdirectory;
    }
    
    return pstDirectory;
}

/*
* 增
*/
HASHTABLE_S* insertnode(HASHTABLE_S *ptable, DIRECTORY_S *pnode)
{
    int iRet = 0;
    unsigned char ucindex = 0;
    DIRECTORY_S *pstDirectory = NULL;
    DIRECTORY_S *pstnext = NULL;
    if(NULL == ptable || NULL == pnode)
    {
        toupcam_log_f(LOG_ERROR, "HASHTABLE_S is %s, DIRECTORY_S is %s", ptable?"Yes":"NULL", pnode?"Yes":"NULL");
        return NULL;
    }
    
    iRet = hashmapindex(ptable->iHashVal, pnode);
    if(-1 == iRet)
    {
        toupcam_log_f(LOG_ERROR, "Hash Table index error");
        return NULL;
    }
    ucindex = (unsigned char)iRet;

    if(NULL == ptable->Table[ucindex])
    {
        ptable->Table[ucindex] = pnode;
    }
    else
    {
        pstnext = ptable->Table[ucindex];
        while (pstnext->pdirectory)
        {
            pstnext = pstnext->pdirectory;
        }
        pstnext->pdirectory = pnode;
    }

    return ptable;
}

/* 
* 删
*/
HASHTABLE_S* deletenode(HASHTABLE_S *ptable, DIRECTORY_S *pnode)
{
    int iRet = 0;
    int iFind = 0;
    unsigned char ucindex = 0;
    DIRECTORY_S *pstDirectory = NULL;
    DIRECTORY_S *pstnext = NULL;
    if(NULL == ptable || NULL == pnode)
    {
        toupcam_log_f(LOG_ERROR, "HASHTABLE_S is %s, DIRECTORY_S is %s", ptable?"Yes":"NULL", pnode?"Yes":"NULL");
        return NULL;
    }
    
    iRet = hashmapindex(ptable->iHashVal, pnode);
    if(-1 == iRet)
    {
        toupcam_log_f(LOG_ERROR, "Hash Table index error");
        return NULL;
    }
    ucindex = (unsigned char)iRet;

    pstnext = ptable->Table[ucindex];
    while (pstnext)
    {
        if(!strcmp(pnode->pcname, pstnext->pcname))
        {
            if(pstDirectory)
            {
                pstDirectory->pdirectory = pstnext->pdirectory;
            }
            else /* 头删 */
            {
                ptable->Table[ucindex] = pstnext->pdirectory;
            }
            free(pstnext->pcname);
            free(pstnext->pcval);
            pstnext->pcname = NULL;
            pstnext->pcval = NULL;
            free(pstnext);
            pstnext->pdirectory = NULL;
            break;
        }
        pstDirectory = pstnext;
        pstnext = pstnext->pdirectory;
    }

    return ptable;
}

/* 
* 改
*/
HASHTABLE_S* updatenode(HASHTABLE_S *ptable, DIRECTORY_S *pnode)
{
    int iRet = 0;
    unsigned char ucindex = 0;
    DIRECTORY_S *pstDirectory = NULL;
    DIRECTORY_S *pstnext = NULL;
    if(NULL == ptable || NULL == pnode)
    {
        toupcam_log_f(LOG_ERROR, "HASHTABLE_S is %s, DIRECTORY_S is %s", ptable?"Yes":"NULL", pnode?"Yes":"NULL");
        return NULL;
    }

    iRet = hashmapindex(ptable->iHashVal, pnode);
    if(-1 == iRet)
    {
        toupcam_log_f(LOG_ERROR, "Hash Table index error");
        return NULL;
    }
    ucindex = (unsigned char)iRet;

    pstnext = ptable->Table[ucindex];
    while (pstnext)
    {
        if(!strcmp(pnode->pcname, pstnext->pcname))
        {
            free(pstnext->pcval);
            pstnext->pcval = strdup(pnode->pcval);
            toupcam_dbg_f(LOG_INFO, "alter node key:%s,val:%s", pstnext->pcname, pstnext->pcval);
            break;
        }
        pstnext = pstnext->pdirectory;
    }
    
    return ptable;    
}

/* 
* 销毁
*/
void destoryHash(HASHTABLE_S *ptable)
{ 
    int i;
    DIRECTORY_S *pnode = NULL;
    if(NULL == ptable)
    {
        toupcam_log_f(LOG_ERROR, "HASHTABLE_S is %s, DIRECTORY_S is %s", ptable?"Yes":"NULL", pnode?"Yes":"NULL");
        return ;
    }

    for (i = 0; i < HASHTABLESIZE; ++i)
    {
        if(ptable->Table[i])
        {
            while(1)
            {
                if(ptable->Table[i])
                {
                    pnode = ptable->Table[i];
                    free(pnode->pcname);
                    free(pnode->pcval);
                    pnode->pcname = NULL;
                    pnode->pcval = NULL;                    
                    free(pnode);
                    pnode->pdirectory = NULL;
                }
                else
                {
                    break;
                }
                ptable->Table[i] = ptable->Table[i]->pdirectory;
            }
        }
        ptable->Table[i] = NULL;
    }

    free(ptable);
    ptable = NULL;
    return ;
}

#if 0
int main()
{
    HashTable *g_pstToupcamHashTable = createHash();
    if(NULL == g_pstToupcamHashTable)
    {
        toupcam_log_f(LOG_ERROR, "toupcam hash table failed!");
    }

    DIRECTORY_S *pnode = createnode("xiaoliangliang", "xiaojiejie");
    insertnode(g_pstToupcamHashTable, pnode);
    
    pnode = createnode("xiaojiejie", "xiaoliangliang");
    insertnode(g_pstToupcamHashTable, pnode);

    DIRECTORY_S *p = findnnode(g_pstToupcamHashTable, pnode);
    toupcam_log_f(LOG_INFO, "key:%s, val:%s\n", p->pcname, p->pcval);

    deletenode(g_pstToupcamHashTable, pnode);
    
    pnode = createnode("xiaojiejie", "xiaoliangliang");    
    p = findnnode(g_pstToupcamHashTable, pnode);
    if(!p)
    {
        toupcam_log_f(LOG_ERROR, "no node");
    }

    pnode = createnode("xiaoliangliang", "laopo");
    updatenode(g_pstToupcamHashTable, pnode);

    pnode = createnode("xiaoliangliang", "xiaojiejie");
    insertnode(g_pstToupcamHashTable, pnode);

    int iRet = toupcam_cfg_save("./cfg", g_pstToupcamHashTable, 0);
    if(ERROR_FAILED == iRet)
    {
        toupcam_log_f(LOG_ERROR, "save cfg failed!\n");
        return ERROR_FAILED;
    }

    iRet = toupcam_cfg_read("./cfg", g_pstToupcamHashTable, 0);
    if(ERROR_FAILED == iRet)
    {
        toupcam_log_f(LOG_ERROR, "read cfg failed!\n");
        return ERROR_FAILED;
    }

    destoryHash(g_pstToupcamHashTable);
    
    return 0;
}
#endif
