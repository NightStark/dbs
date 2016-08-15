#ifndef __PROCESS_H__
#define __PROCESS_H__

#define GET_PNAME_BUF_LEN (64)
#define GET_PID_BUF_LEN (16)
#define DBG_PROC_ONLY_FILE			"/tmp/debug.pid"
#define DBG_PROC_ONLY_BUF_SIZE     (32)

INT P_GetPID(const CHAR *ProcessName);

#endif//__PROCESS_H__
