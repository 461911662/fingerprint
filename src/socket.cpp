#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include "../include/socket.h"
#include "../include/Error.h"
#include "../include/common_toupcam.h"

char g_cBuffData[TCP_BUFFER_SIZE] = {0};
char g_ReqResFlag;

void common_hander(int fd, void *pBuff, unsigned int uiSize)
{
    if(fd < 0 || NULL == pBuff)
    {
        printf("%s: parse fail", __func__);
        return;
    }

    int i, j;
    int iRet = 0;

    TOUPCAM_COMMON_REQUES_S *pstToupcamReq = (TOUPCAM_COMMON_REQUES_S *)pBuff;
    for(i=0; i<g_Comm_Entry_Size; i++)
    {
        if(pstToupcamReq->com.cmd == g_Comm_Entry[i].cmd_id)
        {
            iRet = g_Comm_Entry[i].handle(fd, pstToupcamReq, uiSize);
            printf("request(%d): cmd:%02x,subcmd:%04x,", fd, pstToupcamReq->com.cmd, pstToupcamReq->com.subcmd);
            for(j=0; j<uiSize-4; j++)
            {
                printf("%02x,", pstToupcamReq->data.reserve[j]);
            }
            printf("cc: %d\n", iRet);
       }
    }
    return;
}

//创建sock套接字
int socket_setup(SOCKCONFIG *socketconfig)
{
    int s;
 
    if ((s=socket(AF_INET, (socketconfig->type), (socketconfig->protocol)))== -1)
    {
        fail("socket: %s\n", strerror(errno));
    }
 
    return s;
}
 
//填充sockets结构
struct sockets *get_sockets(int sock)
{
 
    struct sockets *soc;
    soc = (struct sockets *)malloc(sizeof(struct sockets));
    if (soc == NULL)
    {
        warn("malloc failed.\n");
        return NULL;
    }
 
    soc->local = sock;
    soc->in = fdopen(sock, "r");
    soc->out = fdopen(sock, "w");
 
    return soc;
}

unsigned int socket_dgram_init()
{
    int iRet = ERROR_SUCCESS;
    
	//创建配置信息
    SOCKCONFIG config;
 
    config.time=5;
    config.type=SOCK_DGRAM;
    config.protocol=0;
 
    //建立网络套接字
    int s=socket_setup(&config);
    sock=get_sockets(s);
 
    //创建，填充客户端地址
    struct sockaddr_in* sockcli;
    sockcli = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in)*2);
    memset(sockcli,0,sizeof(struct sockaddr_in)*2);
 
    sockcli->sin_port=htons(UDP_PORT1);
    sockcli->sin_family = AF_INET;
    sockcli->sin_addr.s_addr=inet_addr(CLIENT_IP);

    (sockcli+1)->sin_port=htons(UDP_PORT2);
    (sockcli+1)->sin_family = AF_INET;
    (sockcli+1)->sin_addr.s_addr=inet_addr(CLIENT_IP);
 
    sock->cliaddr[0]=sockcli;
    sock->cliaddr[1]=sockcli+1;

    return iRet;
}

unsigned int socket_stream_init()
{
    int iRet = ERROR_SUCCESS;

    SOCKCONFIG config;
    config.time=5;
    config.type=SOCK_STREAM;
    config.protocol=0;

    int iServerFd = socket_setup(&config);
    sock1=get_sockets(iServerFd);

    struct sockaddr_in stSockServer;
    stSockServer.sin_port = htons(TCP_PORT);
    stSockServer.sin_family = AF_INET;
    stSockServer.sin_addr.s_addr = inet_addr(SERVER_IP);

    iRet = bind(sock1->local, (struct sockaddr *)&stSockServer, sizeof(struct sockaddr_in));
    if(ERROR_SUCCESS != iRet)
    {
        perror("stream bind");
        return ERROR_FAILED;
    }

    iRet = listen(sock1->local, TCP_LISTEN_NUM);
    if(ERROR_SUCCESS != iRet)
    {
        perror("stream listen");
        return ERROR_FAILED;
    }   

    return ERROR_SUCCESS;
}
