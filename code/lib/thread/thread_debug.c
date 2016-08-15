#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ns_base.h>
#include <ns_thread.h>

CHAR *aucThrdStatusDeBugList[THREAD_STATUS_MAX] = 
{
	[THREAD_STATUS_IDEL] 	= "idle",
	[THREAD_STATUS_WAIT] 	= "wait",
	[THREAD_STATUS_BLOCK] 	= "block",
	[THREAD_STATUS_RUN] 	= "run",
	[THREAD_STATUS_ERROR] 	= "error",
};

CHAR * Thread_debug_GetThreadStatus(THREAD_STATUS_E eThrdStatus)
{
	if (eThrdStatus >= THREAD_STATUS_IDEL && THREAD_STATUS_IDEL < THREAD_STATUS_MAX)
	{
		return aucThrdStatusDeBugList[eThrdStatus];
	}
	else
	{
		ERR_PRINTF("Invalid thread status!");
		return NULL;
	}
}

