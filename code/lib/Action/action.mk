DIR_LIB_ACTION = $(DIR_LIB)/Action

include $(DIR_LIB_ACTION)/setting/makefile.mk

SRC_NSDB  += $(DIR_LIB_ACTION)/web_action_handle.c
SRC_NSDB  += $(DIR_LIB_ACTION)/web_action_init.c
SRC_NSDB  += $(DIR_LIB_ACTION)/action_dec.c
SRC_NSDB  += $(DIR_LIB_ACTION)/web_http_request_event_dir.c
