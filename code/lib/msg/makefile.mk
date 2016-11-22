DIR_PUB_MSG= $(DIR_LIB)/msg

SRC_NSDB += $(DIR_PUB_MSG)/msg_server_init.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_recv.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_data_encode.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_data_decode.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_encode.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_decode.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_desc.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_server.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_server_link.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_server_task.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_server_ctl.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_send.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_send_thread.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_client_link.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_client_init.c
SRC_NSDB += $(DIR_PUB_MSG)/msg_client_ctl.c

SRC_NSDB_C += $(DIR_PUB_MSG)/msg_client_init.c
#SRC_NSDB_C  += $(DIR_PUB_MSG)/msg_client.c
SRC_NSDB_C += $(DIR_PUB_MSG)/msg_data_encode.c
SRC_NSDB_C += $(DIR_PUB_MSG)/msg_data_decode.c
SRC_NSDB_C += $(DIR_PUB_MSG)/msg_encode.c
SRC_NSDB_C += $(DIR_PUB_MSG)/msg_decode.c
SRC_NSDB_C += $(DIR_PUB_MSG)/msg_desc.c

include $(DIR_PUB_MSG)/msgque/makefile.mk
include $(DIR_PUB_MSG)/signal/makefile.mk
