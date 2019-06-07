/*
 * rtp.c
 *
 *  Created on: Jun 24, 2013
 *      Author: Administrator
 */
#include <string.h> 
#include "../include/rtp.h"
#include "../include/socket.h"
 
 
/* 创建rtp包 */
void *creat_rtp_pack(struct rtp_data *data)
{
    unsigned char *buf;
    /* 数据长度检测，数据长度大于最大包长，则分包。*/
    //if(data->datalen>MAX_PACK_LEN && data->datalen>(data->bufrelen+data->rtpdatakcount))
    if(data->datalen-data->offset>MAX_PACK_LEN)
    {
        /* 分包 */        
        //unsigned int templen=(data->datalen-data->bufrelen-1);
        unsigned int templen=(data->datalen-data->offset-1);
        data->rtpdatakcount+=1;
        if(templen>MAX_PACK_LEN-1)  /* 包含MAX_PACK_LEN */
        {
            buf=(unsigned char *)malloc(MAX_PACK_LEN);
            memset(buf,0,MAX_PACK_LEN);
            if(data->bufrelen==0)
            {
                /* 第一个分包 */
                memcpy((buf+RTP_HEAD_LEN+1), ((unsigned char*)data->buff+data->offset), (MAX_PACK_LEN-RTP_HEAD_LEN-1));
                data->rtp_fui=((*(buf+RTP_HEAD_LEN+1)&0xE0));
                data->rtp_fuh=(*(buf+RTP_HEAD_LEN+1)&0x1f);
                *(buf+RTP_HEAD_LEN+1)=(data->rtp_fuh|(0x80));
                data->bufrelen+=MAX_PACK_LEN;
                data->offset+=MAX_PACK_LEN-RTP_HEAD_LEN-1;
                //printf("first\n");
            }
            else
            {
                /* 中间分包 */
                memcpy((buf+RTP_HEAD_LEN+2),((unsigned char*)data->buff+data->offset),(MAX_PACK_LEN-RTP_HEAD_LEN-2));
                *(buf+RTP_HEAD_LEN+1)=(data->rtp_fuh|(0x00));
                data->bufrelen+=MAX_PACK_LEN;
                data->offset+=MAX_PACK_LEN-RTP_HEAD_LEN-2;
                //printf("middle\n");
            }
 
            *(buf+RTP_HEAD_LEN)=(data->rtp_fui|(0x1c));
 
        }
        else
        {
            /* 最后一个分包 */
            templen=data->datalen-data->offset;
            buf=(unsigned char *)malloc(templen+RTP_HEAD_LEN+2);
            memset(buf,0,templen+RTP_HEAD_LEN+2);
            memcpy((buf+RTP_HEAD_LEN+2),((unsigned char*)data->buff+data->offset),templen);
            *(buf+RTP_HEAD_LEN)=(0x1c|data->rtp_fui);
            *(buf+RTP_HEAD_LEN+1)=(data->rtp_fuh|(0x40));
            data->bufrelen+=templen+RTP_HEAD_LEN+2;
            //data->offset+=templen-1;
            data->offset+=templen;
            //printf("last ");
        }
    } 
    //else if(data->datalen>data->bufrelen)
    else if(data->datalen>data->offset)
    {
        /* 数据长度小于包长，则不分包 */
        unsigned int templen=(data->datalen-data->offset);
#if 0
        buf=(unsigned char *)malloc(templen+RTP_HEAD_LEN);
        //buf=(unsigned char *)malloc(data->datalen+RTP_HEAD_LEN);
        //memset(buf,0,data->datalen+RTP_HEAD_LEN);
        memset(buf,0,templen+RTP_HEAD_LEN);
 
        //memcpy((buf+RTP_HEAD_LEN),data->buff,data->datalen);
        //data->bufrelen+=data->datalen+RTP_HEAD_LEN;
        memcpy((buf+RTP_HEAD_LEN),(unsigned char*)data->buff+data->offset,templen);
        //data->bufrelen+=data->datalen+RTP_HEAD_LEN;
        data->bufrelen+=templen+RTP_HEAD_LEN;
        data->offset += templen;
#endif

        buf=(unsigned char *)malloc(templen+RTP_HEAD_LEN+2);
        memset(buf,0,templen+RTP_HEAD_LEN+2);
        memcpy((buf+RTP_HEAD_LEN+2),((unsigned char*)data->buff+data->offset),templen);
        *(buf+RTP_HEAD_LEN)=(0x1c|data->rtp_fui);
        *(buf+RTP_HEAD_LEN+1)=(data->rtp_fuh|(0x40));
        data->bufrelen+=templen+RTP_HEAD_LEN+2;
        data->offset+=templen;
        //printf("ex first %d\n", templen);
    }
    else
    {
        return NULL;
    }
    return buf;
}

static void *creat_sham_rtp_pack(struct rtp_data *data)
{
    unsigned char *buf;
    /* 数据长度检测，数据长度大于最大包长，则分包。*/
    if(data->datalen > MAX_PACK_LEN && data->datalen > data->bufrelen)
    {
        /* 分包 */
        unsigned int templen=(data->datalen-data->bufrelen);
        data->rtpdatakcount+=1;
        if(templen > MAX_PACK_LEN)
        {
            buf=(unsigned char *)malloc(MAX_PACK_LEN);
            memset(buf,0,MAX_PACK_LEN);
            if(data->bufrelen==0)
            {
                /* 第一个分包 */
                memcpy(buf, (unsigned char*)data->buff, MAX_PACK_LEN);
                data->bufrelen += MAX_PACK_LEN;
                data->offset += MAX_PACK_LEN;
            }
            else
            {
                /* 中间分包 */
                memcpy(buf, ((unsigned char*)data->buff + data->offset), MAX_PACK_LEN);
                data->bufrelen += MAX_PACK_LEN;
                data->offset += MAX_PACK_LEN;
            }
 
        }
        else
        {
            /* 最后一个分包 */
            buf=(unsigned char *)malloc(templen);
            memset(buf,0,templen);
            memcpy(buf, ((unsigned char*)data->buff + data->offset), templen);
            data->bufrelen += templen;
            data->offset += templen;
        }
    }
    else if(data->datalen>data->bufrelen)
    {
        /* 数据长度小于包长，则不分包 */
        buf=(unsigned char *)malloc(data->datalen);
        memset(buf,0,data->datalen);
 
        memcpy(buf,data->buff,data->datalen);
        data->bufrelen += data->datalen;
    }
    else
    {
        return NULL;
    }

    return buf;
}

