#config.c 为odhcp原文件(no use)， config_bhu.c为bhu定制文件。
#blob.c & blobmsg.c 为openwrt上UCI配置使用的接口。bhu不用。
#srcs=dhcpv4.c  dhcpv6.c  dhcpv6-ia.c  ndp.c  odhcpd.c  router.c  uloop.c utils.c  md5.c usock.c blob.c blobmsg.c config_bhu.c ustream.c ustream-fd.c dn_comp.c
#dn_comp.c 来自于openWRT的toolchain-mips_34kc_gcc-5.2.0_musl-1.1.12/musl-1.1.12/src/network
srcs=dhcpv4.c  dhcpv6.c  dhcpv6-ia.c  ndp.c  odhcpd.c  router.c  uloop.c utils.c  md5.c usock.c config_bhu.c ustream.c ustream-fd.c dn_comp.c

objs=$(patsubst %c,%o,$(srcs))

CFLAGS=-std=gnu99 -Wall
CFLAGS += -D_GNU_SOURCE
LIBS=-lresolv

#CC=mips-linux-gcc
#CC=mipsel-openwrt-linux-gcc
#CC=gcc
CC=i486-openwrt-linux-gcc
odhcpd:$(objs)
	$(CC) $(objs) $(CFLAGS) $(LIBS) -o odhcpd
#mips-linux-gcc dhcpv4.c  dhcpv6.c  dhcpv6-ia.c  ndp.c  odhcpd.c  router.c  uloop.c utils.c  md5.c usock.c blob.c blobmsg.c config_bhu.c ustream.c ustream-fd.c -std=gnu99  -Wall -o dhcpd -lresolv


%.o:%.c 
	$(CC) -c $< -o $@ $(CFLAGS) $(LIBS)
	
clean:
	rm *.o -rfv
