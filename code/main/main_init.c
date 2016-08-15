#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>

#include <ns_base.h>

#include <ns_lilist.h>
#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_table_type.h>
#include <ns_bitmap.h>
#include <ns_server.h>
#include <ns_msg_server_init.h>
#include <ns_main_init.h>
#include <ns_log.h>

ULONG MAIN_Init(VOID)
{
    ns_log_init("server");
	BitMap_Init();
	DataBase_Init();
	SERVER_Init();

	return ERROR_SUCCESS;
}

VOID MAIN_Fint(VOID)
{
	SERVER_Fint();
	DataBase_Fini();

	return;
}