struct rtp_pack *sham_rtp_pack(struct rtp_data *pdata)
{
    char *rtp_buff;
    int len=pdata->bufrelen;
 
    /* 获取封包后的rtp数据包 */
    rtp_buff=(char *)creat_sham_rtp_pack(pdata);
     
    struct rtp_pack *pack;

    if(rtp_buff!=NULL)
    {
        /* 创建rtp_pack结构 */
        pack=(struct rtp_pack *)malloc(sizeof(struct rtp_pack));
        pack->databuff=rtp_buff;
        pack->packlen=pdata->bufrelen-len;
    }
    else
    {
        free(pdata->buff);
        pdata->buff=NULL;
        return NULL;
    }
    return pack;    
}
 
struct rtp_pack *rtp_pack(struct rtp_data *pdata,struct rtp_pack_head *head)
{
    char *rtp_buff;
    int len=pdata->bufrelen;
 
    /* 获取封包后的rtp数据包 */
    rtp_buff=(char *)creat_rtp_pack(pdata);
     
    struct rtp_pack *pack;

    if(rtp_buff!=NULL)
    {
        /* 固定头部填充 */
        SET_RTP_FIXHEAD();
        
        /* 创建rtp_pack结构 */
        pack=(struct rtp_pack *)malloc(sizeof(struct rtp_pack));
        pack->databuff=rtp_buff;
        pack->packlen=pdata->bufrelen-len;
    }
    else
    {
        free(pdata->buff);
        pdata->buff=NULL;
        return NULL;
    }
    return pack;
}
 
//H264 开始码检测
char checkend(unsigned char *p)
{
    if((*(p+0)==0x00)&&(*(p+1)==0x00)&&(*(p+2)==0x00)&&(*(p+3)==0x01))
        return 1;
    else
        return 0;
}
 
//压入新读取的字节
void puttochar(unsigned char *tempbuff,unsigned char c)
{
    *(tempbuff+0)=*(tempbuff+1);
    *(tempbuff+1)=*(tempbuff+2);
    *(tempbuff+2)=*(tempbuff+3);
    *(tempbuff+3)=c;
}

//获取H264 数据
void *getdata(FILE *file)
{
    //判断文件是否结束
    if(feof(file)!=0)
        return NULL;
 
    unsigned int len=0;//读取到的数据长度
    unsigned char *buf=(unsigned char *)malloc((sizeof(char)*1024*1024));//分配内存
    if(buf==NULL)
    {
        return NULL;
    }
    memset(buf,0,(sizeof(char)*1024*1024));//清零
 
    unsigned char tempbuff[4];
    unsigned char c;
    unsigned int i=0;
     
    //获取文件的当前读取位置，跳过文件头的开始码
    if(ftell(file)==0)
        fread(tempbuff,sizeof(char),4,file);
         
    //首次读取数据，填满temp缓冲区。
    fread(tempbuff,sizeof(char),4,file);
 
    //开始码检测
    while(!checkend(tempbuff))
    {
        //向数据缓存区，压入数据
        *(buf+i)=tempbuff[0];
        len+=fread(&c,sizeof(char),1,file);
         
        //将下一个字节压入缓冲区。
        puttochar(tempbuff,c);
        i++;
        if(feof(file)!=0)
        {
            memcpy((buf+i),tempbuff,sizeof(tempbuff));
            len+=4;
            break;
        }
    }
 
    unsigned char *databuf=(unsigned char *)malloc(len);
    memcpy(databuf,buf,len);
    
    free(buf);
    buf=NULL;
     
    //rtp_data结构创建，填充
    struct rtp_data *rtp_data=(struct rtp_data *)malloc(sizeof(struct rtp_data));
    memset(rtp_data,0,sizeof(struct rtp_data));
 
    rtp_data->buff=databuf;
    rtp_data->datalen=len;
    rtp_data->bufrelen=0;
    return rtp_data;
}
 
/* rtp数据包发送 */
char rtp_send(struct rtp_pack *rtp,struct sockets *sock)
{
    int num=0;
    
    /* 通过网络将数据发送出去 */
    if((num=sendto(sock->local,rtp->databuff,rtp->packlen,0,(struct sockaddr *)sock->cliaddr[0],sizeof(struct sockaddr_in)))==-1)
    {
        fail("socket: %s\n", strerror(errno));
    }
    unsigned char datartp[rtp->packlen];
    /*
    memcpy(datartp, rtp->databuff, rtp->packlen);
    for(int i = 0; i < rtp->packlen; i++)
    {
        printf("%02X ", datartp[i]);
    }
    printf("\r\n");
    */
    //debug("number is : %d packlen is : %d\n", num, rtp->packlen);
    //debug("send: sock->local:%d ip:%s packet len:%d.\n", sock->local, inet_ntoa(sock->cliaddr[0]->sin_addr), rtp->packlen);
    /* 释放缓冲区 */
    free(rtp->databuff);
    rtp->databuff=NULL;

    return 0;
}
