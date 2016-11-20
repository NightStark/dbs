#ifndef __DEBUG_C__
#define __DEBUG_C__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <sys/epoll.h>
#ifndef NS_EVENTFD
#include <sys/eventfd.h>
#endif

#include <ns_base.h>

#include <ns_thread.h>

STATIC ULONG ulMsgSeq = 0;
STATIC ULONG ulNSDbgSeq = 0; 
#define DBG_THREAD_EPOLL_EVENT_MAX (16)

/* 只可用于Debug 线程打印 */
unsigned long inline get_demsg_seq(void)
{
	return ulMsgSeq++;
}

/* 非Debug线程，多线程使用 */
unsigned long inline get_NsDbg_seq(void)
{
	return ulNSDbgSeq++;
}

#define DBG_ERR_PRINTF_COLOR  ("31");
#define DBG_WARN_PRINTF_COLOR ("33");
#define DBG_SHOW_PRINTF_COLOR ("35");
#define DBG_MSG_PRINTF_COLOR  ("32");

/*
	LOG_EMERG	0	
	LOG_ALERT	1	
	LOG_CRIT	2	
	LOG_ERR		3	
	LOG_WARNING	4	
	LOG_NOTICE	5	
	LOG_INFO	6	
	LOG_DEBUG	7	
 */

STATIC INT g_iDbgType2LeveList[] = {
    [DBG_TYPE_MSG]  = 7,
    [DBG_TYPE_WARN] = 4,
    [DBG_TYPE_SHOW] = 6,
    [DBG_TYPE_ERR]  = 3,
};


STATIC INT DBG_print_Type2Level(DBG_TYPE_E eDbgType)
{
    INT iLevel = 0;

    if (eDbgType < 0 || eDbgType > DBG_TYPE_MAX)
    {
        return -1;
    }

    iLevel = g_iDbgType2LeveList[eDbgType]; 

    return iLevel;
}

VOID DBG_print_ByType(IN DBG_PRINT_BUFFER_S *pstDbgPrintBuf)
{
	const CHAR *pcFormat;
    INT iLevel = 0;

	DBGASSERT(NULL != pstDbgPrintBuf);

	switch (pstDbgPrintBuf->enDbgType)
	{
		case DBG_TYPE_MSG:
			pcFormat = DBG_MSG_PRINTF_COLOR;
			break;
		case DBG_TYPE_WARN:
			pcFormat = DBG_WARN_PRINTF_COLOR;
			break;
		case DBG_TYPE_SHOW:
			pcFormat = DBG_SHOW_PRINTF_COLOR;
			break;
		case DBG_TYPE_ERR:
			pcFormat = DBG_ERR_PRINTF_COLOR;
			break;
		default:
			DBGERR_PRINTF("Invalid Debug Type!\n");
			pcFormat = NULL;
	}

	if (NULL == pcFormat)
	{
		return;
	}

    iLevel = DBG_print_Type2Level(pstDbgPrintBuf->enDbgType);
    if (iLevel < 0) {
        return;
    }

#if 0
	printf( "\033[0;"
				"%sm"
				"[%08lu]"
				"[%s]"
			"\033[0m" 
       		"[file:%s][function:%s][line:%d]\n", 
       		pcFormat,
			((pstDbgPrintBuf)->ulDbgSeq), 	     
			((pstDbgPrintBuf)->pcString),		
			((pstDbgPrintBuf)->pcDbgFileName),	
			((pstDbgPrintBuf)->pcDbgFunName),	
			((pstDbgPrintBuf)->iDbgLine));

#else

    ns_log_mx(pstDbgPrintBuf->iModID, iLevel, 
            "\033[0;"
            "%sm"
            "[%08lu]"
            "[%s]"
            "\033[0m" 
            "[file:%s][function:%s][line:%d]\n", 
            pcFormat,
            ((pstDbgPrintBuf)->ulDbgSeq), 	     
            ((pstDbgPrintBuf)->pcString),		
            ((pstDbgPrintBuf)->pcDbgFileName),	
            ((pstDbgPrintBuf)->pcDbgFunName),	
            ((pstDbgPrintBuf)->iDbgLine));
#endif

	return;
}

