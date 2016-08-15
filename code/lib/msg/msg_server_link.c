#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <ns_base.h>
#include <ns_sm.h>
#include <ns_lilist.h>
#include <ns_msg_server_link.h>


/*************************************************
 * MSG_SRV_LINK_ST: message server link list
 *   is list at server to keep link infomation of
 *   clients. and socket to fsm  .and so on
 *************************************************/

STATIC DCL_HEAD_S g_stMsgSrvLinkListHead;
pthread_mutex_t g_MstSrvLinkList_Mutex;

STATIC INT msg_server_link_createALinkId(VOID)
{
    /* TODO:may be need a better way to create it */
    static int id = 0;
    return id++;
}

MSG_SRV_LINK_ST * MSG_server_link_Create(IN INT iSockFd,
                                         IN INT iThrdID)
{
    MSG_SRV_LINK_ST *stSrvLink = NULL;

    stSrvLink = malloc(sizeof(MSG_SRV_LINK_ST));
    if (NULL == stSrvLink) 
    {
        ERR_PRINTF("malloc failed.");
        return NULL;
    }
    memset(stSrvLink, 0, sizeof(MSG_SRV_LINK_ST));

    stSrvLink->iSockFd   = iSockFd;
    stSrvLink->iThreadID = iThrdID;
    stSrvLink->uiLinkID  = msg_server_link_createALinkId();

    return stSrvLink;
}

VOID MSG_server_link_Destroy (MSG_SRV_LINK_ST *stSrvLink)
{
    if (NULL != stSrvLink) 
    {
        free(stSrvLink);
        stSrvLink = NULL;
    }
    
    return;
}

INT MSG_server_link_Add2List(MSG_SRV_LINK_ST *stSrvLInk)
{
    DBGASSERT(NULL != stSrvLInk);

    pthread_mutex_lock(&g_MstSrvLinkList_Mutex);
    DCL_AddHead(&g_stMsgSrvLinkListHead, &(stSrvLInk->stNode));
    pthread_mutex_unlock(&g_MstSrvLinkList_Mutex);
    
    return 0;
}

MSG_SRV_LINK_ST * MSG_server_link_FindBySockFd(IN INT iSockFd)
{
    MSG_SRV_LINK_ST *pstSrvLink = NULL;

    pthread_mutex_lock(&g_MstSrvLinkList_Mutex);
    DCL_FOREACH_ENTRY(&g_stMsgSrvLinkListHead, pstSrvLink, stNode)
    {
        if (iSockFd == pstSrvLink->iSockFd) {
            break;
        }
    }
    pthread_mutex_unlock(&g_MstSrvLinkList_Mutex);

    return pstSrvLink;
}

INT MSG_server_GetSockFdBySm(IN SRV_SM_ST *pstSrvSm)
{
    MSG_SRV_LINK_ST *pstSrvLink = NULL;

    DBGASSERT(NULL != pstSrvSm);
    
    //pstSrvLink = container_of((VOID **)(&pstSrvSm), MSG_SRV_LINK_ST, pstSrvSm);
    pstSrvLink = (MSG_SRV_LINK_ST *)(pstSrvSm->pLink);

    return pstSrvLink->iSockFd;
}

INT MSG_server_link_DelFList(MSG_SRV_LINK_ST *stSrvLInk)
{
    DBGASSERT(NULL != stSrvLInk);

    pthread_mutex_lock(&g_MstSrvLinkList_Mutex);
    DCL_Del(&(stSrvLInk->stNode));
    pthread_mutex_unlock(&g_MstSrvLinkList_Mutex);
    
    return 0;
}

/*
 * create link, add into link list, start SM
 * */
MSG_SRV_LINK_ST * MSG_server_link_Up(IN INT iSockFd,
                                     IN INT iThrdID)
{
    SRV_SM_ST *pstSrvSm = NULL;
    MSG_SRV_LINK_ST *pstSrvLink = NULL;

    pstSrvLink = MSG_server_link_Create(iSockFd, iThrdID);
    if (NULL == pstSrvLink)
    {
        ERR_PRINTF("creare link failed.");
        return NULL;
    }

    /* cread and start a fsm */
    pstSrvSm = SRV_sm_CreateAndStart();
    if (NULL == pstSrvSm) 
    {
        ERR_PRINTF("create and start server sm failed.");
        return NULL;
    }

    pstSrvLink->pstSrvSm = (VOID *)pstSrvSm;
    pstSrvSm->pLink = pstSrvLink;

    SRV_SM_STATS_CHANGE(pstSrvSm, SRV_SM_STATS_INIT);

    MSG_server_link_Add2List(pstSrvLink);

    SRV_SM_STATS_CHANGE(pstSrvLink->pstSrvSm, SRV_SM_STATS_IDEL);


    return pstSrvLink;
}

INT MSG_server_link_Down(IN INT iSockFd)
{
    MSG_SRV_LINK_ST *pstSrvLink = NULL;

    pstSrvLink = MSG_server_link_FindBySockFd(iSockFd);
    if (NULL == pstSrvLink) {
        WARN_PRINTF("down a not exist sm.");
        return 0;
    }

    //TODO: free all data? keey for little time?
    SRV_sm_Destroy((SRV_SM_ST *)(pstSrvLink->pstSrvSm));
    pstSrvLink->pstSrvSm = NULL;

    MSG_server_link_DelFList(pstSrvLink);
    MSG_server_link_Destroy(pstSrvLink);
    pstSrvLink = NULL;

    return 0;
}

INT MSG_server_link_Init(VOID)
{
    DCL_Init(&g_stMsgSrvLinkListHead);
    pthread_mutex_init(&g_MstSrvLinkList_Mutex, NULL);

    return 0;
}

VOID MSG_server_link_Fini(VOID)
{
    //TODO:foreach entry safe have bug.need it to free all
    //DCL_FOREACH_ENTRY_SAFE
    DCL_Init(&g_stMsgSrvLinkListHead);

    return;
}
