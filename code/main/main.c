#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ns_base.h>

#include <ns_id.h>
#include <ns_main_init.h>

ULONG start_Web(VOID)
{
	INT iRet;
	pid_t wed_pid;
	wed_pid = vfork();
	if(-1 == wed_pid)
	{
		ERR_PRINTF("Create vfork Failed!");
		return ERROR_FAILE;
	}
	iRet = execl("../web/web", "web", NULL);
	if(-1 == iRet)
	{
		ERR_PRINTF("Start Web Server Failed!");
		return ERROR_FAILE;
	}

	return ERROR_SUCCESS;
}

int main(void){
	LONG lBitMapID = 0;
	
	(VOID)MAIN_Init();
	
	lBitMapID = CreateIDPool(128);
	MSG_PRINTF("id = %d", AllocID(lBitMapID, 25));
	MSG_PRINTF("id = %d", AllocID(lBitMapID, 25));
	MSG_PRINTF("id = %d", AllocID(lBitMapID, 25));
	MSG_PRINTF("id = %d", AllocID(lBitMapID, 25));

	DestroyIDPool(lBitMapID);

	MAIN_Fint();

	return 0; 
}



