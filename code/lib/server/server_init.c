#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_table.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_lilist.h>
#include <ns_thread.h>
#include <ns_msg_server_init.h>
#include <ns_server.h>
#include <ns_ssl.h>

VOID SERVER_Init(VOID)
{
	Server_CreateClientTable();
	THREAD_init();
    MSG_server_link_Init();
    EVENT_fd_list_Init();
    TIMER_thread_init(); /* 需要线程的支持，注意调用位置 */
	SERVER_SSL_link_init();
	MSG_server_Init();
	return;
}

VOID SERVER_Fint(VOID)
{
	Server_DestroyClientTable();
	THREAD_Fint();
    TIMER_thread_Fini();
	MSG_server_Fint();
    MSG_server_link_Fini();
	SERVER_SSL_link_fini();
	return;
}
