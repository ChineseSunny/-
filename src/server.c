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

//定义服务器结构体变量
MngServer_Info sevifo;

//通信
void* start_routine(void *arg)
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
		cfd = (int)arg;
		timeout = 2;
		out = NULL;
		outlen = -1;
		req = NULL;
	
		type = -1;
	
		resData = NULL;
		reslen = 0;
		
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
			printf("服务器读取数据失败\n");
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
			case KeyMng_Check:
				//密钥校验
				//ret = MngClient_Check(&mngClientInfo);
				break;	
			case KeyMng_Revoke:
				//密钥注销
				//ret = MngClient_Revoke(&mngClientInfo);
				break;
				
			default:
				break; 
		}
		
		//send msg
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
	//int port = 9999;
	int lfd = -1;
	int timeout = 3;
	int connfd = -1;
	 
	unsigned char *out = NULL;
	int *outlen = 0;
	
	pthread_t pid = -1;
	
	//初始化服务器信息      传出参数
	ret = MngServer_InitInfo(&sevifo);
	
	if(0!=ret){
		KeyMng_Log(__FILE__, __LINE__, KeyMngLevel[4], ret, "服务器端结构体信息初始化失败");
		return ret;
	}
	
	//服务器端连接初始化
	ret = sckServer_init(sevifo.serverport,&lfd);
	if (ret != 0) {
		KeyMng_Log(__FILE__, __LINE__, KeyMngLevel[4], ret, "sckServer_init err\n",ret);
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
