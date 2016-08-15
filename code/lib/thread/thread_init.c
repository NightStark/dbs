#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include <ns_base.h>
#include <ns_thread.h>

ULONG THREAD_init(VOID)
{
	Thread_server_Init();
	
	return ERROR_SUCCESS;
}

VOID THREAD_Fint(VOID)
{
	Thread_server_Fint();
}



