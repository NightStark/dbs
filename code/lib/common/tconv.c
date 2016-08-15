/*****************************************************************************
//
//  Copyright (c) 2011 Shanghai Shenghua Communication Co., Ltd.
//  All Rights Reserved
//  No portions of this material may be reproduced in any form without the
//  written permission of:
//	Shanghai Shenghua Communication Co., Ltd.
//	Room 1222, Building 3, No.100, Jinyu Road,
//	Pudong New District, Shanghai, China
//  All information contained in this document is Shanghai Shenghua
//  Communication Co., Ltd. private, proprietary, and trade secret.
//
******************************************************************************
//
//  Filename:       tconv.c
//  Author:         Terry Yang <ybable@gmail.com>
//  Creation Date:  19/08/2011
//  Change history:
//
//  Description:
//	TODO
//
*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "ns_type_o.h"

#define TC_STR_ENABLE	"enable"
#define TC_STR_DISABLE	"disable"
#define TC_STR_DENY    	"deny"
#define TC_STR_ADMIT    "allow"
#define TC_STR_TRUE     "true"
#define TC_STR_FALSE    "false"

#define TC_STR_SWITCH_ON	"on"
#define TC_STR_SWITCH_OFF	"off"

static const char *const week_days[] = {
    "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun",
};

char *
tc_ip2str (u32 ipv4, char *buf)
{
	init_buf(buf, 4*4);
	sprintf(buf, "%d.%d.%d.%d", (ipv4>>24)&0xff, (ipv4>>16)&0xff,
			(ipv4>>8)&0xff, ipv4&0xff);
	return buf;
}

char *
tc_ipport2str (u32 ipv4, u16 port, char *buf)
{
	if (port) {
		init_buf(buf, 22);
		sprintf(buf, "%d.%d.%d.%d:%d", (ipv4>>24)&0xff, (ipv4>>16)&0xff,
				(ipv4>>8)&0xff, ipv4&0xff, port);
		return buf;
	} else {
		return tc_ip2str(ipv4, buf);
	}
}

u32
tc_str2ip (const char *str)
{
	return ntohl(inet_addr(str));
}

int
tc_str2ip6 (const char *str, struct in6_addr *in6)
{
	return inet_pton(AF_INET6, str, in6);
}

int
tc_str2ipport (const char *str, u32 *ip, u16 *port)
{
	const char *sp;
	char buf[16];
	int n;

	if (str == NULL)
		return -1;

	sp = strchr(str, ':');
	if (sp != NULL) {
		n = sp - str;
		sp++;
		*port = atoi(sp);
	} else {
		n = strlen(str);
		*port = 0;
	}
	if (n<8 || n>15)
		return -1;
	memcpy(buf, str, n);
	buf[n] = 0;

	*ip = ntohl(inet_addr(buf));

	return 0;
}

char *
tc_ip6addr2str (struct in6_addr *in6, char *buf)
{
	int nb = 0;
	if (buf == NULL) {
		buf = malloc(64);
		nb = 1;
		if (buf == NULL)
			return NULL;
	}
	if (inet_ntop(AF_INET6, in6, buf, 63) == NULL)  {
		if (nb)
			free(buf);
		return NULL;
	}
	return buf;
}
/* return mask */
int
tc_str2ip6addr (const char *str, struct in6_addr *in6, u8 *prefix)
{
	char buf[64];
	char *slash;

	slash = strchr(str, '/');
	memcpy(buf, str, slash - str);
	buf[slash-str] = 0;
	if (inet_pton(AF_INET6, buf, in6) != 1)
		return -1;
	if (slash == NULL)
		return 128;
	*prefix = atoi(slash+1);
	return *prefix;
}

u8
tc_str2inetmask (const char *str)
{
	if (str == NULL)
		return 0;
	if (strchr(str, '.') == NULL)
		return atol(str);
	else {
		int i = 0;
        unsigned int msk = 0x80000000;
		unsigned int m = inet_addr(str);
        while(msk){
			if (m & msk)
                i ++;
			msk >>= 1;
		}
		return i;
	}
}

u32
tc_inetmask2mask (u8 im)
{
	u32 i, j;
	for (i=0, j=0; i < im; i++) {
		j >>= 1;
		j |= 0x80000000;
	}
	return j;
}

u8
tc_mask2inetmask (u32 mask)
{
	u8 i;
	if (mask == 0)
		return 0;
	for (i=1; i<=32 && (mask & (1<<(32-i))); i++)
		;
	return i-1;
}

