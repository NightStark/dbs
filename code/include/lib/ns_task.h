#ifndef __NS_TASK_H__
#define __NS_TASK_H__

typedef ULONG (* pfTaskFunc)(VOID *args /* ==> (NS_TASK_INFO *) */);

typedef struct tag_ns_task_info
{
    DCL_NODE_S stNodeTask; /* for task to manage */
    DCL_NODE_S stNodeThrd; /* for work thread to manage */

    UINT uiTaskId;
    pfTaskFunc pfTask;
    ULONG ulArgs[4];
    ULONG ulRetVal;

    UINT uiThrdId; /* which thread is RUN */
}NS_TASK_INFO;

typedef struct tag_thrd_quemsg_data_task_dispatch
{
    UINT uiTaskId;
}THRD_QUEMSG_DATA_TASK_DISPATCH_S;

ULONG Server_Task_Create(pfTaskFunc pfTask, VOID *pArgs);
ULONG Server_Task_Handle(VOID *pQueMsgData);

#endif //__NS_TASK_H__
