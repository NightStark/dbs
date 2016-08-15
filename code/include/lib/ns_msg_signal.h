#ifndef __MSG_SIGNAL_H__
#define __MSG_SIGNAL_H__

#define SIG_MSG 		(SIGUSR1)
#define DBG_ENV_PID_LEN (8)
#define DBG_ENV_PNAME 	("DEBUG")

ULONG MSG_sigthrd_GetCurrentThreadSigMask(INOUT sigset_t *pSigSet);
INT MSG_sigthrd_SigToFd(IN sigset_t *pSigMask);

#endif//__MSG_SIGNAL_H__
