/***************************************************************************
 * file  : postman.c
 * author: lang.yanjun@tianyuantechnology.com
 * description: sharm memory api for process lock and data sync
 * data  : 2017-5-25 16:00
 ***************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/resource.h>

#ifdef __LOG
#undef __LOG
#endif
#define __LOG(level, fmt, ...) \
    do { \
        printf("[%s][%s][%d]"fmt"\n" , "shlock", __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define SHLOCK_NAME_SIZE      (32)
#define SHLOCK_SHMEM_KEY_BASE (200)

typedef struct shlock_obj {
	int index;
	char name[SHLOCK_NAME_SIZE];
	int shmid;
	int shmsize;
	int semid;
	int semn;
	int cnt;
	void *shmaddr;
}SHLOCK_INFO_ST;

pthread_mutex_t  g_shlock_op_mutex = PTHREAD_MUTEX_INITIALIZER;
#define SHLOACK_OP_LOCK \
    pthread_mutex_lock(&g_shlock_op_mutex)
#define SHLOACK_OP_UNLOCK \
    pthread_mutex_unlock(&g_shlock_op_mutex)

SHLOCK_INFO_ST g_ns_shlock_info;


int shlock_create(SHLOCK_INFO_ST *shlock)
{
	struct shmid_ds shm;
	int key = SHLOCK_SHMEM_KEY_BASE + shlock->index;

    SHLOACK_OP_LOCK;
    shlock->shmid = shmget(key, shlock->shmsize, (SHM_W | SHM_R | IPC_EXCL | IPC_CREAT));
    if (shlock->shmid < 0) {
        if (errno == EEXIST) {
            __LOG(LOG_DEBUG, "shmget is already exist.");
            shlock->shmid = shmget(key, shlock->shmsize, (SHM_W | SHM_R | IPC_EXCL));
            if (shlock->shmid < 0) {
                __LOG(LOG_ERR, "shmget exist failed.");
                goto error;
            }
            shmctl(shlock->shmid, IPC_STAT, &shm);
            if (shlock->shmsize != shm.shm_segsz) {
                __LOG(LOG_ERR, "shmget exist size is not same to us.");
                goto error;
            }
        } else {
            __LOG(LOG_ERR, "shmget failed.");
            perror("shmget:");
            goto error;
        }
    }

    shlock->semid = semget(key, shlock->semn, (0666 | IPC_EXCL | IPC_CREAT));
    if (shlock->semid < 0) {
        if (errno == EEXIST) {
            __LOG(LOG_ERR, "semget alreay exist.");
            shlock->semid = semget(key, shlock->semn, (0666 | IPC_EXCL));
            if (shlock->semid < 0) {
                __LOG(LOG_ERR, "semget failed.");
                perror("semget:");
                shmctl(shlock->shmid, IPC_RMID, &shm);
                shlock->shmid = -1;
                goto error;
            }
        } else {
            __LOG(LOG_ERR, "semget failed.");
            perror("semget:");
			shmctl(shlock->shmid, IPC_RMID, &shm);
			shlock->shmid = -1;
            goto error;
        }
    } else {
        if (semctl(shlock->semid, 0, SETVAL, 1) < 0) {
            __LOG(LOG_ERR, "semctl failed.");
        }
    }

    shlock->cnt = 0;

    SHLOACK_OP_UNLOCK;

    __LOG(LOG_DEBUG, "shmid = %d", shlock->shmid);
    __LOG(LOG_DEBUG, "semid = %d", shlock->semid);

    return 0;

error:
    //TODO release shmget semget!!??

    SHLOACK_OP_UNLOCK;
    return -1;
}
int _shlock_at(SHLOCK_INFO_ST *shlock)
{
	int shmflg = 0;
	struct sembuf sem;

    if (shlock->shmid < 0) {
        __LOG(LOG_ERR, "shmid is invalid.");
        return -1;
    }
    if (shlock->semid < 0) {
        __LOG(LOG_ERR, "semid is invalid.");
        return -1;
    }

	sem.sem_num = 0;
	sem.sem_op = -1;
	sem.sem_flg = 0;
	if(semop(shlock->semid, &sem, 1) < 0) {
        __LOG(LOG_ERR, "%s: set semaphore error(%d)!\n",
                __func__, errno);
        goto error;
	}
    shlock->shmaddr = shmat(shlock->shmid, NULL, shmflg);
	if (shlock->shmaddr == NULL) {
		__LOG(LOG_ERR, "%s: May shared memory error(%d)!\n",
			__func__, errno);
		sem.sem_num = 0;
		sem.sem_op = 1;
		sem.sem_flg = 0;
		semop(shlock->semid, &sem, 1);
		goto error;
	}

    return 0;

error:
    return -1;
}

/* attaches share memory */
int shlock_at(SHLOCK_INFO_ST *shlock)
{
    int ret = -1;

    ret = semctl(shlock->semid, 0, GETVAL, 0);
    __LOG(LOG_DEBUG, ">>>>>>>>>>>>> v = %d", ret);
    SHLOACK_OP_LOCK;
    if (shlock->cnt == 0) {
        ret = _shlock_at(shlock);
    }
    shlock->cnt++;
    SHLOACK_OP_UNLOCK;
    ret = semctl(shlock->semid, 0, GETVAL, 0);
    __LOG(LOG_DEBUG, "<<<<<<<<<<<<< v = %d", ret);

    return ret;
}

