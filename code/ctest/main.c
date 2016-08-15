#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ns_base.h>

#include <ns_table.h>
#include <ns_opdata.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_lilist.h>
#include <ns_log.h>
#include <ns_get.h>
#include <ns_id.h>
#include <ns_thread.h>
#include <ns_timer.h>
#include <ns_event.h>
#include <ns_mysql.h>
#include <ns_config.h>

#include "ctest.h"

ULLONG ullTimerNow = 0;
ULLONG ullTimerPre = 0;
INT    g_iWorkThradId = -1;
ULONG TimerCB_TEST(INT iTimerId, VOID *arg)
{
	STATIC INT    iCnt = 0;
	INT    iThreadId = -1;
	ULONG *pulPara = NULL;
    THREAD_INFO_S *pstThrdInfo = NULL;
    INT  iQMsgFd = -1;

	pulPara = (ULONG *)arg;
	iThreadId = pulPara[0];

    ullTimerNow = TIMER_get_BootTime();
    ERR_PRINTF("timer out (%lld)", ullTimerNow - ullTimerPre);
    ullTimerPre = ullTimerNow;

    if (g_iWorkThradId < 0) {
        return 0;
    }
    pstThrdInfo = Thread_server_GetByThreadId(g_iWorkThradId);
    iQMsgFd = pstThrdInfo->stThrdQueMsgInfo.iThrdQueMsgEventFd;
    ERR_PRINTF("iQMsgFd = %d", iQMsgFd);
    //THREAD_server_QueMsg_Send(IN INT iThrdId,						    
    ERR_PRINTF("Cnt(%d)", iCnt);
	if (iCnt++ > 10) {
		TIMER_DeleteForThread(iThreadId, iTimerId);
		Thread_server_KillByThrdID(g_iWorkThradId);

	}



	return ERROR_SUCCESS;
}

ULONG TIMER_THRD_TEST_CB(VOID * pArgs/* THREAD_INFO_S * */)
{
    THREAD_INFO_S *pstThrdInfo = NULL;

    DBGASSERT(NULL != pArgs);

    pstThrdInfo = (THREAD_INFO_S *)pArgs;
	MSG_PRINTF("---- Timer Thread Init Start ----");

	ULONG ulPara[4] = {0};
    ulPara[0] = (ULONG)(pstThrdInfo->iThreadID);
	TIMER_CreateForThread(pstThrdInfo->iThreadID,
  								     1,
  								     TimerCB_TEST,
  								     ulPara);

	MSG_PRINTF("---- Timer Thread Init End----");

    return ERROR_SUCCESS;
}

STATIC VOID QueMsgRecv_TEST(IN INT iSrcThrdId, IN const THREAD_QUEMSG_DATA_S *pstThrdQueMsg)
{
	ULONG ulRet;
    (VOID)ulRet;

	DBGASSERT(NULL != pstThrdQueMsg);

	ERR_PRINTF("Web Action Handle Failed!");
	
    return;
}

int main(int argc, char *argv[])
{
    ns_log_init("ctest");
	MSG_PRINTF("CTEST is on.");

    //msg_en_de_test_main();
	const char * file = "http://7xl3iu.dl1.z0.glb.clouddn.com/device/build/tcpdump";
	G_OPT_SS stOptss;
	memset(&stOptss, 0, sizeof(stOptss));
	stOptss.flag = OPT_FLAG_DOWNLOAD_FILE | OPT_FLAG_NO_TEST;
	stOptss.duration = 3000;
	stOptss.period = 1;
	//snprintf(stOptss.opts_d.urlBuf, sizeof(stOptss.opts_d.urlBuf), "%s", "http://dldir1.qq.com/qqfile/qq/QQ8.3/18033/QQ8.3.exe");
	snprintf(stOptss.opts_d.urlBuf, sizeof(stOptss.opts_d.urlBuf), "%s", file);
	//snprintf(stOptss.opts_d.filePath, sizeof(stOptss.opts_d.filePath), "%s", "./xx");

	ns_get_main(&stOptss);

/*
	BitMap_Init();
	THREAD_quemsg_Init();
    Thread_server_Init();
    EVENT_fd_list_Init();
    TIMER_thread_init();

    Thread_server_CreateWithEp(TIMER_THRD_TEST_CB, 
                                                NULL, 
                                                THREAD_TYPE_WORK);

	g_iWorkThradId = Thread_server_CreateWithEpQMsg(NULL,
											   NULL,
											   THREAD_TYPE_WORK,
											   QueMsgRecv_TEST);
                                               */

    //mysql_main("a");
	/*
    NET_IP_INFO_S *pstIfInfo = NULL;
    pstIfInfo = NET_GetIFInfo();
    if (pstIfInfo == NULL) {
        ERR_PRINTF("get local ip info failed");
    }
    INT i = 0;
    MSG_PRINTF("cnt[%d]", pstIfInfo->iCnt);
    for (i = 0; i < pstIfInfo->iCnt; i++) {
        MSG_PRINTF("ifname:[%s]", pstIfInfo->pstNetIFData[i].szIFName);
        MSG_PRINTF("mac   :[%s]", pstIfInfo->pstNetIFData[i].szIFMACAddr);
        MSG_PRINTF("ifaddr:[%s]", pstIfInfo->pstNetIFData[i].szIFAddr);
        MSG_PRINTF("ifaddr:[0x%x]", pstIfInfo->pstNetIFData[i].uiIFAddr);
        if (strncmp(pstIfInfo->pstNetIFData[i].szIFName, "eth0", 4) == 0) {
            ERR_PRINTF("xx");
        } else if (strncmp(pstIfInfo->pstNetIFData[i].szIFName, "eno", 3) == 0) {
            ERR_PRINTF("xx");
        }
    }
    NET_DestroyIFInfo(pstIfInfo);
    pstIfInfo = NULL;
	*/

	//cfg_init();
	m_download_test();

	/*
    while(1) {
        sleep(5);
    }

    Thread_server_Fint();
	Thread_server_Fint();
	*/


	MSG_PRINTF("CTEST is over.");
	return 0;
}