void
tc_cutwrap(char *str)
{
    int i;
    int len;
    if(!str)
        return;
    len = strlen(str);
    for(i = len - 1; i >= 0; i --){
        if(str[i] == '\r' || str[i] == '\n')
            str[i] = 0;
        else
            break;
    }
}


char *
tc_mac2str (const u8 *mac, char *buf)
{
	init_buf(buf, 6*3);
	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return buf;
}

u8 *
tc_str2mac (const char *str, u8 *buf)
{
	unsigned int mac[6];
	int num1=0, num2=0, i=0;
	init_buf(buf, 6);
	memset(buf, 0, 6);
	for(i=0; str[i]!='\0'; i++) {
		if(str[i] == ':')
			num1 ++;
		if(str[i] == '-')
			num2 ++;
	}
	if(num1 == 5)
		i = sscanf(str, "%2x:%2x:%2x:%2x:%2x:%2x",
				mac, mac+1, mac+2, mac+3, mac+4, mac+5);
	else if(num2 == 5)
		i = sscanf(str, "%2x-%2x-%2x-%2x-%2x-%2x",
				mac, mac+1, mac+2, mac+3, mac+4, mac+5);
	else
		return buf;

	if (i < 6)
		return NULL;

	*(uint8_t *)buf = mac[0];
	*(uint8_t *)(buf+1)  = mac[1];
	*(uint8_t *)(buf+2)  = mac[2];
	*(uint8_t *)(buf+3)  = mac[3];
	*(uint8_t *)(buf+4)  = mac[4];
	*(uint8_t *)(buf+5)  = mac[5];
	return buf;
}

u8 *
tc_str2mac_ext(const char *str, u8 *buf)
{
	unsigned int mac[6] = {0};
	int num1=0, num2=0, i=0, len=0, flag = 0;
	const  char *pos = NULL, *pre = NULL;
	
	init_buf(buf, 6);
	memset(buf, 0, 6);
	
	for(i=0; str[i]!='\0'; i++) {
		if(str[i] == ':')
			num1 ++;
		if(str[i] == '-')
			num2 ++;
	}
	
	pre = str;
	if(num1 == 5) {
		pos = strchr(pre, ':');
	} else if(num2 == 5){
		pos = strchr(pre, '-');
	} else {
		return NULL;
	}	
	while(pos) {
		if(strncasecmp(pre, "xx", pos-pre)) {
			len ++;
		} else {
			flag = 1;
			break;
		}
		pre = pos+1;
		if(num1 == 5) {
			pos = strchr(pre, ':');
		} else if(num2 == 5){
			pos = strchr(pre, '-');
		} else {
			return NULL;
		}	
	}
	if(!flag) {
		pos = &str[strlen(str)];
		if(strncasecmp(pre, "xx", pos-str)) {
			len ++;
		}
	}

	if(len == 0)
		return NULL;

	if(num1 == 5)
		i = sscanf(str, "%2x:%2x:%2x:%2x:%2x:%2x",
				mac, mac+1, mac+2, mac+3, mac+4, mac+5);
	else if(num2 == 5)
		i = sscanf(str, "%2x-%2x-%2x-%2x-%2x-%2x",
				mac, mac+1, mac+2, mac+3, mac+4, mac+5);
	else
		return NULL;
	
	if(i < len)
		return NULL;

	*(uint8_t *)buf = mac[0];
	*(uint8_t *)(buf+1)  = mac[1];
	*(uint8_t *)(buf+2)  = mac[2];
	*(uint8_t *)(buf+3)  = mac[3];
	*(uint8_t *)(buf+4)  = mac[4];
	*(uint8_t *)(buf+5)  = mac[5];
	*(uint8_t *)(buf+6)  = (uint8_t)len;
	return buf;
}

u8
tc_str2enable (const char *str)
{
	if (strcmp(TC_STR_ENABLE, str) == 0)
		return 1;
	else
		return 0;
}

u8
tc_str2bool (const char *str)
{
    if (strcmp(TC_STR_TRUE, str) == 0)
        return 1;
    else
        return 0;
}

u8
tc_deny2enable (const char *str)
{
	if ((strcmp(TC_STR_ADMIT, str) == 0) 
#ifdef KOCHAB_TMP_VER_T
            || (strcmp(TC_STR_ENABLE, str) == 0)
#endif
            )
		return 0;
	else
		return 1;
}