#define RTERMINAL_NAME_BUF_LEN (32)
STATIC CHAR g_acRTerminalName[RTERMINAL_NAME_BUF_LEN];
STATIC INT  g_IRTerminalFd = -1;

VOID DBG_print_SetRemoteTerminalName(IN const CHAR *pcRTName)
{
	DBGASSERT(NULL != pcRTName);

	snprintf(g_acRTerminalName, RTERMINAL_NAME_BUF_LEN, pcRTName);
	MSG_PRINTF("Set Remote Terminal Name : [%s].", g_acRTerminalName);

	return;
}

ULONG DBG_print_OpenRTerminal(VOID)
{
	INT iRTFd;
	
	iRTFd = open(g_acRTerminalName, O_RDWR|O_CREAT, 0666);
	if (iRTFd < 0)
	{
		DBGERR_PRINTF("Open Remote Terminal Fiale!");
		return ERROR_FAILE;
	}

	g_IRTerminalFd = iRTFd;
	MSG_PRINTF("Open Remote Terminal SUCCESS!");

	return ERROR_SUCCESS;
}

VOID DBG_print_CloseRTerminal(VOID)
{
	close(g_IRTerminalFd);
	g_IRTerminalFd = -1;

	return;
}

ULONG DBG_print_OnRemoteTerminal(IN const DBG_PRINT_BUFFER_S *pstDbgPrintBuf)
{
	CHAR ucBuf[1024];
	INT  iLen = 0;

	DBGASSERT(NULL != pstDbgPrintBuf);
	
	memset(ucBuf, 0, sizeof(ucBuf));

	/*
	iLen = snprintf(ucBuf, sizeof(ucBuf), 
									"\033[0;32m[%08lu][%s]\033[0m" \
									"[file:%s][function:%s][line:%d]\n",  \
									get_demsg_seq(),				\
									((pstDbgPrintBuf)->pcString),		\
									((pstDbgPrintBuf)->pcDbgFileName),	\
									((pstDbgPrintBuf)->pcDbgFunName),	\
									((pstDbgPrintBuf)->iDbgLine));
	*/
	iLen = snprintf(ucBuf, sizeof(ucBuf),
								  "<ul class=\"table_log_list_elment\">"
								      "<li id=\"l_no\" class=\"log_no\">%08lu</li>"
								      "<li id=\"l_func\" class=\"log_c\">%s</li>"
								      "<li id=\"l_stt\" class=\"log_stt\">%s</li>"
								      "<li id=\"l_func\" class=\"log_func\">%s()</li>"
								      "<li id=\"l_ln\" class=\"log_ln\">%d</li>"
								  "</ul>",
									((pstDbgPrintBuf)->ulDbgSeq),
									((pstDbgPrintBuf)->pcString),		
									((pstDbgPrintBuf)->pcDbgFileName),	
									((pstDbgPrintBuf)->pcDbgFunName),	
									((pstDbgPrintBuf)->iDbgLine)
		  );

	iLen = write(g_IRTerminalFd, ucBuf, iLen);
	if (iLen < 0)
	{
		DBGERR_PRINTF("Write Message On Remote Terminal Failed!");
		return ERROR_FAILE;
	}

	return ERROR_SUCCESS;
}

