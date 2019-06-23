#include <pthread.h>
#include <list>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include "../include/fixframerate.h"

using namespace std;

FixFrameRate::FixFrameRate(int height, int width, int bit, int fixnum):iCnt(0)
{
	this->height = height;
	this->width = width;
	this->bit = bit;
	this->fixnum = fixnum;
	this->ndata = new list<char *>;
    pthread_mutex_init(&this->lock, NULL);
}

int FixFrameRate::getQueueNum()
{
	return this->iCnt;
}

int FixFrameRate::getFixnum()
{
    return this->fixnum;
}

void FixFrameRate::PushQueue(char *byte)
{
	if(NULL == byte)
	{
		cout << __func__ <<": input invalid" << endl;
		return;
	}
	pthread_mutex_lock(&lock);
	if(this->fixnum == this->iCnt)
	{
		this->ndata->pop_front();
		this->iCnt--;
	}
	
	ndata->push_back(byte);
	this->iCnt++;
    //cout << "cur Queue Num:" << this->iCnt << endl;
	pthread_mutex_unlock(&lock);
	return;
}

char* FixFrameRate::PopQueue()
{
    if(0 == this->iCnt)
    {
        return NULL;
    }
    
	pthread_mutex_lock(&lock);
	if(1 == this->iCnt)
	{
	    pthread_mutex_unlock(&lock);
		return ndata->front();
	}
	char *pcTmp = ndata->front();
	this->ndata->pop_front();
	this->iCnt--;
    //cout << "cur Queue Num:" << this->iCnt << endl;
	pthread_mutex_unlock(&lock);
	return pcTmp;
}

FixFrameRate::~FixFrameRate()
{
	cout << __func__ << "exit." << endl;
	while(this->iCnt)
	{
		this->PopQueue();
		this->iCnt--;
	}
    pthread_mutex_destroy(&this->lock);
	if(NULL != ndata)
	{
		free(this->ndata);
		cout << __func__ << "free(ndata)." << endl;
	}
}

#if 0
int main()
{
	FixFrameRate ffr(1,2,3,4);
	char cCnt = 1;
	/*
	while(1)
	{
		sleep(1);
		ffr.PushQueue(&cCnt);
		cout << "ffr Num:" << ffr.getQueueNum() << endl;
	}
	*/
	ffr.PushQueue(&cCnt);
	ffr.PushQueue(&cCnt);
	ffr.PushQueue(&cCnt);
	cout << "ffr Num:" << ffr.getQueueNum() << endl;
	cout << "Pop:" << static_cast<int>(*ffr.PopQueue()) << endl;
	cout << "ffr Num:" << ffr.getQueueNum() << endl;
	return 0;
}
#endif
