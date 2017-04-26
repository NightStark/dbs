#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ns_log.h>


unsigned long  get_demsg_seq(void);
void Dbg_Print_Log(const char *pcFileName, char *pcChar, unsigned long ulLen);

#define DBGASSERT(exp) 	\
	if(!(exp))			\
	{					\
		printf("Failed! in file:%s at line:%d\n",\
			__FILE__,__LINE__	\
		);				\
	}					\
		

/*
 * 0: Close
 * 1: Emergency
 * 2: Alert
 * 3: Critical
 * 4: Error
 * 5: Warning
 * 6: Notice
 * 7: Info
 * 8: Debug
 *
 */

typedef enum DBG_TYPE
{
	DBG_TYPE_MSG = 0, /* Debug */
	DBG_TYPE_WARN,    /* Warning */
	DBG_TYPE_SHOW,    /* notice */
	DBG_TYPE_ERR,     /* Error */
	
	DBG_TYPE_MAX,
}DBG_TYPE_E;

#include <ns_lilist.h>
typedef struct tag_DBG_PRINT_BUFFER
{
	DCL_NODE_S stNode;

	ULONG ulDbgSeq;
    CHAR *pcString;
    const CHAR *pcDbgFunName; /* DBG时的函数名 */
    const CHAR *pcDbgFileName;
    INT iDbgLine;
    INT iModID;

    DBG_TYPE_E enDbgType;
}DBG_PRINT_BUFFER_S;

 VOID Debug_DestroyPrintData(IN DBG_PRINT_BUFFER_S *pstDbgPrintBuf);
	
VOID  Dbg_print(IN DBG_TYPE_E enDbgType, 
                      IN INT   iModID,
					  IN const CHAR *pcDbgFileName,
					  IN const CHAR *pcDbgFunName, 
					  IN ULONG ulDbgLine,
					  IN const char *fmt, ...);

/* 如果开启线程间通信的debug打印，则说有的打印将直接打印的屏幕，如果在daemon状态，则有可能什么也不打印 */
#define DEBUG_ON 
//#define THREAD_DEBUG_ON	
//#define _DEBUG_THREAD_

#ifdef DEBUG_ON

