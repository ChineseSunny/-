#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "keymng_msg.h"
#include "myipc_shm.h"
#include "poolsocket.h"
#include "keymnglog.h"
#include "keymngclientop.h"
#include "icdbapi.h"
#include "keymng_dbop.h"
#include "myipc_shm.h"

//初始化客户端
int MngClient_InitInfo(MngClient_Info *pCltInfo)
{
	int ret = 0;
	strcpy(pCltInfo->clientId,"1111");
	strcpy(pCltInfo->AuthCode,"1111");
	strcpy(pCltInfo->serverId,"0001");
	strcpy(pCltInfo->serverip,"127.0.0.1");
	
	pCltInfo->serverport = 8000;
	pCltInfo->maxnode = 9;
	pCltInfo->shmkey = 0x5555;
	pCltInfo->shmhdl = 0;
	
	//创建 打开共享内存
	ret = KeyMng_ShmInit(pCltInfo->shmkey, pCltInfo->maxnode, &(pCltInfo->shmhdl));
	if (ret != 0) {
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret, "KeyMng_ShmInit error:%d", ret);	
		return ret;	
	}
	KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[2], ret, "客户端共享内存创建/打开成功");	
	
	
	return 0;	
}

//协商密钥
int MngClient_Agree(MngClient_Info *pCltInfo)
{
	int 			ret = 0, i=0;
	unsigned char	*outData = NULL ;
	int				outLen = 0;
	
	int 			mytime = 3;
	int 			connfd = 0;
	
	if(NULL == pCltInfo)
	{
		ret = 3;
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"MngClient_Agree err%d\n",ret);
		return ret;
	}
			
	// 组织请求报文 
	MsgKey_Req 	msgKeyReq;	
	memset(&msgKeyReq, 0,sizeof(MsgKey_Req));
	
	msgKeyReq.cmdType = KeyMng_NEWorUPDATE;
	//msgKeyReq.clientId = pCltInfo->clientId;
	strcpy(msgKeyReq.clientId,pCltInfo->clientId);
	strcpy(msgKeyReq.AuthCode,pCltInfo->AuthCode);
	strcpy(msgKeyReq.serverId,pCltInfo->serverId);
	
	
	// 产生随机码
	for (i=0; i<64; i++)
	{
		msgKeyReq.r1[i] = 'a' + i;   // abcdefg...
	}
	
	//2 MsgEncode编码请求报文
	ret = MsgEncode((void *)&msgKeyReq,ID_MsgKey_Req,&outData,&outLen);
	if(ret)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"MsgEncode err%d\n",ret);
		 return ret;
	}

	//socketapi发送请求报文
	
	//客户端 初始化 socketapi
	ret = sckClient_init();
	if(ret)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"MngClient_InitInfo err%d\n",ret);
		 return ret;
	}

	//客户端 连接服务器  socketapi
	ret = sckClient_connect(pCltInfo->serverip,pCltInfo->serverport, mytime, &connfd);
	if(ret == Sck_ErrTimeOut)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"连接服务器超时%d\n",ret);
		 return ret;
	}
	else if(ret!=0)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"sckClient_connect err%d\n",ret);
		 return ret;
	}
		
	//客户端 发送报文  socketapi
	ret = sckClient_send(connfd,mytime,outData,outLen);
	//if(ret == Sck_ErrTimeOut)
	
	if(Sck_ErrTimeOut == ret)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"Sck_ErrTimeOut send err%d\n",ret);
		 return ret;
	}
	else if(ret)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"sckClient_send err%d\n",ret);
		 return ret;
	}

	//客户端接收应答报文 socketapi
	//开辟空间接收报文
	unsigned char	*msgKeyResData = NULL;  
	int				msgKeyResDataLen = 0;
	
	ret = sckClient_rev(connfd,mytime,&msgKeyResData,&msgKeyResDataLen);
	if(ret == Sck_ErrTimeOut)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"sckClient_rev timeout err%d\n",ret);
		 return ret;
	}
	else if(ret)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"sckClient_rev err%d\n",ret);
		 return ret;
	}
		
	//客户端解析应答报文   TLV --> C
	//开辟解析应答报文空间
	MsgKey_Res		*pMsgKeyRes = NULL;		
	int 			iMsgKeyResTag = 0;
	
	ret = MsgDecode(msgKeyResData,msgKeyResDataLen,(void **)&pMsgKeyRes,&iMsgKeyResTag);
	if(ret)
	{
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"MsgDecode err%d\n",ret);
		return ret;
	}

	//客户端判断，服务器密钥协商是否成功。
	if(pMsgKeyRes->rv)
	{
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"客户端检测到服务器协商秘钥失败 %d\n",ret);
		return ret;
	}
	else
	{
		printf("密钥协商成功： key no：%d\n",pMsgKeyRes->seckeyid);
	}
	
	
	//组织密钥信息结构体
	NodeSHMInfo *pNodeSHMInfo = malloc(sizeof(NodeSHMInfo));
	pNodeSHMInfo->status = 0;
	strcpy(pNodeSHMInfo->clientId, msgKeyReq.clientId);
	strcpy(pNodeSHMInfo->serverId, msgKeyReq.serverId);
	pNodeSHMInfo->seckeyid = pMsgKeyRes->seckeyid;
	
	for(i = 0; i < 64; ++i){
		pNodeSHMInfo->seckey[2*i] = msgKeyReq.r1[i];
		pNodeSHMInfo->seckey[2*i+1] = pMsgKeyRes->r2[i];
	}
	
	//客户端写共享内存
	ret = KeyMng_ShmWrite(pCltInfo->shmhdl,pCltInfo->maxnode,pNodeSHMInfo);
	if (ret) {
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret, "KeyMng_ShmWrite error:%d", ret);	
		return ret;	
	}
	
	//客户端 释放
	sckClient_closeconn(connfd);
	sckClient_destroy();
	return ret;
}