/*****************************************************************************
 Prototype    : Dbg_Thread_SendEventToDbgThread
 Description  : 向DBG线程发送 DBG信息，和其他信息
 Input        : IN CHAR *pcString                             
                IN DBG_THRD_PRINTF_FLAG_E enDbgThrdPrintFlag  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/9/16
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
extern INT g_iDbgThreadEventFd;
extern DBG_THRD_PRINTF_FLAG_E g_enDbgThreadPrintFlag;

INT Dbg_Thread_SendEventToDbgThread(IN DBG_THRD_PRINTF_FLAG_E enDbgThrdPrintFlag)
{
    INT    iRet      = 0;
	INT    iDbgThreadEvFd;
    UINT64 uiEvtData = 1; /* 必须有值(否则目标线程没有事件)，但是 eventfd write read 的 UINT64 之间的关系不明白 */

	iDbgThreadEvFd = Dbg_Thread_GetDbgThreadEventFd();
	if (-1 == iDbgThreadEvFd)
	{
		DBGERR_PRINTF("Dbg Thread Event Fd is invalid!");
		return -1;
	}
	
    /* 向目标线程发通知 */
    iRet = write(Dbg_Thread_GetDbgThreadEventFd(), 
                 &uiEvtData, 
                 sizeof(uiEvtData));
    if (0 > iRet)
    {
		DBGERR_PRINTF("QueMSg EVENT write Failed!");
		return -1;
    }

	//TODO:set before write ^
	Dbg_Thread_SetDbgThreadPrintFlag(enDbgThrdPrintFlag);

    return 0;
}

/* 创建打印数据结构，并初始化 */
STATIC inline DBG_PRINT_BUFFER_S * debug_CreatePrintData(IN DBG_TYPE_E  enDbgType, 
 														 IN const CHAR *pcDbgFileName,
								 					     IN const CHAR *pcDbgFunName, 
								 					     IN ULONG       ulDbgLine,
                                                         IN INT         iModID,
								 					     IN INT         iStrLen,
								 					     IN const CHAR *pString)
{
	DBG_PRINT_BUFFER_S *pstDbgPrintBuf = NULL;
	
	pstDbgPrintBuf = (DBG_PRINT_BUFFER_S *)malloc(sizeof(DBG_PRINT_BUFFER_S));
	if (NULL == pstDbgPrintBuf)
	{
		DBGERR_PRINTF("malloc Failed");
		
		return NULL;
	}
	memset(pstDbgPrintBuf, 0 ,sizeof(DBG_PRINT_BUFFER_S));
	
	pstDbgPrintBuf->pcString = (CHAR *)malloc(iStrLen); /* 这里+1，就是因为vsnprintf的返回值没有包括'\0' */
	if (NULL == pstDbgPrintBuf->pcString)
	{
		DBGERR_PRINTF("malloc Failed");
		free(pstDbgPrintBuf);
		
		return NULL;
	}
	memset(pstDbgPrintBuf->pcString, 0, iStrLen);

	pstDbgPrintBuf->ulDbgSeq      = get_NsDbg_seq();
	strncpy(pstDbgPrintBuf->pcString, pString, iStrLen);
	pstDbgPrintBuf->pcDbgFunName  = pcDbgFunName;
	pstDbgPrintBuf->pcDbgFileName = pcDbgFileName;
    pstDbgPrintBuf->iModID        = iModID,
	pstDbgPrintBuf->iDbgLine	  = ulDbgLine;
	pstDbgPrintBuf->enDbgType	  = enDbgType;
	
	return pstDbgPrintBuf;
}

/* 释放打印数据结构 */
inline VOID Debug_DestroyPrintData(IN DBG_PRINT_BUFFER_S *pstDbgPrintBuf)
{
	if (NULL !=pstDbgPrintBuf && NULL != pstDbgPrintBuf->pcString)
	{
		free(pstDbgPrintBuf->pcString);
		free(pstDbgPrintBuf);
	}

	return;
}


