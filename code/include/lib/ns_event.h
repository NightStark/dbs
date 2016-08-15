#ifndef __NS_EVENT_H__
#define __NS_EVENT_H__

typedef struct t_evt_pipe_fds
{
    DCL_NODE_S stNode;
    INT iReadFd;
    INT iWriteFd;
}EVT_PIPE_FDS_ST;

INT EVENT_Create(VOID);
VOID EVENT_Destroy(IN INT iReadFd);
INT INLINE EVENT_ReadFD_2_WriteFd(IN INT iReadFd);
INT INLINE EVENT_WriteFD_2_ReadFd(IN INT iWriteFd);
ULONG EVENT_fd_list_Init(void);
ULONG EVENT_SendEvt(IN INT iWriteFd);

#endif //__NS_EVENT_H__
