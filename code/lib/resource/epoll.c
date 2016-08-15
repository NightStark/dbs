#ifndef __MSG_EPOLL_C__
#define __MSG_EPOLL_C__

#include <stdio.h>  
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>
#include <sys/epoll.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <fcntl.h>  
#include <errno.h>  
#include <time.h>

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#include <ns_table.h>
#include <ns_net.h>
#include <ns_msg.h>
#include <ns_epoll.h>

// set event  
ULONG Epoll_EventSet(IN    INT fd, 
 					 IN    VOID *call_back,
 					 INOUT EPOLL_EVENTS_S *pstEpollEvents)  
{  
    pstEpollEvents->fd = fd;  
    pstEpollEvents->call_back = call_back;  
    pstEpollEvents->events = 0;  
    pstEpollEvents->arg = (VOID *)pstEpollEvents;  
    pstEpollEvents->status = 0;
    //bzero(ev.buff, sizeof(ev.buff));
    pstEpollEvents->s_offset = 0;  
    pstEpollEvents->len = 0;
    pstEpollEvents->last_active = time(NULL);  

    return ERROR_SUCCESS;
}  
// add/mod an event to epoll  
ULONG Epoll_EventAdd(INT iEpFd,
								INT events, 
								EPOLL_EVENTS_S *pstEpollEvents)  
{  
    struct epoll_event epv = {0, {0}};  
    INT                op  = 0;  

	epv.data.ptr = pstEpollEvents;  
    epv.events = pstEpollEvents->events = events;  
    
    if(pstEpollEvents->status == 1){  
        op = EPOLL_CTL_MOD;  
    }  
    else{  
        op = EPOLL_CTL_ADD;  
        pstEpollEvents->status = MSG_EPOLL_STATUS_IDEL;  
    } 
    
    if(epoll_ctl(iEpFd, op, pstEpollEvents->fd, &epv) < 0)  
    {
        ERR_PRINTF("Event Add failed[fd=%d], evnets[%d]", pstEpollEvents->fd, events);  
        return ERROR_FAILE;
    }
    else  
    {
        MSG_PRINTF("Event Add SUCCESS[fd=%d], op=%d, evnets[%0X]", pstEpollEvents->fd, op, events);  
    }

    return ERROR_SUCCESS;
}  
// delete an event from epoll  
VOID Epoll_EventDel(INT epollFd, EPOLL_EVENTS_S *ev)  
{  
    struct epoll_event epv = {0, {0}};  
    if(ev->status != MSG_EPOLL_STATUS_IDEL) return;  
    
    epv.data.ptr = ev;  
    ev->status = MSG_EPOLL_STATUS_NULL;
    epoll_ctl(epollFd, EPOLL_CTL_DEL, ev->fd, &epv);  
}  

INT Epoll_Create(INT iEpNum)
{
	INT  iEpFd  = -1;
	
	iEpFd  = epoll_create(iEpNum);
    if (-1 == iEpFd)
    {
    	perror("epoll_create");
        ERR_PRINTF("epoll_create Failed!");
    }

    return iEpFd;
}

VOID Epoll_Destroy(IN EPOLL_EVENTS_S *pstEpollEvents)
{
    free(pstEpollEvents);

    return;
}

#endif //__MSG_EPOLL_C__

