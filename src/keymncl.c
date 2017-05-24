#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "keymng_msg.h"
#include "myipc_shm.h"
#include "poolsocket.h"
#include "keymnglog.h"
#include "keymngclientop.h"


int Usage()
{
	int usel = -1;
	system("clear");
 	printf("\n  /*************************************************************/");
  printf("\n  /*************************************************************/");
  printf("\n  /*     1.密钥协商                                            */");
  printf("\n  /*     2.密钥校验                                            */");
  printf("\n  /*     3.密钥注销                                            */");
  printf("\n  /*     4.密钥查看                                            */");
  printf("\n  /*     0.退出系统                                            */");
  printf("\n  /*************************************************************/");
  printf("\n  /*************************************************************/");
  printf("\n\n  选择:");
  
  scanf("%d",&usel);
  while(getchar()!='\n');
  
  return usel;
}

int main()
{
	int ret = 0, usel = 0;
	
	//初始化客户端信息
	MngClient_Info pCltInfo;
	memset(&pCltInfo,0,sizeof(MngClient_Info));
	
	ret = MngClient_InitInfo(&pCltInfo);
	
	//判断是否初始化成功
	if(0!=ret)
	{
		printf("MngClient_InitInfo err\n");
		return ret;
	}
	
	while(1)
	{
		usel = Usage();
		switch(usel)
		{
			case 0:
				return 0;
			
			case KeyMng_NEWorUPDATE:
				ret = MngClient_Agree(&pCltInfo);
				break;
				
			case KeyMng_Check:
				ret = MngClient_Check(&pCltInfo);
				break;
				
			/*case KeyMng_Revoke:
				ret = MngClient_Revoke(&pCltInfo);
				break;
				
			case KeyMng_View:
				ret = MngClient_view(&pCltInfo);
				break;
				*/
				
			default:
				printf("输入的数值不合法");
				break;	
		}
		
		if (ret)
		{
			printf("\n!!!!!!!!!!!=======ERROR=========!!!!!!!!!!!");
			printf("\n错误码是：%d\n", ret);
		}
		else
		{
			printf("\n!!!!!!!!!!!!!!!!!!!!SUCCESS!!!!!!!!!!!!!!!!!!!!\n");
		}	
		getchar();
	}
	
	printf("hello keymngclient...\n");
	
	return 0;
}