#ifndef _SOCKET_H_
#define _SOCKET_H_

/* sock ip */
#define SERVER_IP   "192.168.1.100" /* default */
#define CLIENT_IP   "192.168.1.100" /* default */

#define UDP_PORT  5004
#define TCP_PORT  5005
#define TCP_LISTEN_NUM 10
#define TCP_BUFFER_SIZE  1024
#define TCP_REQUEST 0x01
#define TCP_RESPONSE 0x02
#define TCP_BUFFERSIZE 1024

struct sockets
{
    int local;
    int reclen;
    FILE *in,*out;
    struct sockaddr_in *cliaddr;
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
extern void common_hander(int fd, char *pBuff, unsigned short ucSize);




#endif