//密钥检查
int MngClient_Check(MngClient_Info *pCltInfo)
{
	//组织密钥信息结构体
	int 			ret = 0;
	NodeSHMInfo pNodeSHMInfo;

	memset(&pNodeSHMInfo, 0,sizeof(NodeSHMInfo));
	MsgKey_Req  msgKey_Req;
	unsigned char	*outData = NULL;
	int		outLen = 0;
	int connfd = 0;
	int sendtime = 3;
	if(NULL == pCltInfo)
	{
		ret = 3;
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"MngClient_Check err%d\n",ret);
		return ret;
	}
	//int KeyMng_ShmRead(int shmhdl, char *clientId, char *serverId,  int maxnodenum, NodeSHMInfo *pNodeInfo);

	//读共享内存
	ret = KeyMng_ShmRead(pCltInfo->shmhdl,pCltInfo->clientId, pCltInfo->serverId, pCltInfo->maxnode,&pNodeSHMInfo);
	if (ret) {
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret, "KeyMng_ShmRead error:%d", ret);	
		return ret;	
	}
	
	//组织请求报文	MsgKey_Req 成员赋值。
	memset(&msgKey_Req, 0,sizeof(MsgKey_Req));
	msgKey_Req.cmdType = KeyMng_Check;
	
	strcpy(msgKey_Req.AuthCode,pCltInfo->AuthCode);
	strcpy(msgKey_Req.clientId,pCltInfo->clientId);
	strcpy(msgKey_Req.serverId,pCltInfo->serverId);
		
	strncpy(msgKey_Req.r1,pNodeSHMInfo.seckey,10);
	
	//编码请求报文	MsgEncode
	ret = MsgEncode((void*)&msgKey_Req,ID_MsgKey_Req,&outData,&outLen);
	if (ret) {
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret, "MsgEncode error:%d", ret);	
		return ret;	
	}
	
	//客户端 初始化 socketapi
	ret = sckClient_init();
	if(ret)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"MngClient_InitInfo err%d\n",ret);
		 return ret;
	}
	
	//客户端 连接服务器  socketapi
	ret = sckClient_connect(pCltInfo->serverip,pCltInfo->serverport, 3, &connfd);
	if(ret == Sck_ErrTimeOut)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"Sck_ErrTimeOut conn err%d\n",ret);
		 return ret;
	}
	else if(ret!=0)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"sckClient_connect err%d\n",ret);
		 return ret;
	}
		
	//发送请求报文
	ret = sckClient_send(connfd,sendtime,outData,outLen);
	if(ret == Sck_ErrTimeOut)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"Sck_ErrTimeOut conn err%d\n",ret);
		 return ret;
	}
	else if(ret!=0)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"sckClient_connect err%d\n",ret);
		 return ret;
	}
	
	
	//客户端接收应答报文 socketapi
	unsigned char	*msgKeyResData = NULL;  
	int				msgKeyResDataLen = 0;
	
	ret = sckClient_rev(connfd,sendtime,&msgKeyResData,&msgKeyResDataLen);
	if(ret == Sck_ErrTimeOut)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"sckClient_rev timeout err%d\n",ret);
		 return ret;
	}
	else if(ret)
	{
		 KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"sckClient_rev err%d\n",ret);
		 return ret;
	}
	
	//客户端解析应答报文   TLV --> C
	MsgKey_Res		*pMsgKeyRes = NULL;		
	int 			iMsgKeyResTag = 0;
	
	ret = MsgDecode(msgKeyResData,msgKeyResDataLen,(void **)&pMsgKeyRes,&iMsgKeyResTag);
	if(ret)
	{
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"MsgDecode err%d\n",ret);
		return ret;
	}
	
	if(0 == pMsgKeyRes->rv)
	{
		printf("密钥检查成功！");
	}
	else{
		printf("密钥检查fail！");
	}
	
	printf("%s\n",pMsgKeyRes->r2);
	
	//客户端释放链接和资源
	sckClient_closeconn(connfd);
	sckClient_destroy();
	
	return ret;
}

int MngClient_Revoke(MngClient_Info *pCltInfo)
{
	
	return 0;
}

int MngClient_view(MngClient_Info *pCltInfo)
{
	return 0;
}