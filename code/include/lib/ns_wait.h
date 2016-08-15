/******************************************************************************

  Copyright (C), 2013-2014, Night Stark. langyanjun.

 ******************************************************************************
  File Name     : ns_wait.h
  Version       : Initial Draft
  Author        : langyanjun
  Created       : 2014/8/30
  Last Modified :
  Description   : Wait Conditon
  Function List :
  History       :
  1.Date        : 2014/8/30
    Author      : langyanjun
    Modification: Created file

******************************************************************************/
#ifndef __WAIT_H__
#define __WAIT_H__

ULONG THREAD_wait_CondInit(VOID);
ULONG THREAD_wait_CondFint(VOID);
ULONG THREAD_wait(VOID *pData, EPOLL_WATI_WAKEUP_DO pfEpWaitWakeupDo);
ULONG THREAD_wait_wakeup(VOID *pData, EPOLL_WAIT_SIG_DO pfEpWaitSigDo);

#endif //__WAIT_H__
