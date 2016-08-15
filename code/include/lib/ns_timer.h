#ifndef __TIMER_H__
#define __TIMER_H__

typedef ULONG (*timerout_callback_t)(INT /* timer id */, VOID * /* para: UINT x 4 */);

typedef struct tagTimerData /* 待升级为链表 */
{
	INT iTimerID;
	UINT auiPara[4];
	timerout_callback_t pfTmOutCB;
}TIMER_DATA_S;

INT   Timer_Create(IN INT iSec);
ULONG TIMER_DeleteAllForThread(IN INT iThreadId);
ULONG TIMER_DeleteForThread(IN INT iThreadId, IN INT iTimerId);
INT   TIMER_CreateForThread(IN INT iThreadId, /* 待整合 */
                            IN INT iMSec, 
                            IN timerout_callback_t pfTmOutCB,
                            IN VOID *pPara /* len = 4 个 字 */);
ULONG TIMER_thread_init (VOID);
VOID TIMER_thread_Fini (VOID);
INLINE ULLONG TIMER_get_BootTime(VOID);
UINT INLINE TIMER_info_ReadFdisTimerFd(IN INT iReadFd);

#endif //__TIMER_H__

