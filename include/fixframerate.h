#ifndef _FIX_FRAME_RATE
#define _FIX_FRAME_RATE
#include <list>
#include <pthread.h>

using namespace std;

class FixFrameRate
{
public:
        FixFrameRate(int height, int width, int bit, int fixnum);
        ~FixFrameRate();
        int getQueueNum();
        int getFixnum();
        //int getQueueFrameFormat(int &h, int &w, int &b);
        char *PopQueue();
        void PushQueue(char *byte);
private:
        int height;
        int width;
        int bit;
        int iCnt;
        int fixnum;
        list<char *> *ndata; 
        pthread_mutex_t lock;
};

#endif
