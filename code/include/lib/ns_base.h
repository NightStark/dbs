#ifndef __NS_BASE_H__
#define __NS_BASE_H__

#include <ns_symbol.h>
#include <ns_type.h>
#include <ns_debug.h>
#include <ns_string.h>

#ifdef NS_EVENTFD
#define  eventfd 
#endif
#ifdef NS_TIMERFD
#define timerfd_create
#endif

#endif //__NS_BASE_H__
