#ifndef _SOCKET_H_
#define _SOCKET_H_

/* sock ip */
#ifdef TOUPCAM_RELEASE
#define SERVER_IP   "192.168.1.100" /* stream sock1 default */
#define CLIENT_IP   "192.168.1.100" /* dgram sock default */
#else
#define SERVER_IP   "192.168.100.1" /* stream sock1 default */
#define CLIENT_IP   "192.168.100.102" /* dgram sock default */
#endif

#define UDP_PORT1  5004
#define UDP_PORT2  5008
#define TCP_PORT  9999
#define TCP_LISTEN_NUM 1
#define TCP_BUFFER_SIZE  1024
#define TCP_REQUEST 0x01   /* 服务端请求手机端 */
#define TCP_RESPONSE 0x02  /* 服务端响应手机端请求 */
#define UDP_DATATSM 0x05
#define TCP_BUFFERSIZE 1024
//#define TCP_BUFFERSIZE 1023


struct sockets
{
    int local;
    int reclen;
    FILE *in,*out;
    struct sockaddr_in *cliaddr[2];
};
 
//配置信息
typedef struct config {
    char* ip;
    char time;
    int port;
    int type;
    int protocol;
}SOCKCONFIG;


extern char g_cBuffData[TCP_BUFFER_SIZE];
extern char g_ReqResFlag;

extern unsigned int socket_dgram_init();
extern unsigned int socket_stream_init();
extern void common_hander(int fd, void *pBuff, unsigned int uiSize);


#endif