#define DBGBUF_MAX (511)
VOID inline Dbg_print(IN DBG_TYPE_E enDbgType, 
                      IN INT   iModID,
					  IN const CHAR *pcDbgFileName,
 					  IN const CHAR *pcDbgFunName, 
 					  IN ULONG ulDbgLine,
 					  IN const char *fmt, ...)
{
	INT     iRet;
	INT     iStrLen = 0;
	/* 多次copy影响性能，但会节省memory */
	CHAR    szString[DBGBUF_MAX + 1];
	DBG_PRINT_BUFFER_S *pstDbgPrintBuf = NULL;
    va_list args;
	 
    va_start(args, fmt);

    memset(szString, 0, DBGBUF_MAX + 1);

	/*
		The functions snprintf() and vsnprintf() do not write  more  than  size
		bytes  (including the terminating null byte ('\0')).  If the output was
		truncated due to this limit then the return	value  is  the	number	of
		characters  (excluding the terminating null byte) which would have been
		written to the final string if enough space had been available.	 Thus,
		a  return  value  of  size or more means that the output was truncated.
		(See also below under NOTES.)

		vsnprintf()会吧'\0'带上，但返回值不包括'\0',奶奶的。
	*/
	iRet = vsnprintf(szString, DBGBUF_MAX, fmt, args);
	if (iRet < 0)
	{
		DBGERR_PRINTF("vsnprintf Failed!");
		return;
	}

	/* 补充长度 */
	iStrLen = iRet + 1;

	/* 创建打印数据结构 */
	pstDbgPrintBuf = debug_CreatePrintData(enDbgType,
 										   pcDbgFileName,
										   pcDbgFunName,
										   ulDbgLine,
                                           iModID,
										   iStrLen,
										   szString);
	if (NULL == pstDbgPrintBuf)
	{
		DBGERR_PRINTF("Create Print Data Failed!\n");
        return;
	}

#ifndef NS_EVENTFD
	/* debug 线程没有准备好，不会处理Degug的，直接打印或输出 */
	/* 说以在远程或文件中的Debug信息会少一些，
	   这些需要在当前的设备终端在会可以看得到，
	   可以加个缓冲链表，在Debug线程启动前线缓冲一下
	   */
	if (BOOL_TRUE == DBG_thread_IsRun())
	{	
		if (BOOL_TRUE == Dbg_Thread_ISDebugThread())
		{
			/* 如果为debug 线程 则直接打印发送什么 */
			Dbg_process_ByFriend(pstDbgPrintBuf);
			Debug_DestroyPrintData(pstDbgPrintBuf);
		}
		else
		{
			/* 写入链表交由debeg线程，并由debug线程释放 */
			Dbg_thread_WriteDbgMsgList(pstDbgPrintBuf);
			Dbg_Thread_SendEventToDbgThread(DBG_TPF_STRING);
		}
	}
	else
	{
#endif
		/* 打印输出 释放结构 */
		//printf("[*]"); /* 不交由Debug线程，直接打印。 */
		DBG_print_ByType(pstDbgPrintBuf);
		Debug_DestroyPrintData(pstDbgPrintBuf);
		
		//DBGERR_PRINTF("**** DBG thrad is not ready!  ****\n");
#ifndef NS_EVENTFD
	}
#endif

    va_end(args);

	return;
 }

#define PROCP_DIV (50)
 unsigned int DBG_Print_Rate_of_Progress(unsigned int now, unsigned int total)
 {	 
	 unsigned int uiLoop;
	 unsigned char bCh = 0;
	 unsigned int uiProcDiv = PROCP_DIV;
	 unsigned char ucProgressBar = 0;			/* 运行了的百分比色块数 =(色块个数 X uiProcDiv) */
	 unsigned char ucProgressBar_A[100] = {0};	/* 运行了的的色块 */
	 unsigned char ucProgressBar_N[100] = {0};  /* 未运行的的色块 */
	 float fProcDivw = 0;	/* 每一格代表的百分数(100 / 色块个数) */
	 float fProcDRunD = 0;	/* 当前的进程率 =(now/total) */

	 if (now > total)
	 {
	 	DBGERR_PRINTF("now > total, i think you Are wrong!\n");
		return 0;
	 }
 
	 if (total > PROCP_DIV)
	 {
		 if (0 != (now % (total / uiProcDiv)))
			 return 0;
	 }
	 
	 fProcDRunD = ((float)now)	 / ((float)total);
	 
	 for(uiLoop = 1; uiLoop <=	PROCP_DIV; uiLoop++)
	 {
		 if (uiLoop == (unsigned int)round(fProcDRunD * PROCP_DIV))
		 {
			 ucProgressBar = uiLoop;
			 bCh = 1;
		 }
		 
		 if (0 == bCh)
		 {
			 ucProgressBar_A[uiLoop - 1] = ' ';
		 }
		 else
		 {
			 ucProgressBar_N[uiLoop - 1 - ucProgressBar] = ' ';
		 }
	 }
	 fProcDivw = 100 / (float)uiProcDiv;
 
	 printf("\r \033[0;42m%s\033[0;43m%s\033[0m [%3d%%]", 
				 ucProgressBar_A,
				 ucProgressBar_N,
				 (unsigned int)round(ucProgressBar * fProcDivw)
				 );
	 fflush(stdout);
 
	 return now;
 
 }

