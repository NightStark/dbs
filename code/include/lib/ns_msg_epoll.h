#ifndef __MSG_EPOLL_H__
#define __MSG_EPOLL_H__

#define MSG_EPOLL_MAX_EVENTS 1024
#define MAX_EVENTS 500  


enum{
    MSG_EPOLL_STATUS_NULL = 0,
    MSG_EPOLL_STATUS_IDEL,
    MSG_EPOLL_STATUS_RUNING
};

typedef VOID (*EPOLL_CALL_BACK)(INT, INT, VOID *);
typedef struct epoll_event EPOLL_EVENT_S;

typedef struct tag_msg_events  
{  
    INT 	fd;  
    VOID   *call_back;  
    INT 	events;  
    VOID   *arg;  
    INT 	status; 		// 1: in epoll wait list, 0 not in  
    CHAR 	buff[128]; 		// recv data buffer  
    INT 	len, s_offset;  
    LONG 	last_active; 	// last active time  
}EPOLL_EVENTS_S;  

ULONG Epoll_EventSet(  IN    INT fd, 
    							 IN    VOID *call_back,
    							 INOUT EPOLL_EVENTS_S *pstEpollEvents);
ULONG Epoll_EventAdd(INT iEpFd,
								INT events, 
								EPOLL_EVENTS_S *pstEpollEvents);
VOID  Epoll_EventDel(INT epollFd, EPOLL_EVENTS_S *ev);
INT   Epoll_Create(INT iEpNum);
UINT  Epoll_Destroy(IN EPOLL_EVENTS_S *pstEpollEvents);




#endif //__MSG_EPOLL_H__