const char *
tc_deny2str (u8 i)
{
	if (i > 0)
		return TC_STR_DENY;
	else
		return TC_STR_ADMIT;
}

const char *
tc_enable2str (u8 i)
{
	if (i > 0)
		return TC_STR_ENABLE;
	else
		return TC_STR_DISABLE;
}

const char *
tc_bool2str(u8 i)
{
    if (i > 0) 
        return TC_STR_TRUE;
    else
        return TC_STR_FALSE;
}

u8
tc_str2switch (const char *str)
{
	if (strcmp(TC_STR_SWITCH_ON, str) == 0)
		return 1;
	else
		return 0;
}

const char *
tc_switch2str (u8 i)
{
	if (i > 0)
		return TC_STR_SWITCH_ON;
	else
		return TC_STR_SWITCH_OFF;
}

char *
tc_i2str_buf(int i, char *buf)
{
	init_buf(buf, 12);
	sprintf(buf, "%d", i);
	return buf;
}

char *
tc_ll2str_buf(long long ll, char *buf)
{
	init_buf(buf, 22);
	sprintf(buf, "%lld", ll);
	return buf;
}
char *
tc_ull2str_buf(unsigned long long ull, char *buf)
{
	init_buf(buf, 22);
	sprintf(buf, "%llu", ull);
	return buf;
}

char *
tc_i2str (int i)
{
	char *buf = NULL;
	init_buf(buf, 12);
	sprintf(buf, "%d", i);
	return buf;
}

char *
tc_ll2str (long long ll)
{
	char *buf = NULL;
	init_buf(buf, 22);
	sprintf(buf, "%lld", ll);
	return buf;
}
char *
tc_ull2str (unsigned long long ull)
{
	char *buf = NULL;
	init_buf(buf, 22);
	sprintf(buf, "%llu", ull);
	return buf;
}

int
tc_str2enum (const char *strs[], int n, int i, char *s)
{
	if (strs == NULL || i < 0)
		return -1;
	for (; i < n; i++) {
		if (strcasecmp(strs[i], s) == 0)
			return i;
	}
	return -1;
}

int
tc_isvalidmac (const u8 *mac, int len)
{
	if(len < 6) {
		return 0;
	}
	if (mac[0]!=0 || mac[1]!=0 || mac[2]!=0 || mac[3]!=0 ||mac[4]!=0 || mac[5]!=0)
		return 1;
	return 0;
}
char *
tc_bin2hexstr(const u8 *in, int len)
{
    int i = 0;
    char  * outkey = malloc(2*len+1);
    if(outkey == NULL){
        return NULL;
    }

    for(i = 0; i < len; i++){
        sprintf(outkey+i*2, "%02x", in[i]);
    }
    return outkey;
}
int
tc_hexstr2bin(const char *in, int len, u8 ** out, int *outlen)
{
    int i = 0, len_t = len/2+1, k =0;

    if(len == 0 || len%2 == 1){
        return -1;
    }

    *out = malloc(len_t);
    if(*out == NULL){
        return -1;
    }
    memset(*out, 0 , len_t);

    for(i = 1, k = 0; i <= len; k = i/2, i++){
        if(in[i-1] >= '0' && in[i-1] <= '9'){
            if(i%2 == 1){
                 (*out)[k] = (*out)[k] | ((in[i-1] - '0') << 4);
            }else{
                 (*out)[k] += (in[i-1] - '0');
            }
        }else if(in[i-1] >= 'A' && in[i-1] <= 'F'){
            if(i%2 == 1){
                (*out)[k] = (*out)[k] | (in[i-1]-'A' + 10) << 4;
            }else{
                (*out)[k] += in[i-1]-'A' + 10;
            }
        }else if(in[i-1] >= 'a' && in[i-1] <= 'f'){
            if(i%2 == 1){
                (*out)[k] = (*out)[k] |  (in[i-1]-'a' + 10) << 4;
            }else{
                (*out)[k] += in[i-1]-'a' + 10;
            }
        }else{
            free(*out);
            return -1;
        }
    }
    
    *outlen = len_t-1;
    return 0;
}


int
tc_str2daysecs(const char *str)
{
	int h = 0, m = 0, s = 0;
	if(str == NULL)
		return -1;
	if(strlen(str) < 5)
		return -1;
	if(sscanf(str, "%d:%d:%d", &h, &m, &s) != 3)
		return -1;
	if(h < 0 || h > 23 ||
		m < 0 || m > 59 ||
		s < 0 || s > 59)
		return -1;
	return h * 3600 + m * 60 + s;
}

