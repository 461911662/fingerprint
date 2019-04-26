/*
 * rtp.h
 *
 *  Created on: Jun 24, 2013
 *      Author: Administrator
 */
 
#ifndef RTP_H_
#define RTP_H_
 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <arpa/inet.h>
 
#include <ifaddrs.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
 
#include <unistd.h>
#include <getopt.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
 
#include "Error.h"
 
#define SPS (1)
#define PPS (2)
#define RTP_VERSION_OFFSET  (0)
#define RTP_MARK_OFFSET     (1)
#define RTP_SEQ_OFFSET      (2)
#define RTP_TIMETAMP_OFFSET (4)
#define RTP_SSRC_OFFSET     (8)
 
#define MAX_PACK_LEN (1448)
#define RTP_HEAD_LEN (12)
 
#define H264_TYPE           (0x60)
//rtp头 版本号设置
#define SET_RTP_VERSION()   {*(rtp_buff+RTP_VERSION_OFFSET)=0x80;}
#define SET_RTP_MARK()      {*(rtp_buff+RTP_MARK_OFFSET)=(H264_TYPE&0x7f);}
//rtp头 序列号设置
#define SET_RTP_SEQ()       {*(rtp_buff+RTP_SEQ_OFFSET+1)=(head->sernum>>0)&0xff;\
                             *(rtp_buff+RTP_SEQ_OFFSET)=(head->sernum>>8)&0xff;\
                            }
//rtp头 时间戳设置
#define SET_RTP_TIMETAMP()  {*(rtp_buff+RTP_TIMETAMP_OFFSET+3)=(head->timtamp>>0)&0xff;\
                             *(rtp_buff+RTP_TIMETAMP_OFFSET+2)=(head->timtamp>>8)&0xff;\
                             *(rtp_buff+RTP_TIMETAMP_OFFSET+1)=(head->timtamp>>16)&0xff;\
                             *(rtp_buff+RTP_TIMETAMP_OFFSET+0)=(head->timtamp>>24)&0xff;\
                            }
 
//rtp头 ssrc流标记设置
#define SET_RTP_SSRC()  {*(rtp_buff+RTP_SSRC_OFFSET+3)=(head->ssrc>>0)&0xff;\
                         *(rtp_buff+RTP_SSRC_OFFSET+2)=(head->ssrc>>8)&0xff;\
                         *(rtp_buff+RTP_SSRC_OFFSET+1)=(head->ssrc>>16)&0xff;\
                         *(rtp_buff+RTP_SSRC_OFFSET+0)=(head->ssrc>>24)&0xff;\
                            }
#define SET_RTP_FIXHEAD()   {SET_RTP_VERSION();\
                             SET_RTP_MARK();\
                             SET_RTP_SEQ();\
                             SET_RTP_TIMETAMP();\
                             SET_RTP_SSRC();\
                            }

//rtp头结构
struct rtp_pack_head
{
    unsigned short sernum;
    unsigned int timtamp;
    unsigned int ssrc;
};
 
//rtp数据结构
struct rtp_data
{
    void *buff;
    char rtp_fui;
    char rtp_fuh;
    unsigned int offset;
    unsigned int datalen;
    unsigned int bufrelen;
    unsigned int rtpdatakcount;
};
//rtp 包结构
struct rtp_pack
{
    void *databuff;
    unsigned int packlen;
    unsigned int packtype;
};
 

extern pthread_mutex_t g_PthreadMutexUDP;
//FILE *temp;
//创建rtp头
void creat_rtp_head(struct rtp_pack *rtp,struct rtp_pack_head *head);
//rtp封包
struct rtp_pack *rtp_pack(struct rtp_data *pdata,struct rtp_pack_head *head);
struct rtp_pack *sham_rtp_pack(struct rtp_data *pdata);
//获取264数据
void *getdata(FILE *file);
//rtp数据发送
char rtp_send(struct rtp_pack *rtp,struct sockets *sock);

 
#endif /* RTP_H_ */