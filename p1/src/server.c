#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "poolsocket.h"

//通信
void* start_routine(void *arg)
{
	int ret = 0;
	int cfd = (int)arg;
	int timeout = 2;
	unsigned char *out = NULL;
	int outlen = -1;
	
	while(1)
	{
		//接收数据
		ret = sckServer_rev(cfd, timeout, &out,&outlen);
		if(Sck_ErrTimeOut == ret)
		{
			printf("服务器接收数据超时n");
			continue;
		}
		else if(Sck_ErrPeerClosed == ret)
		{
			printf("cllient has closed\n");
			break;
		}	
		else if( 0 !=ret )
		{
			printf("服务器读取数据失败\n");
			break;
		}
		
		printf("recv data:%s\n",out);
		
		//send msg
		ret = sckClient_send(cfd,timeout, out, outlen);
		if(Sck_ErrTimeOut == ret)
		{
			printf("服务器端发送数据超时\n");
			continue;
		}
		else if(Sck_ErrPeerClosed == ret)
		{
			printf("cllient has closed\n");
			break;
		}
		else if(0!=ret)
		{
			sck_FreeMem(&out);
			printf("服务器读取数据失败\n");
			break;
		}
	}
	
	if(*out!=NULL) 
	{
		sck_FreeMem(&out);
	}
	sckServer_close(cfd);
	
	return NULL;
}


int main()
{
	int ret = 0;
	int port = 9999;
	int lfd = -1;
	int timeout = 3;
	int connfd = -1;
	
	unsigned char *out = NULL;
	int *outlen = 0;
	
	pthread_t pid = -1;
	
	//初始化服务器             传出参数
	ret = sckServer_init(port, &lfd);
	if(0!=ret){
		printf("sckServer_init err:%d\n", ret);
		return ret;
	}
			
	while(1)
	{
		//建立连接
		ret = sckServer_accept(lfd,timeout,&connfd);
		if(Sck_ErrTimeOut==ret)
		{
			printf("客户端建立连接超时\n");
			continue;
		}
		else if(0!=ret)
		{
			printf("sckServer_accept err:%d\n", ret);
			break;
		}
		
		//创建线程进行通信                             * 创建线程 传出 cfd 通信描述符*
		ret = pthread_create(&pid, NULL,start_routine, (void *)connfd);
		if(0!=ret)
		{
			printf("pthread_create err:%d\n", ret);
			break;
		}
	}	
	
	sckServer_destroy();
}
