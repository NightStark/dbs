#include <paths.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>

#include "ns_log.h"

#define NS_LOG_SYSLOG_NAME "nightstark"
#define NS_LOG_BLOCK_FILE "/etc/dlogblk"

static const char *ns_log_name = NS_LOG_SYSLOG_NAME;

static int ns_log_conf_fd = -1;
static void *ns_log_conf_addr = NULL;

pthread_mutex_t syslog_mutex;
static struct timeval syslog_lastm;
static int syslog_fd = -1;
static int syslog_conn = 0;
extern struct ns_log_ctl g_ns_log_ctl;


static const struct sockaddr syslog_addr = {
	.sa_family = AF_UNIX, /* sa_family_t (usually a short) */
	.sa_data = _PATH_LOG  /* char [14] */
};


static int
ns_log_open_syslog ()
{
        int log_type = SOCK_DGRAM;
	int bufsize, ret;
	socklen_t len;

	if (pthread_mutex_lock(&syslog_mutex) < 0) {
		fprintf(stderr, "Get syslog mutex error!(open)\n");
	}

        if (syslog_fd == -1) {
retry:
		if ((syslog_fd = socket(AF_UNIX, log_type, 0)) == -1) {
			goto err;
		}
		fcntl(syslog_fd, F_SETFD, 1); /* 1 == FD_CLOEXEC */
		if((access(NS_LOG_BLOCK_FILE, F_OK)) == -1) {
			/* We don't want to block if e.g. syslogd is SIGSTOPed */
			fcntl(syslog_fd, F_SETFL, O_NONBLOCK | fcntl(syslog_fd, F_GETFL));
		}
		
		len = sizeof(bufsize);
		bufsize = 1024 * 1024;
		ret = setsockopt(syslog_fd, SOL_SOCKET, SO_SNDBUF, &bufsize, len);
		if (ret != 0) {
			perror("setsocket error.");
		}
        }

        if (syslog_fd != -1 && !syslog_conn) {
                if (connect(syslog_fd, &syslog_addr, sizeof(syslog_addr)) != -1) {
                        syslog_conn = 1;
                } else {
                        if (syslog_fd != -1) {
                                close(syslog_fd);
                                syslog_fd = -1;
                        }
                        if (log_type == SOCK_DGRAM) {
                                log_type = SOCK_STREAM;
                                goto retry;
                        } else {
				goto err;
			}
                }
        }
	pthread_mutex_unlock(&syslog_mutex);
	return 0;
err:
	pthread_mutex_unlock(&syslog_mutex);
	return -1;
}

static void
ns_log_close_syslog ()
{
	if (pthread_mutex_lock(&syslog_mutex) < 0) {
		fprintf(stderr, "Get syslog mutex error!(close)\n");
	}
	if (syslog_fd != -1) {
		(void) close(syslog_fd);
	}
	syslog_fd = -1;
	syslog_conn = 0;
	pthread_mutex_unlock(&syslog_mutex);
}

static void
ns_log_print_syslog (char *buf, int len)
{
	int rc;
	char *p = buf;

	if (syslog_fd < 0) {
		if (ns_log_name) {
			struct timespec ts;
			struct timeval now, tm;

			if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
				return;
			now.tv_sec = ts.tv_sec;
			now.tv_usec = ts.tv_nsec / 1000; 

			timersub(&now, &syslog_lastm, &tm);
			if (tm.tv_sec > 2) {
				syslog_lastm = now;
				if (ns_log_open_syslog()) {
					return;
				}
			} else {
				return;
			}
		} else {
			return;
		}
	}
	do {
		rc = write(syslog_fd, p, len);
		if (rc < 0) {/* Maybe syslogd has gone. */
			ns_log_close_syslog();
			return;
		}
		len -= rc;
		p += rc;
	} while (len > 0);
}



int
ns_log_open_conf (int rw, int *fd, void **addr)
{
	int fflag, mflag;
	unsigned char c;

	if (*fd > 0)
		return 0;

	if (rw) {
		fflag = O_CREAT|O_RDWR;
		mflag = PROT_READ|PROT_WRITE;
	} else {
		fflag = O_CREAT|O_RDONLY;
		mflag = PROT_READ;
	}

	*fd = open(NS_LOG_CONF_FILE, fflag, 644);
	if (*fd < 0) {
		fprintf(stderr, "Open dlog config file error! %d\n", errno);
		return -1;
	}
	if (rw) {/* FIXME */
		lseek(*fd, g_ns_log_ctl.size, SEEK_SET);
		c = 0xff;
		write(*fd, &c, sizeof(c));
	}

	*addr = mmap(NULL, g_ns_log_ctl.size, mflag, MAP_SHARED,
			*fd, 0);//NS_LOG_CONF_FILE_OFFSET);
	if (*addr == MAP_FAILED) {
		fprintf(stderr, "Map dlog config file error! %d\n", errno);
		close(*fd);
		*fd = -1;
		return -1;
	}

	return 0;
}

void
ns_log_close_conf (int *fd, void **addr)
{
	if (*addr) {
		munmap(*addr, g_ns_log_ctl.size);
	}
	*addr = NULL;

	if (*fd) {
		close(*fd);
	}
	*fd = -1;
}

int
ns_log_commit_conf (void *addr, int len)
{
	return msync(addr, len, MS_SYNC);
}

