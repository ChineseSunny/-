#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main()
{
	//int shmget(key_t key, size_t size, int shmflg);
	
	int shmid = -1;
	//                          权限
	shmid = shmget(0x8888, 512, 0666 | IPC_CREAT);
	
	if(-1== shmid)
	{
		perror("shmget error:");
		exit(1);	
	}

	printf("------shm create: shmid:%d\n", shmid);
	
	//void *shmat(int shmid, const void *shmaddr, int shmflg);
	
	int *p = 0;
	
	p = shmat(shmid, NULL, 0);
	
	if (p == (void *)-1) {
		perror("shmat error:");
		exit(1);
	}
	
	printf("-----p:%d\n", p);
	
	printf("-----Enter ANY key 删除共享内存----\n");
	getchar();
	
	int ret = shmctl(shmid, IPC_RMID, NULL);
	
	if(-1 == ret)
	{
		perror("shmctl error:");
		exit(1);
	}
	
		printf("-----共享内存 已经销毁----\n");
}