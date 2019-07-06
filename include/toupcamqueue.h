#ifndef _TOUPCAMQUEUE_H_
#define _TOUPCAMQUEUE_H_

#define QUEUE_SIZE 60

typedef struct toupcam_data_queue{
    char *acdata[QUEUE_SIZE];
    int iCnt;
    int head;
    int tail;
}TOUPCAM_DATA_QUEUE_S;

TOUPCAM_DATA_QUEUE_S *CreateQueue(int width, int height, int bit);
int InQueue(TOUPCAM_DATA_QUEUE_S *pstQueue);
int DeQueue(TOUPCAM_DATA_QUEUE_S *pstQueue);
void DestoryQueue(TOUPCAM_DATA_QUEUE_S *pstQueue);
#endif
