#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include "../include/toupcam_log.h"
#include "../include/toupcamqueue.h"

#define ERROR_FAILED -1


TOUPCAM_DATA_QUEUE_S *CreateQueue(int width, int height, int bit)
{
    if(width <= 0 || height <= 0 || bit <= 0)
    {
        printf("invaild input parameter\n");
	    return NULL;	
    }
    int iSize = width*height*bit/8;
    int i;
    TOUPCAM_DATA_QUEUE_S *pstDataQueue = (TOUPCAM_DATA_QUEUE_S *)malloc(sizeof(TOUPCAM_DATA_QUEUE_S));
    for(i = 0; i < QUEUE_SIZE; i++)
    {
        pstDataQueue->acdata[i] = (char *)malloc(iSize);
    }
    pstDataQueue->head = 0;
    pstDataQueue->tail = 0;
    pstDataQueue->iCnt = 0;
    pthread_mutex_init(&pstDataQueue->lock, NULL);
    sem_init(&pstDataQueue->semaphore, 0, 1);
    return pstDataQueue;
}

int InQueue(TOUPCAM_DATA_QUEUE_S *pstQueue)
{
    if(NULL == pstQueue)
    {
        printf("invaild input parameter\n");
	    return ERROR_FAILED;
    }

    if(QUEUE_SIZE == (QUEUE_SIZE - pstQueue->tail + pstQueue->head)-1 && pstQueue->head != pstQueue->tail)
    {
        return pstQueue->tail;
    }

    
    if(QUEUE_SIZE == pstQueue->tail + 1)
    {
        pstQueue->tail = 0;
    }
    else
    {
        pstQueue->tail++;
    }

    if(QUEUE_SIZE == pstQueue->iCnt)
    {
        pstQueue->tail--;
    }
    else
    {
        pstQueue->iCnt++;
    }

    if(-1 == pstQueue->tail)
    {
        pstQueue->tail = QUEUE_SIZE - 1;
    }
    return pstQueue->tail;
}

int DeQueue(TOUPCAM_DATA_QUEUE_S *pstQueue)
{
    if(NULL == pstQueue)
    {
        printf("invaild input parameter\n");
	    return ERROR_FAILED;
    }

    if(QUEUE_SIZE == (QUEUE_SIZE - pstQueue->head + pstQueue->tail)-1 && pstQueue->head != pstQueue->tail)
    {
        return pstQueue->head;
    }
    
    if(QUEUE_SIZE == pstQueue->head + 1)
    {
        pstQueue->head = 0;
    }
    else
    {
        pstQueue->head++;
    }

    if(1 == pstQueue->iCnt)
    {
        pstQueue->head--;
    }
    else
    {
        pstQueue->iCnt--;
    }
    if(-1 == pstQueue->head)
    {
        pstQueue->head = QUEUE_SIZE - 1;
    }
    return pstQueue->head;
}

void Handle3dNoise(TOUPCAM_DATA_QUEUE_S *pstQueue, unsigned int uiIndex, unsigned int uiSize)
{
    if(NULL == pstQueue || 0 == uiSize)
    {
        return;
    }
    unsigned int i;
    unsigned char ucAverage = 0;

    if(1 == uiIndex)
    {
        for(i=0; i<uiSize; i++)
        {
            ucAverage = (pstQueue->acdata[uiIndex][i] + pstQueue->acdata[uiIndex-1][i] + pstQueue->acdata[QUEUE_SIZE-1][i])/3;
            pstQueue->acdata[uiIndex][i] = ucAverage;
        }
    }
    else if(0 == uiIndex)
    {
        for(i=0; i<uiSize; i++)
        {
            ucAverage = (pstQueue->acdata[uiIndex][i] + pstQueue->acdata[QUEUE_SIZE-1][i] + pstQueue->acdata[QUEUE_SIZE-2][i])/3;
            pstQueue->acdata[uiIndex][i] = ucAverage;
        }
    }
    else
    {
        for(i=0; i<uiSize; i++)
        {
            ucAverage = (pstQueue->acdata[uiIndex][i] + pstQueue->acdata[uiIndex-1][i] + pstQueue->acdata[uiIndex-2][i])/3;
            pstQueue->acdata[uiIndex][i] = ucAverage;
        }
    }
}

void DestoryQueue(TOUPCAM_DATA_QUEUE_S *pstQueue)
{
    if(NULL == pstQueue)
    {
        printf("invaild input parameter\n");
	    return;
    }
    int i;
    for(i = 0; i < QUEUE_SIZE; i++)
    {
        free(pstQueue->acdata[i]);
    }
    sem_destroy(&pstQueue->semaphore);
    pthread_mutex_destroy(&pstQueue->lock);
    free(pstQueue);
}


#if 0
int main()
{
	TOUPCAM_DATA_QUEUE_S *pstTmp = CreateQueue(720, 480, 24);
	if(NULL == pstTmp)
	{
		printf("pstTmp is nullptr\n");
		return ERROR_FAILED;
	}

	int i;
	InQueue(pstTmp);
	InQueue(pstTmp);
	InQueue(pstTmp);
	InQueue(pstTmp);
	InQueue(pstTmp);
	i = InQueue(pstTmp);
	printf("my Queue(%d) iCnt:%d\n", i, pstTmp->iCnt);
	i = DeQueue(pstTmp);
	i = DeQueue(pstTmp);
	i = DeQueue(pstTmp);
	i = DeQueue(pstTmp);
	i = DeQueue(pstTmp);
	i = DeQueue(pstTmp);
	i = DeQueue(pstTmp);
	i = DeQueue(pstTmp);
	printf("my Queue(%d) iCnt:%d\n", i, pstTmp->iCnt);

	DestoryQueue(pstTmp);
	return 0;
}
#endif
