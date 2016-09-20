#ifndef _TCONV_H_
#define _TCONV_H_

#include <netinet/in.h>

char *
tc_ip2str (u32 ipv4, char *buf);
char *
tc_i2str_buf(int i, char *buf);
char *
tc_ll2str_buf(long long ll, char *buf);
char *
tc_ull2str_buf(unsigned long long ull, char *buf);
char *
tc_ipport2str (u32 ipv4, u16 port, char *buf);

u32
tc_str2ip (const char *str);

int
tc_str2ip6 (const char *str, struct in6_addr *in6);

int
tc_str2ipport (const char *str, u32 *ip, u16 *port);

char *
tc_ip6addr2str (struct in6_addr *in6, char *buf);

int
tc_str2ip6addr (const char *str, struct in6_addr *in6, u8 *prefix);

u8
tc_str2inetmask (const char *str);

u32
tc_inetmask2mask (u8 im);

u8
tc_mask2inetmask (u32 mask);

char *
tc_mac2str (const u8 *mac, char *buf);

u8 *
tc_str2mac (const char *str, u8 *buf);

u8 *
tc_str2mac_ext (const char *str, u8 *buf);

u8
tc_str2enable (const char *str);

u8
tc_str2bool (const char *str);

u8
tc_deny2enable (const char *str);

const char *
tc_enable2str (u8 i);

const char *
tc_bool2str (u8 i);

const char *
tc_deny2str (u8 i);


u8
tc_str2switch (const char *str);

const char *
tc_switch2str (u8 i);

char *
tc_i2str (int i);

char *
tc_ull2str (unsigned long long i);

char *
tc_ll2str (long long ll);

int
tc_str2enum (const char *strs[], int n, int i, char *s);

int
tc_isvalidmac (const u8 *mac, int len);

int
tc_str2daysecs(const char *str);

char *
tc_daysecs2str(long secs);

char *
tc_bin2hexstr(const u8 *in, int len);
int
tc_hexstr2bin(const char *in, int len, u8 ** out, int *outlen);

time_t
tc_str2datesecs(const char *str);

char *
tc_datesecs2str(time_t date);

int
tc_weekdays2i(const char *weekdays);

const char *
tc_i2weekdays(int i);

int
tc_range2i(const char *str, int *start, int *end);
void
tc_cutwrap(char *str);


#define tc_str2enum2(ss, s) tc_str2enum(ss, sizeof(ss)/sizeof(*ss), 1, s)

char *urlencode(char *str);

#endif /* _TCONV_H_ */
