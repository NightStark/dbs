#!/bin/sh
gcc config.c  dhcpv4.c  dhcpv6.c  dhcpv6-ia.c  ndp.c  odhcpd.c  router.c  ubus.c
mips-linux-gcc dhcpv4.c  dhcpv6.c  dhcpv6-ia.c  ndp.c  odhcpd.c  router.c  uloop.c utils.c  md5.c usock.c blob.c blobmsg.c config_bhu.c ustream.c ustream-fd.c -std=gnu99  -Wall -o dhcpd -lresolv
