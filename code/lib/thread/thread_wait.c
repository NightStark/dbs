#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>  
#include <unistd.h>  

#include <ns_base.h>

#include <ns_table.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_lilist.h>
#include <ns_thread.h>
#include <ns_wait.h>
 
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t cond;  

ULONG THREAD_wait_CondInit(VOID)
{
	ULLONG ulRet = 0;
	ulRet = pthread_cond_init(&cond, NULL);
	if (0 != ulRet)
	{
		return ERROR_FAILE;
	}
	
	return ERROR_SUCCESS;
}

ULONG THREAD_wait_CondFint(VOID)
{
	ULLONG ulRet = 0;
	ulRet = pthread_cond_destroy(&cond);
	if (0 != ulRet)
	{
		return ERROR_FAILE;
	}
	
	return ERROR_SUCCESS;
}


/* 回调函数 pfEpWaitWakeupDo 的返回值会决定是否继续等待 */
ULONG THREAD_wait(VOID *pData, EPOLL_WATI_WAKEUP_DO pfEpWaitWakeupDo)
{  
	ULONG ulRet = 0;
	
    pthread_mutex_lock(&mtx); 

    /* 临界区 */

    while(1)
    {
    	/* 非临界区 */
		ulRet = pthread_cond_wait(&cond, &mtx); 
		/* 临界区 */
		if (0 != ulRet)
		{
			ulRet = ERROR_FAILE;
			break;
		}
		
		ulRet = pfEpWaitWakeupDo(pData);
		if (ERROR_SUCCESS == ulRet)
		{
			break;
		}
    }
    
    pthread_mutex_unlock(&mtx);             
    //临界区数据操作完毕，释放互斥锁  

    
	return ulRet;
}

ULONG THREAD_wait_wakeup(VOID *pData, EPOLL_WAIT_SIG_DO pfEpWaitSigDo)
{
	ULONG ulRet = 0;

	pthread_mutex_lock(&mtx);	 //需要操作临界资源，先加锁，	

	ulRet = pfEpWaitSigDo(pData);
	
	pthread_cond_signal(&cond);  
	pthread_mutex_unlock(&mtx); 	 //解锁  

	return ulRet;
}

