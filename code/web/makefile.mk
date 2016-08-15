DIR_WEB = $(DIR_TREE)/web

include $(DIR_WEB)/http/makefile.mk
include $(DIR_WEB)/setting/makefile.mk

#web_main_init.c可以放到lib/thread里面去
#SRC_NSDB += $(DIR_WEB)/web_main_init.c

SRC_WEBS += $(DIR_WEB)/web_server_init.c
SRC_WEBS += $(DIR_WEB)/main.c


