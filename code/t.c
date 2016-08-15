#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <ns_log.h>

#define VOID void

void *pdata = NULL;

#ifdef MOD_ID
#undef MOD_ID
#endif
#define MOD_ID NS_LOG_ID(NS_LOG_MOD_THREAD, 1)

VOID * g_Thread_main(VOID *pData)
{
	ns_log(LOG_ERR, "Son therad is start!\n");
	ns_log(LOG_WARNING, "Son therad is start!\n");
	ns_log(LOG_NOTICE, "Son therad is start!\n");
	ns_log(LOG_INFO, "Son therad is start!\n");
	ns_log(LOG_DEBUG, "Son therad is start!\n");

	pdata = malloc(64);
	pdata = malloc(64);
	if (NULL == pdata)
	{
		printf("malloc fialed\n");
	}

	return (VOID *)0;
}

int Dbg_Thread_Create(VOID)
{
	int iRet;
	int iEpFd  = -1;
	pthread_t pThreadID;
	pthread_attr_t thrd_Attr;


	iRet  = pthread_attr_init(&thrd_Attr);
	iRet |= pthread_attr_setdetachstate(&thrd_Attr, PTHREAD_CREATE_DETACHED);
	if (iRet != 0)
	{
        printf("DBG pthread attr set detached failed!\n");
	}

    iRet = pthread_create(&pThreadID,
					      &thrd_Attr, 
					      g_Thread_main, 
					      (VOID *)(iEpFd));
    if(0 != iRet)
    {
        printf("DBG Thread create failed!\n");
    }

    pthread_attr_destroy(&thrd_Attr);

	return 0;
}

int main(void)
{
	
    ns_log_init("test");
	Dbg_Thread_Create();

	sleep(2);

	free(pdata);
	
	return 0;
}
