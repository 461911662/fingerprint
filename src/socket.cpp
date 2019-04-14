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

void common_hander(int fd, char *pBuff, unsigned short ucSize)
{
    if(fd < 0 || NULL == pBuff)
    {
        printf("%s: parse fail", __func__);
        return;
    }

    int i, j;
    int iRet = 0;
    /*
    for(i = 0; i < ARRAY_SIZE(g_Comm_Entry); i++)
    {
        if(*pBuff == g_Comm_Entry[i].cmd_id)
        {
            iRet = g_Comm_Entry[i].handle(fd, pBuff+1, ucSize-1);
            printf("request: cmd:%c,subcmd:%hd", *pBuff, *(unsigned short*)(pBuff+1));
            for(j=2; j<ucSize; j++)
            {
                printf(" %c", pBuff[i]);
            }
            printf("cc:%d\n", iRet);
        }
    }*/
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
    sockcli=(struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(sockcli,0,sizeof(struct sockaddr_in));
 
    sockcli->sin_port=htons(UDP_PORT);
 
    sockcli->sin_family = AF_INET;
    sockcli->sin_addr.s_addr=inet_addr(CLIENT_IP);
 
    sock->cliaddr=sockcli;

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