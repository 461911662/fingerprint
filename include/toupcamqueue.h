#ifndef _TOUPCAMQUEUE_H_
#define _TOUPCAMQUEUE_H_
#include <pthread.h>

#define QUEUE_SIZE 60

typedef struct toupcam_data_queue{
    char *acdata[QUEUE_SIZE];
    pthread_mutex_t lock;
    sem_t semaphore;
    int iCnt;
    int head;
    int tail;
}TOUPCAM_DATA_QUEUE_S;

TOUPCAM_DATA_QUEUE_S *CreateQueue(int width, int height, int bit);
int InQueue(TOUPCAM_DATA_QUEUE_S *pstQueue);
int DeQueue(TOUPCAM_DATA_QUEUE_S *pstQueue);
void Handle3dNoise(TOUPCAM_DATA_QUEUE_S *pstQueue, unsigned int uiIndex , unsigned int uiSize);
void DestoryQueue(TOUPCAM_DATA_QUEUE_S *pstQueue);
#endif