#define NS_LOG_DIR "log"
void Dbg_Print_Log(const char *pcFileName, char *pcChar, unsigned long ulLen)
{												
	int iRet = 0;
	int dbg_fd = 0;		
	char szFileDir[256] = {0};

	iRet = access(NS_LOG_DIR, F_OK | R_OK | W_OK | X_OK);
	if (0 != iRet) {
		iRet = mkdir(NS_LOG_DIR, 0775);
		if (0 != iRet) {
			printf("mkdir [%s] failed.\n", NS_LOG_DIR);
			return;
		}
	}

	sprintf(szFileDir, "log//%s", pcFileName);
	dbg_fd = open((szFileDir), O_CREAT|O_TRUNC|O_RDWR, 664);
	if(-1 == dbg_fd)							
	{										
		ERR_PRINTF("open log file failed!");
	}										
	write(dbg_fd, (pcChar), (ulLen));		
	close(dbg_fd);							
}	

#ifdef __THREAD_DEBUG__
#endif

#define  THRD_NAME_LIST_MAX (256)
#define  THRD_NAME_LEN_MAX  (32)
STATIC CHAR paucThreadNameList[THRD_NAME_LIST_MAX][THRD_NAME_LEN_MAX + 1] = {{0}};
/*****************************************************************************
 Prototype    : Dbg_ThrdNameReg
 Description  : 吧线程名字和线程ID成对存入数组
 Input        : INT iThrdId       
                CHAR *pcThrdName  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/9/11
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
VOID Dbg_ThrdNameReg(INT iThrdId, CHAR *pcThrdNameFmt, ...)
{
	va_list args;
	
	DBGASSERT(NULL != pcThrdNameFmt);

	if (iThrdId > THRD_NAME_LIST_MAX)
	{
		ERR_PRINTF("Debug Reg Thread name Failed!\n");
		return;
	}

	/*
		int vsprintf(char *str, const char *format, va_list ap);
		int vsnprintf(char *str, size_t size, const char *format, va_list ap);
		
		The functions snprintf() and vsnprintf() do not write  more  than  size
		bytes  (including the terminating null byte ('\0')).
	*/
	va_start(args, pcThrdNameFmt);
	
	vsprintf(paucThreadNameList[iThrdId], pcThrdNameFmt, args);
	
	va_end(args);

	return;
}

/*****************************************************************************
 Prototype    : Dbg_ThrdNameGet
 Description  : 根据线程ID获取线程名字
 Input        : INT iThrdId  
 Output       : None
 Return Value : CHAR
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2014/9/11
    Author       : langyanjun
    Modification : Created function

*****************************************************************************/
CHAR *Dbg_ThrdNameGet(INT iThrdId)
{
	CHAR *pcThrdName = NULL;
	
	if (iThrdId > THRD_NAME_LIST_MAX)
	{
		ERR_PRINTF("Debug Get Thread name Failed!\n");
		return NULL;
	}
	
	pcThrdName = paucThreadNameList[iThrdId];
	if (NULL == pcThrdName)
	{
		return "unKnow-name";
	}

	return pcThrdName;
}

#endif //__DEBUG_C__
