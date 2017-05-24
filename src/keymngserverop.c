#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "keymnglog.h"
#include "keymngserverop.h"
#include "poolsocket.h"
#include "keymng_msg.h"
#include "myipc_shm.h"
#include "keymng_shmop.h"
#include "icdbapi.h"
#include "keymng_dbop.h"

//初始化服务器
int MngServer_InitInfo(MngServer_Info *svrInfo)
{
	int ret = 0;
	strcpy(svrInfo->serverId,"0001");
	strcpy(svrInfo->dbuse,"SECMNG");
	strcpy(svrInfo->dbpasswd,"SECMNG");
	strcpy(svrInfo->dbsid,"orcl");   //数据库实例/数据库名
	
	svrInfo->dbpoolnum = 10;
	strcpy(svrInfo->serverip,"127.0.0.1");
	svrInfo->serverport = 8888;
	svrInfo->maxnode = 5;
	svrInfo->shmkey = 0x1112;
	svrInfo->shmhdl = 0;
	
	//共享内存初始化
	ret = KeyMng_ShmInit(svrInfo->shmkey, svrInfo->maxnode, &(svrInfo->shmhdl));
	if(ret)
	{
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"KeyMng_ShmInit err %d\n",ret);
		return ret;
	}
	
	KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[2], ret,"初始化共享内存成功");
	
	//初始化数据库连接池
	ret = IC_DBApi_PoolInit(svrInfo->dbpoolnum, svrInfo->dbsid, svrInfo->dbuse, svrInfo->dbpasswd);
	if(ret)
	{
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"IC_DBApi_PoolInit err %d\n",ret);
		return ret;
	}
	
	return 0;
}

static int myseckeyid = 100;

//服务器端密钥协商 应答流程
//1 组织应答报文
//2 编码应答报文 

//3 协商密钥
//4 写共享内存
//5 网点密钥写数据库

int MngServer_Agree(MngServer_Info *svrInfo, MsgKey_Req *msgkeyReq, unsigned char **outData, int *datalen)
{
	int ret = 0, i = 0;
	
	ICDBHandle handle = NULL;
	
	//检查参数是否合法，错误打日志
	if( NULL == svrInfo || NULL == msgkeyReq)
	{
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"参数传递为空 %d\n",ret);
		return ret;
	}	
	//从连接池获取数据库连接
		ret = IC_DBApi_ConnGet(&handle, 0, 0);
		if(ret)
		{
			KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"IC_DBApi_ConnGet err%d",ret);
			return ret;
		}
		//开启事务
		ret = IC_DBApi_BeginTran(handle);
		if(ret)
		{
			KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"IC_DBApi_BeginTran err%d",ret);
			return ret;
		}
		
		int keyid = 0;
		//获取数据库中密钥编号(密钥序列号)
		ret = KeyMngsvr_DBOp_GenKeyID(handle,&keyid);
		if(ret)
		{
			KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"KeyMngsvr_DBOp_GenKeyID err%d",ret);
			return ret;
		}
	
	//定义应答报文结构体，赋初值。
	MsgKey_Res	msgKeyRes;    //clientID/ serverID / rv=1 r2[] seckeyid
	
	memset(&msgKeyRes,0,sizeof(msgKeyRes));
	
	msgKeyRes.rv = 0;
	strcpy(msgKeyRes.clientId,msgkeyReq->clientId);
	strcpy(msgKeyRes.serverId,msgkeyReq->serverId);
	
	msgKeyRes.seckeyid = keyid ;
	// 产生随机码
	for (i=0; i<64; i++)
	{
		msgKeyRes.r2[i] = 'a' + i;  	//aabbccddeeffgg.....
	}
	
		//密钥信息结构体
		
		NodeSHMInfo * pNodeSHMInfo = (NodeSHMInfo *)malloc(sizeof(NodeSHMInfo));
		
		memset(pNodeSHMInfo,0,sizeof(NodeSHMInfo));
		
		pNodeSHMInfo->status = 0;
		strcpy(pNodeSHMInfo->clientId,msgkeyReq->clientId);
		strcpy(pNodeSHMInfo->serverId,msgkeyReq->serverId);
		pNodeSHMInfo->seckeyid = msgKeyRes.seckeyid;
		
		//服务器协商密钥
		for(i = 0;i < 64;i++)
		{
			pNodeSHMInfo->seckey[2*i] = msgkeyReq->r1[i];
			pNodeSHMInfo->seckey[2*i+1] = msgKeyRes.r2[i];
		}
		
		
		//写共享内存
		ret = KeyMng_ShmWrite(svrInfo->shmhdl, svrInfo->maxnode,pNodeSHMInfo);
		if(ret)
		{
			KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"server KeyMng_ShmWrite err%d",ret);
			return ret;
		}
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[2], ret,"server KeyMng_ShmWrite success");
		
		//写数据库
		ret = KeyMngsvr_DBOp_WriteSecKey(handle,pNodeSHMInfo);
		if(ret==0)
 		{
 			KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[2], ret,"KeyMngsvr_DBOp_WriteSecKey success !提交");
			IC_DBApi_Commit(handle);
 		}
 		else
 		{
 			KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[2], ret,"KeyMngsvr_DBOp_WriteSecKey err！回退");
 			
 			IC_DBApi_Rollback(handle);
 		}
 		if(ret == IC_DB_CONNECT_ERR)
 		{
 			IC_DBApi_ConnFree(handle,0);
 		}
 		else
 		{
 			IC_DBApi_ConnFree(handle,1);
 		}
 		
		//编码应答报文 
 		ret = MsgEncode((void *)&msgKeyRes,ID_MsgKey_Res,outData,datalen);
 		if(ret)
 		{
 			KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"server MsgEncode err%d",ret);
			return ret;
 		}
	
	//验证客户端请求报文的serverid 和 服务器端的serverid是否相同
	if(strcmp(svrInfo->serverId,msgkeyReq->serverId)!=0)
	{
		KeyMng_Log(__FILE__,__LINE__, KeyMngLevel[4], ret,"请求serverid 与服务器serverid不同%d\n",ret);
		return -1;
	}
	return ret;
}