int _shlock_dt(SHLOCK_INFO_ST *shlock)
{
    struct sembuf sem;

    if (shlock->shmaddr) {
        shmdt(shlock->shmaddr);
        shlock->shmaddr = NULL;
    }

    if (shlock->semid >= 0) {
        sem.sem_num = 0;
        sem.sem_op  = 1;
        sem.sem_flg = 0;

        if (semop(shlock->semid, &sem, 1) < 0) {
            __LOG(LOG_ERR, "semop failed.");
            perror("semod failed");
            return -1;
        }
    }
    
    return 0;
}

/* detaches share memory */
int shlock_dt(SHLOCK_INFO_ST *shlock)
{
    int ret = -1;

    ret = semctl(shlock->semid, 0, GETVAL, 0);
    __LOG(LOG_DEBUG, ">>>>>>>>>>>>> v = %d", ret);
    SHLOACK_OP_LOCK;
    if (shlock->cnt == 1) {
        ret = _shlock_dt(shlock);
        shlock->cnt = 0;
    } else if (shlock->cnt > 1) {
        shlock->cnt--;
    } else {
        __LOG(LOG_ERR, "error.");
    }
    SHLOACK_OP_UNLOCK;
    ret = semctl(shlock->semid, 0, GETVAL, 0);
    __LOG(LOG_DEBUG, "<<<<<<<<<<<<< v = %d", ret);

    return ret;
}

int shlock_init(int size)
{
    memset(&g_ns_shlock_info, 0, sizeof(SHLOCK_INFO_ST));
    g_ns_shlock_info.semn = 1; /* only use one sem */
    g_ns_shlock_info.shmsize = size;

    shlock_create(&g_ns_shlock_info);


    return 0;
}

int shlock_destroy(SHLOCK_INFO_ST *shlock)
{
    struct shmid_ds shm;

    if (shlock->shmid >= 0) {
        if (shmctl(shlock->shmid, IPC_RMID, &shm) < 0) {
            __LOG(LOG_ERR, "remove share memory failed.");
            return -1;
        }
        shlock->shmid = -1;
    }

    if (shlock->semid >= 0) {
        if (semctl(shlock->semid, 0 ,IPC_RMID, 0) < 0) {
            __LOG(LOG_ERR, "remove semaphore failed.");
            return -1;
        }
    }

    return 0;
}

int      
file_close_fds(void)                             
{
    struct rlimit lim;
    unsigned int i;

    if (getrlimit(RLIMIT_NOFILE, &lim) < 0)
        return -1;

    if (lim.rlim_cur == RLIM_INFINITY)
        lim.rlim_cur = 1024;

    printf("cur = %ld\n", lim.rlim_cur);
    for (i = 3; i < lim.rlim_cur; i++) {
        if (close(i) < 0 && errno != EBADF) {
            return -1;
        }
    }

    return 0;
}

int main(void)
{
    char *s = NULL;
    shlock_init(128);
    shlock_at(&g_ns_shlock_info);

    s = g_ns_shlock_info.shmaddr;
    printf("s = 0x%X\n", s);
    if (s[0] != 'h') {
        snprintf(s, 128, "%s", "hello");
    } else {
        printf("VVVV = %s\n", s);
    }
    sleep(5);

    shlock_dt(&g_ns_shlock_info);
    //shlock_destroy(&g_ns_shlock_info);

    return 0;
}
