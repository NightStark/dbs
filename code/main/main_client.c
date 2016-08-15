#include <ns_base.h>

#include <ns_msg_client_init.h>

INT main(VOID)
{
	BitMap_Init();
	THREAD_quemsg_Init();
	Thread_server_Init();
	
	DataBase_client_init();

	while(1);
	THREAD_quemsg_Fint();
	Thread_server_Fint();
}


