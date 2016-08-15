/******************************************************************************

  Copyright (C), 2013-2014, Night Stark. langyanjun.

 ******************************************************************************
  File Name     : main.c
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2014/9/14
  Last Modified :
  Description   : WebServer 进程入口
  Function List :
  History       :
  1.Date        : 2014/9/14
    Author      : langyanjun
    Modification: Created file

******************************************************************************/

#include <stdio.h>                                                                                               
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ns_base.h>
#include <ns_daemon.h>

#include <ns_config.h>
#include <ns_bitmap.h>
#include <ns_ab.h>
#include <ns_avl.h>
#include <ns_event.h>
#include <ns_thread.h>
#include <ns_msgque.h>

#include <ns_websetting.h>
#include <ns_web_action.h>
#include <ns_web.h>
#include <ns_log.h>
#include <ns_msg_client_init.h>
#include <ns_ssl.h>

STATIC ULONG webmain_exit_cb(VOID *arg)
{
	/* 通知debug线程退出 */
	SHOW_PRINTF("Notify Debug Thread to exit! ...");
	Dbg_Thread_SendEventToDbgThread(DBG_TPF_EXIT);

	/* 等待Debug线程退出 */
	while(1)
	{
		if (BOOL_TRUE == DBG_thread_IsStop())
		{
			break;
		}
		usleep(25);
		printf("=");
	}
	SHOW_PRINTF("\nDebug Thread is exit!\n");

	return ERROR_SUCCESS;
}

STATIC INT init(PRUNTYPE_E enPRunType)
{
	THREAD_MAIN_PROC_S stThrdMainProc;

	cfg_init();

	AB_Init();
	BitMap_Init();
	THREAD_quemsg_Init();
	AVL_Sys_Init();
	MsgQue_Init();
#ifdef _AVL_TEST
	AVL_test_Main();
#endif		
	CLIENT_SSL_link_init();
	Thread_server_Init();
	WebSetting_Init(enPRunType);
	
    EVENT_fd_list_Init();
    TIMER_thread_init(); /* 需要线程的支持，注意调用位置 */

    ns_log_init("ns");
	DBG_Thread_Init();
#ifndef NS_EVENTFD
    //TODO:和dbg线程间的交互需要eventfd支持,可以使用socket
#endif
	/* 
		web_server_init 和 WEB_action_Init
		初始化出来的线程资源，将在线程退出时释放掉。
	*/
	Web_server_init();
	WEB_action_Init();

	memset(&stThrdMainProc, 0, sizeof(THREAD_MAIN_PROC_S));
	stThrdMainProc.pfThrdMainExit = webmain_exit_cb;
	Thread_main_init();
    /******/
    DataBase_client_init();
    /******/
	Thread_main_Loop(&stThrdMainProc);

	DBG_thread_Fini();
	
	THREAD_quemsg_Fint();
	Thread_server_Fint();
    TIMER_thread_Fini();
	CLIENT_SSL_link_fini();
	/* 非debug进程不需要关闭消息列队，需要在MsgQue_Fini里面加全局判断 */
	//MsgQue_Fini();
	AVL_Sys_Fini();

	SHOW_PRINTF("Progress is Exit!");
	
	return 0;
}

INT main(INT argc, CHAR *argv[])
{
	INT iRet = 0;
	PRUNTYPE_E enPRunType = PRUNTYPE_NORMAL;

	if (argc > 1)
	{
		/* ./WebServer daemon : 以守护进程的方式启动进程 */
		iRet = strcmp(argv[1], "daemon");
		if (0 == iRet)
		{
			//调用glibc库函数daemon,创建daemon守护进程
			iRet = NS_Daemon(0, 0);
			if (iRet < 0)
			{
				ERR_PRINTF("Init as deamon progress Failed!");
				return -1;
			}
			enPRunType = PRUNTYPE_DAEMON;
		}
	}

	init(enPRunType);
	
	return 0;
}
