#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <ns_base.h>
#include <ns_client.h>

/* 工作线程单独处理各种操作 */
void * client_work_ThreadMain(VOID *arg)
{
	ULONG ulRet;
	INT iClientfd = 0;

	iClientfd = (INT)(ULONG)arg;
	MSG_PRINTF("Client work thread is stark");/* 改成其他事件触发 */
	MSG_PRINTF("Client will ask after 1s...");
	sleep(1);
	
	ulRet = CLIENT_AskServer(iClientfd);
	if (ERROR_SUCCESS != ulRet)
	{
		ERR_PRINTF("Ask Server Failed!");
	}

	MSG_PRINTF("Ask Server SUCCESS");

	/* 进入命令行模式 */

	//
	CLIENT_CreateClientTable(iClientfd);

	return (VOID *)0;
	
}

ULONG CLIENT_work_ThreadStart(IN INT iClientfd)
{
	ULONG ulRet = 0;
    pthread_t tClientThrdID = 0;
    
	pthread_create(&tClientThrdID, NULL ,client_work_ThreadMain, (VOID *)(LONG)iClientfd); /* Transfor to LONG is for 64bit or can cut the date*/
    if(0 != ulRet)
    {
        ERR_PRINTF("thread create failed!");
    }

    return ERROR_SUCCESS;
}



