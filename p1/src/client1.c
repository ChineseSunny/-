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

int  main(void)
{
	int 		ret = 0;
	char 		*ip = "127.0.0.1"; 
	int 		port = 9999;
	int 		time = 3;
	int 		connfd = 0;
	
	//客户端 初始化
	ret = sckClient_init();
	if (ret != 0) {
			printf("客户端初始化失败：%d\n", ret);
			return ret;		
		}
		
	//客户端 连接服务器
	ret = sckClient_connect(ip, port, time, &connfd);
		if (Sck_ErrTimeOut == ret) {
			printf("与服务器建立连接超时\n");
			return ret;	
	}
	else if (ret != 0) {
			printf("与服务器建立连接失败：%d\n", ret);
			return ret;		
		}
			
	unsigned char *data = "abdefghijklmnopqrst";
	int 		datalen = 8;
	int sendtime = 2;
	int revtime = 2;
	//客户端 发送报文
	
	ret = sckClient_send(connfd,sendtime,data, datalen);
		if (Sck_ErrTimeOut == ret) {
			printf("sendtime out\n");
			return ret;
	}	else if (ret != 0) {
			printf("send msg fail %d\n", ret);
			return ret;		
		}
		
	unsigned char *myout = NULL;
	int 			outlen = 0;	
	//客户端 接受报文
	ret = sckClient_rev(connfd, revtime, &myout,&outlen);
	if (Sck_ErrTimeOut == ret) {
			printf("recvtime out\n");
			return ret;
	}	
	else if (ret != 0) {
			printf("send msg fail %d\n", ret);
			return ret;		
	}
	printf("client recv:%s  len:%d\n",myout,outlen);
	
	//客户端 释放内存
	sck_FreeMem((void **)&myout);
	
	//客户端 关闭和服务端的连接
	ret = sckClient_closeconn(connfd);

	//客户端 释放
	sckClient_destroy();	
	return 0;
}

