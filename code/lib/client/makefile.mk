DIR_PUB_CLIENT = $(DIR_LIB)/client

SRC_NSDB_C += $(DIR_PUB_CLIENT)/client_work.c
SRC_NSDB_C += $(DIR_PUB_CLIENT)/client_recv.c
SRC_NSDB_C += $(DIR_PUB_CLIENT)/client_askserver.c
SRC_NSDB_C += $(DIR_PUB_CLIENT)/client_nsdb.c
SRC_NSDB_C += $(DIR_PUB_CLIENT)/client_fsm.c
SRC_NSDB_C += $(DIR_PUB_CLIENT)/client_fsm_proc.c
SRC_NSDB_C += $(DIR_PUB_CLIENT)/client_send.c

SRC_NSDB += $(DIR_PUB_CLIENT)/client_work.c
SRC_NSDB += $(DIR_PUB_CLIENT)/client_recv.c
SRC_NSDB += $(DIR_PUB_CLIENT)/client_askserver.c
SRC_NSDB += $(DIR_PUB_CLIENT)/client_nsdb.c
SRC_NSDB += $(DIR_PUB_CLIENT)/client_fsm.c
SRC_NSDB += $(DIR_PUB_CLIENT)/client_fsm_proc.c
SRC_NSDB += $(DIR_PUB_CLIENT)/client_send.c