char *
tc_daysecs2str(long secs)
{
	int h = 0, m = 0, s = 0;
	char *buf = NULL;
	init_buf(buf, 9);
	if(secs < 0)
		return NULL;
	secs = (secs + 3600 * 24) % (3600 *24);
	h = secs / 3600;
	m = (secs / 60 + 60) % 60;
	s = (secs + 60) % 60;
	sprintf(buf, "%02d:%02d:%02d", h, m, s);
	return buf;
}

time_t
tc_str2datesecs(const char *str)
{
    struct tm tm;
    char *env;
	int y = 0, m = 0, d = 0;
    time_t ret = 0;


	if(str == NULL)
		return -1;
	if(strlen(str) < 8)
		return -1;
	if(sscanf(str, "%d-%d-%d", &y, &m, &d) != 3)
		return -1;

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = y - 1900;
    tm.tm_mon = m - 1;
    tm.tm_mday = d;

    env = getenv("TZ");
    setenv("TZ", env, 1);
    tzset();
    ret = mktime(&tm);
    setenv("TZ", env, 1);
    return ret;
}

char *
tc_datesecs2str(time_t date)
{
    struct tm *tm;
    char *buf = NULL;
    tm = gmtime(&date);
    if(tm == NULL)
        return NULL;
	init_buf(buf, 11);

	sprintf(buf, "%04u-%02u-%02u", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
	return buf;

}

int
tc_weekdays2i(const char *weekdays)
{
    int i = 0;
    for(i = 0; i < sizeof(week_days)/sizeof(week_days[0]); i ++){
        if(strcmp(weekdays, week_days[i]) == 0)
            return i + 1;
    }
    return -1;
}

const char *
tc_i2weekdays(int i)
{
    if(i <= 7 && i >= 1)
        return week_days[i - 1];
    return NULL;
}

int
tc_range2i(const char *str, int *start, int *end){
    int s, e;
    if(str == NULL)
        return -1;

	if(sscanf(str, "%d-%d", &s, &e) != 2)
		return -1;

    *start = s;
    *end = e;
    return 0;
}

/* Overencodes */
#define URL_XALPHAS     (unsigned char) 1
#define URL_XPALPHAS    (unsigned char) 2
	
static unsigned char isAcceptable[96] =

/*      Bit 0           xalpha          -- see HTFile.h
**      Bit 1           xpalpha         -- as xalpha but with plus.
**      Bit 2 ...       path            -- as xpalpha but with /
*/
    /*   0 1 2 3 4 5 6 7 8 9 A B C D E F */
    {    7,0,0,0,0,0,0,0,0,0,7,0,0,7,7,0,       /* 2x   !"#$%&'()*+,-./ */
         7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,       /* 3x  0123456789:;<=>?  */
         7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,       /* 4x  @ABCDEFGHIJKLMNO */
         7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,7,       /* 5X  PQRSTUVWXYZ[\]^_ */
         0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,       /* 6x  `abcdefghijklmno */
         7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,0 };     /* 7X  pqrstuvwxyz{\}~ DEL */
 
#define ACCEPTABLE(a)   ( a>=32 && a<128 && ((isAcceptable[a-32]) & mask))

static const char *hex = "0123456789ABCDEF";

char *urlencode(char *str)
{
    unsigned char mask = URL_XPALPHAS;
    char * p;
    char * q;
    char * result;
    int unacceptable = 0;
    for(p=str; *p; p++) {
      if (!ACCEPTABLE((unsigned char)*p)) {
                unacceptable +=2;
      }
    }
    result = (char *) malloc(p-str + unacceptable + 1);
    bzero(result,(p-str + unacceptable + 1));

    if (result == NULL)
    {
	return(NULL);
    }
    for(q=result, p=str; *p; p++) {
        unsigned char a = *p;
        if (!ACCEPTABLE(a)) {
            *q++ = '%';  /* Means hex coming */
            *q++ = hex[a >> 4];
            *q++ = hex[a & 15];
        } else if (a == ' ') {
	  *q++ = '+';  /* space becomes + */
	} else {
	  *q++ = *p;
	}
    }
    *q++ = 0;                   /* Terminate */
    return result;
}