static int
ns_log_mod(int id, int level)
{
	int type = id >> 16;
	
	if (ns_log_conf_addr == NULL) {
		return -1;
	}

	if (type >= g_ns_log_ctl.n) {
		return -1;
	}
	if (g_ns_log_ctl.f[type].func == NULL) {
		type = 0;
	}
	return g_ns_log_ctl.f[type].func(ns_log_conf_addr + g_ns_log_ctl.f[type].start,
			id&~NS_LOG_TYPE_MASK, level);
}

int
ns_log_mx(int id, int level, const char *fmt, ...)
{
    va_list argptr;
    char buff[1024];
    char *p, *end, *head_end, *last_chr;
    time_t now;
    int mask, pri, fd;

    if ((mask = ns_log_mod(id, level)) == 0) {
        return 0;
    } else if (mask < 0) {
        /* When there has error, print log to stderr. */
        mask = NS_LOG_TO_STDOUT;
    }

    if (NS_LOG_MOD(id) == NS_LOG_MOD_KERNEL) {
        pri = LOG_KERN|(level&LOG_PRIMASK)|NS_LOG_TO_CONVERT(mask);
    } else {
        pri = LOG_LOCAL0|(level&LOG_PRIMASK)|NS_LOG_TO_CONVERT(mask);
    }

    (void)time(&now);
    p = buff + sprintf(buff, "<%d>%.15s %s[%d]: ", pri, ctime(&now) + 4,
            ns_log_name, getpid());

    head_end = p;
    end = buff + sizeof(buff) - 2;
    va_start (argptr, fmt);
    p += vsnprintf(p, end - p, fmt, argptr);
    va_end(argptr);

    if (p >= end || p < head_end) {/* Returned -1 in case of error... */
        static const char truncate_msg[12] = "[truncated] "; /* no NUL! */
        memmove(head_end + sizeof(truncate_msg), head_end,
                end - head_end - sizeof(truncate_msg));
        memcpy(head_end, truncate_msg, sizeof(truncate_msg));
        if (p < head_end) {
            while (p < end && *p) {
                p++;
            }
        }
        else {
            p = end - 1;
        }
    }
    last_chr = p;

    if (mask & NS_LOG_TO_STDOUT) {
        if (*(last_chr-1) != '\n') {
            *last_chr = '\n';
        }
        (void)fwrite(buff, last_chr - buff + 1, 1, stdout);
    }

    if (mask & NS_LOG_TO_SYSLOG) {
        *last_chr = 0;
        ns_log_print_syslog(buff, last_chr - buff + 1);
    }

    if (NS_LOG_MOD(id) == NS_LOG_MOD_KERNEL) { /* Not Print kernel msg to console again */
        return 0;
    }

    if ((mask & NS_LOG_TO_CONSOLE) &&
            (fd = open(_PATH_CONSOLE, O_WRONLY | O_NOCTTY)) >= 0) {
        p = strchr(buff, '>') + 1;
        if (*(last_chr-1) == '\n') {
            last_chr--;
        }
        last_chr[0] = '\r';
        last_chr[1] = '\n';
        (void)write(fd, p, last_chr - p + 2);
        (void)close(fd);
    }

    return 0;
}

int
ns_log_init(const char *name)
{
	
	if (name == NULL) {
		name = NS_LOG_SYSLOG_NAME;
	}
	if (strlen(name) > 64) {
		fprintf(stderr, "Log name is too long! %s\n", name);
		return -1;
	}

	if (ns_log_open_conf(0, &ns_log_conf_fd, &ns_log_conf_addr) < 0) {
		printf("open conf failed \n");
		return -1;
	}

	pthread_mutex_init(&syslog_mutex, NULL);

	if (0 != ns_log_open_syslog()) {
		printf("open syslog failed \n");
		return -1;
	}

	ns_log_name = name;

	return 0;
}

int
ns_log_exit() 
{
	if (ns_log_name) {
		return 0;
	}
	ns_log_name = NULL;

	ns_log_close_syslog();

	ns_log_close_conf(&ns_log_conf_fd, &ns_log_conf_addr);

	pthread_mutex_destroy(&syslog_mutex);

	return 0;
}

void
print_buf(char * buf, int len)
{
	int i = 0;
	while (i < len) {
		printf("%02x ", (unsigned char)*(buf + i));
		i ++;
		if(i % 8 == 0)
			printf("\t");
		if(i % 16 == 0)
			printf("\n");
	}
	printf("\n");
}

#define LOG_FILE_DIR "/home/log/"
#define LOG_FILE_PREF "file_syslog"
#define LOG_FILE_UPLOAD_DIR "/tmp/uploadlog/"
int 
ns_log_pack_flash_log()
{
	int ret = 0;
	char buf[256] = {0};
	
	ret = remove(LOG_PACK_FILE_PATH);
	if (ret != 0) {
		ns_log(LOG_ERR, "%s: rm %s fail! %s", __func__, LOG_PACK_FILE_PATH, strerror(errno));
	}
	
	/* 这里不能改为后台运行，后台运行会导致web开始上传时该文件没有准备好 */
	sprintf(buf, "{ if [ ! -d %s ]; then mkdir %s;fi ; cp %s* %s; cp /tmp/syslog %s ; cd %s; tar zcvf %s * ;rm %s -rf; }", 
			LOG_FILE_UPLOAD_DIR, LOG_FILE_UPLOAD_DIR, LOG_FILE_DIR, LOG_FILE_UPLOAD_DIR,
			LOG_FILE_UPLOAD_DIR, LOG_FILE_UPLOAD_DIR, LOG_PACK_FILE_PATH,LOG_FILE_UPLOAD_DIR);
	system(buf);

	return 0;
}

void
ns_log_backtrace ()
{
}

