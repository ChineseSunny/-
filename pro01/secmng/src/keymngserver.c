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
#include "keymngserverop.h"
#include "keymnglog.h"
#include "keymng_msg.h"

#define CREATE_DAEMO \
if(fork()>0) exit(0);\
	setsid();\
if(fork()>0) exit(0);
		
//定义服务器结构体变量
MngServer_Info sevifo;

int flg = 0;

void sighandler_t(int signum)
{
	flg = 1;
	return;
}

//通信
void* myConnect(void *arg)
{
	int ret = 0;
	int cfd = (int)arg;

	int timeout = 2;
	unsigned char *out = NULL;
	int outlen = -1;
	MsgKey_Req *req = NULL;
	
	int type = -1;
	
	unsigned char *resData = NULL;
	int reslen = 0;
	
	while(1)
	{
		ret = 0;
		timeout = 2;
		out = NULL;
		outlen = -1;
		req = NULL;
	
		type = -1;
	
		resData = NULL;
		reslen = 0;
		
		if(1==flg)
		{
			break;
		}
		//接收数据
		
		ret = sckServer_rev(cfd, timeout, &out,&outlen);
		if(Sck_ErrTimeOut == ret)
		{
			printf("服务器接收数据超时\n");
			continue;
		}
		else if(Sck_ErrPeerClosed == ret)
		{
			printf("cllient has closed\n");
			break;
		}	
		else if( 0 !=ret )
		{
			printf("------服务器读取数据失败------: cfd = %d, ret = %d\n", cfd, ret);
			break;
		}
		
		printf("recv data:%s\n",out);
		
		//解码请求报文
		ret = MsgDecode(out,outlen,(void **)&req,&type);
		if (ret) {
			KeyMng_Log(__FILE__, __LINE__, KeyMngLevel[4], ret, "MsgDecode error:%d", ret);
			return ret;
		}
		
		//判断报文类型
		switch(req->cmdType)
		{
			case KeyMng_NEWorUPDATE:
				//密钥协商
				ret = MngServer_Agree(&sevifo, req, &resData, &reslen);
				if (ret != 0) 
				{
					KeyMng_Log(__FILE__, __LINE__, KeyMngLevel[4], ret, "MngServer_Agree error:%d", ret);
					break;
				}	
				break;
			case KeyMng_Check:
				//密钥校验MngServer_Check(MngServer_Info *svrInfo, MsgKey_Req *msgkeyReq, unsigned char **outData, int *datalen);
				
				ret = MngServer_Check(&sevifo, req, &resData, &reslen);
				if (ret != 0) 
				{
					KeyMng_Log(__FILE__, __LINE__, KeyMngLevel[4], ret, "MngClient_Check error:%d", ret);
					break;
				}	
				break;	
			case KeyMng_Revoke:
				//密钥注销
				ret = MngClient_Revoke(&sevifo, req, &resData, &reslen);
				break;
				
			default:
				break; 
		}
		
		//send msg  服务器发送应答报文
		ret = sckServer_send(cfd,timeout,resData,reslen);
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
			printf("服务器发送数据失败\n");
			break;
		}
	}
	
	//内存释放后服务器就退出了
	/*if(*out!=NULL) 
	{
		sck_FreeMem(&out);
	}*/

	//sckServer_close(cfd);
	
	return NULL;
}


int main()
{
	int ret = 0;
	int lfd = -1;
	int timeout = 3;
	int connfd = -1;
	 
	unsigned char *out = NULL;
	int *outlen = 0;
	
	pthread_t pid = -1;
	
	//CREATE_DAEMO
	
	signal(SIGUSR1,sighandler_t);
	
	//初始化服务器信息      传出参数
	ret = MngServer_InitInfo(&sevifo);
	if(0!=ret){
		KeyMng_Log(__FILE__, __LINE__, KeyMngLevel[4], ret, "服务器端结构体信息初始化失败");
		return ret;
	}
	
	//服务器端 socket监听初始化
	ret = sckServer_init(sevifo.serverport, &lfd);
	if (ret != 0) {
		KeyMng_Log(__FILE__, __LINE__, KeyMngLevel[4], ret, "sckServer_init err\n",ret);
		return ret;
	}
			
	while(1)
	{
		if(1==flg)
		{
			break;
		}
		//建立连接

		ret = sckServer_accept(lfd,timeout,&connfd);

		if(Sck_ErrTimeOut==ret)
		{
			printf("客户端建立连接超时\n");
			continue;
		}
		else if(ret)
		{
			printf("sckServer_accept err:%d\n", ret);
			break;
		}
		
		//创建线程进行通信                          * 创建线程 传出 cfd 通信描述符*
		
		ret = pthread_create(&pid, NULL, myConnect, (void *)connfd);
		if(0!=ret)
		{
			printf("pthread_create err:%d\n", ret);
			break;
		}
	}	
	
	// 释放数据库连接池
	int IC_DBApi_PoolFree();
	
	//释放服务器连接
	sckServer_destroy();
	printf("=========8888=========\n");
	system("clear");
}
