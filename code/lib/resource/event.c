#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ns_base.h>

#include <ns_lilist.h>
#include <ns_bitmap.h>
#include <ns_table.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_thread.h>
#include <ns_event.h>

#define EVENT_READ_FD_TO_PIPE_ST(iReadFd) \
    container_of(&iReadFd, EVT_PIPE_FDS_ST, iReadFd)

STATIC DCL_HEAD_S g_stEvtFdListHead;

INT EVENT_Create(VOID)
{
    INT   iFds[2];
    INT   iRet  = 0;
    EVT_PIPE_FDS_ST *pstEvtFds = NULL;

    pstEvtFds = malloc(sizeof(EVT_PIPE_FDS_ST));
    if (NULL == pstEvtFds)
    {
        ERR_PRINTF("malloc failed.");
        return ERROR_FAILE;
    }
    memset(pstEvtFds, 0, sizeof(EVT_PIPE_FDS_ST));

    iRet = pipe(iFds);
    if (iRet < 0) {
        free(pstEvtFds);
        pstEvtFds = NULL;
        ERR_PRINTF("pipe failed.");
        return ERROR_FAILE;
    }

    //pstEvtFds = (EVT_PIPE_FDS_ST*)iFds;
    pstEvtFds->iReadFd  = iFds[0];
    pstEvtFds->iWriteFd = iFds[1];

    DCL_AddTail(&g_stEvtFdListHead, &(pstEvtFds->stNode));

	return pstEvtFds->iReadFd;
}

EVT_PIPE_FDS_ST *EVENT_pipe_fds_FindByReadFd(IN INT iReadFd)
{
    EVT_PIPE_FDS_ST *pstEvtFds = NULL;

    DCL_FOREACH_ENTRY(&g_stEvtFdListHead, pstEvtFds, stNode) {
        if (pstEvtFds->iReadFd == iReadFd) {
            break;
        }
    }

    return pstEvtFds;
}

EVT_PIPE_FDS_ST *EVENT_pipe_fds_FindByWriteFd(IN INT iWriteFd)
{
    EVT_PIPE_FDS_ST *pstEvtFds = NULL;

    DCL_FOREACH_ENTRY(&g_stEvtFdListHead, pstEvtFds, stNode) {
        if (pstEvtFds->iWriteFd== iWriteFd) {
            break;
        }
    }

    return pstEvtFds;
}

VOID EVENT_Destroy(IN INT iReadFd) 
{
    EVT_PIPE_FDS_ST *pstEvtFds = NULL;

    pstEvtFds = EVENT_pipe_fds_FindByReadFd(iReadFd);
    if (pstEvtFds == NULL) {
        ERR_PRINTF("INVALID Read FD");
        return;
    }
    if (pstEvtFds->iReadFd > 0) {
        close(pstEvtFds->iReadFd);
        pstEvtFds->iReadFd = -1;
    }
    if (pstEvtFds->iWriteFd> 0) {
        close(pstEvtFds->iWriteFd);
        pstEvtFds->iWriteFd = -1;
    }

    return;
}

INT INLINE EVENT_ReadFD_2_WriteFd(IN INT iReadFd)
{
    EVT_PIPE_FDS_ST *pstEvtFds = NULL;

    pstEvtFds = EVENT_pipe_fds_FindByReadFd(iReadFd);
    if (pstEvtFds == NULL) {
        ERR_PRINTF("INVALID Read FD");
        return -1;
    }

    return pstEvtFds->iWriteFd;
}

INT INLINE EVENT_WriteFD_2_ReadFd(IN INT iWriteFd)
{
    EVT_PIPE_FDS_ST *pstEvtFds = NULL;

    pstEvtFds = EVENT_pipe_fds_FindByWriteFd(iWriteFd);
    if (pstEvtFds == NULL) {
        ERR_PRINTF("INVALID Read FD");
        return -1;
    }

    return pstEvtFds->iReadFd;
}

ULONG EVENT_SendEvt(IN INT iWriteFd)
{
    ULONG ulRet = ERROR_FAILE;
    UINT64 uiEvtData = 0xBB;

    ulRet = write(iWriteFd, 
                &uiEvtData, 
                sizeof(uiEvtData));
    if (0 > ulRet)
    {
		ERR_PRINTF("QueMSg EVENT write Failed!");
		//THREAD_quemsg_Free(0);/* 未实现 */
		ulRet = ERROR_FAILE;
    }

    return ulRet;
}

ULONG EVENT_fd_list_Init(void)
{
    DCL_Init(&g_stEvtFdListHead);
    
    return ERROR_SUCCESS;
}