#define ERR_PRINTF_ONT(templt,...)\
	printf("\033[0;31m[%08lu][", get_demsg_seq());\
	printf(templt,##__VA_ARGS__);\
	printf("]\033[0m[file:%s][function:%s][line:%d]", __FILE__, __func__,__LINE__);\
	printf("\n")

#define WARN_PRINTF_ONT(templt,...)\
	printf("\033[0;33m[%08lu][", get_demsg_seq());\
	printf(templt,##__VA_ARGS__);\
	printf("][0m[file:%s][function:%s][line:%d]\033",__FILE__, __func__,__LINE__);\
	printf("\n")
	
#define SHOW_PRINTF_ONT(templt,...)\
	printf("\033[0;35m[%08lu][", get_demsg_seq());\
	printf(templt, ##__VA_ARGS__);\
	printf("]\033[0m");\
	printf("\n")
	
#define MSG_PRINTF_ONT(templt,...)\
	printf("\033[0;32m[%08lu][", get_demsg_seq());\
	printf(templt, ##__VA_ARGS__);\
	printf("][fun:%s]\033[0m", __func__);\
	printf("\n")

#ifndef DBG_MOD_ID
#define DBG_MOD_ID NS_LOG_ID(NS_LOG_MOD_DEFAULT, 1)
#endif

#define DBG_DO_PRINT(type , templt, ...) \
    Dbg_print(type, DBG_MOD_ID, __FILE__, __func__, __LINE__, templt, ##__VA_ARGS__)
		
#define ERR_PRINTF(templt,...)				\
		DBG_DO_PRINT(DBG_TYPE_ERR, templt, ##__VA_ARGS__)
	
#define WARN_PRINTF(templt,...)				\
		DBG_DO_PRINT(DBG_TYPE_WARN, templt, ##__VA_ARGS__)
	
#define SHOW_PRINTF(templt,...)				\
		DBG_DO_PRINT(DBG_TYPE_SHOW, templt, ##__VA_ARGS__)
	
#define MSG_PRINTF(templt,...)				\
		DBG_DO_PRINT(DBG_TYPE_MSG, templt, ##__VA_ARGS__)

#define DBGMSG_PRINTF(templt,...)			\
		MSG_PRINTF_ONT(templt,##__VA_ARGS__)
#define DBGERR_PRINTF(templt,...)			\
		ERR_PRINTF_ONT(templt,##__VA_ARGS__)


#define DBG_PRINT_LOG(pcFileName, pcChar, ulLen)\
{												\
	Dbg_Print_Log(pcFileName, pcChar, ulLen);   \
}	

#define DBG_PRINTF_BIN(pcTitle ,pBuf, pcPLen) \
{											\
	unsigned int i = 0;						\
	char *pcBuf = (char *)pBuf;				\
	printf("\033[0;31m[%s]\033[0m\n", pcTitle); \
	for(i = 0; i < pcPLen; i++)				\
	{										\
		DBGASSERT(NULL != (pcBuf+pcPLen));	\
		printf("0x%02x ", pcBuf[i]);		\
		if(0 == i % 16)						\
			printf("\n");					\
	}										\
	printf("\n");							\
}

#else

#define  ERR_PRINTF(templt,...)
#define WARN_PRINTF(templt,...)
#define SHOW_PRINTF(templt,...)
#define  MSG_PRINTF(templt,...)

#define  DBG_PRINT_LOG(pcFileName, pcChar, ulLen)
#define  DBG_PRINTF_BIN(pcBuf, pcPLen)	

#define  ERR_PRINTF_ONT(templt,...)
#define WARN_PRINTF_ONT(templt,...)
#define SHOW_PRINTF_ONT(templt,...)
#define  MSG_PRINTF_ONT(templt,...)
#define DBGERR_PRINTF(templt,...)

#endif

#define __THREAD_DEBUG__
#ifdef __THREAD_DEBUG__
VOID  Dbg_ThrdNameReg(INT iThrdId, CHAR *pcThrdNameFmt, ...);
CHAR *Dbg_ThrdNameGet(INT iThrdId);
#define DBG_THRD_NAME_REG(iThrdId, pcThrdName, ...) \
	Dbg_ThrdNameReg((iThrdId), (pcThrdName), ##__VA_ARGS__)
#define DBG_THRD_NAME_GET(iThrdId) Dbg_ThrdNameGet((iThrdId))
#else
#define DBG_THRD_NAME_REG(iThrdId, pcThrdName) 
#define DBG_THRD_NAME_GET(iThrdId) 
#endif

typedef enum DBGTHRDPRINTFFLAG
{
	DBG_TPF_NONE = 0,

	DBG_TPF_STRING,
	DBG_TPF_EXIT,
	
	DBG_TPF_MAX,
}DBG_THRD_PRINTF_FLAG_E;

 BOOL_T Dbg_Thread_ISDebugThread(VOID);
 VOID Dbg_Thread_SetDbgThreadPrintFlag(DBG_THRD_PRINTF_FLAG_E enDbgFlag);
 DBG_THRD_PRINTF_FLAG_E Dbg_Thread_GetDbgThreadPrintFlag(VOID);
 INT Dbg_Thread_GetDbgThreadEventFd(VOID);

VOID   Dbg_Thread_SetDbgThreadID(IN INT iDbgThrdId);
ULONG  DBG_Thread_Init(VOID);
VOID   DBG_thread_Fini(VOID);
BOOL_T DBG_thread_IsRun(VOID);
BOOL_T DBG_thread_IsStop(VOID);
ULONG Dbg_thread_WriteDbgMsgList(IN DBG_PRINT_BUFFER_S *pstDbgPrintBuf);

VOID  DBG_print_ByType(IN DBG_PRINT_BUFFER_S *pstDbgPrintBuf);
VOID  Dbg_process_ByFriend(IN DBG_PRINT_BUFFER_S *pstDbgPrintBuf);
ULONG Debug_process_Init(VOID);
VOID  DBG_print_SetRemoteTerminalName(IN const CHAR *pcRTName);
ULONG DBG_print_OpenRTerminal(VOID);
VOID  DBG_print_CloseRTerminal(VOID);

unsigned int DBG_Print_Rate_of_Progress(unsigned int now, unsigned int total);
INT Dbg_Thread_SendEventToDbgThread(IN DBG_THRD_PRINTF_FLAG_E enDbgThrdPrintFlag);

#endif //__DEBUG_H__
