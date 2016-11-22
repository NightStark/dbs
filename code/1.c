#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ns_base.h>

#include <ns_table.h>
#include <ns_net.h>
#include <ns_msg.h>
typedef struct tag_msg_type_desc
{
    INT type_type[128];
    INT iMsgType;
}MSG_TYPE_DESC_ST;

MSG_TYPE_DESC_ST g_past_type_desc_list[] = 
{
//typedef struct tag_msg_mng_join_req
{
 
	{
		6,
		1,
	},
	 MSG_MNG_JOIN_REQ,
},
//typedef struct tag_msg_mng_join_req
{
 
	{
		6,
		1,
	},
	 MSG_MNG_JOIN_REQ,
},
//typedef struct tag_msg_mng_join_resp
{
 
	{
		6,
		8,
	},
	 MSG_MNG_JOIN_RESP,
},
//typedef struct tag_msg_mng_confirm
{
 
	{
		6,
	},
	 MSG_MNG_CONFIRM,
},
//typedef struct tag_msg_mng_ok
{
 
	{
		6,
	},
	 MSG_MNG_OK,
},
//typedef struct tag_msg_ctl_attach
{
 
	{
		6,
		6,
		6,
	},
	 MSG_CTL_ATTACH,
},
};

int main(void)
{
   int i = 0, j = 0; 
   for (i = 0; i < 6; i++) {
       for (j = 0; j < 6; j++) {
           printf("type type=%d\n", g_past_type_desc_list[i].type_type[j]);
       }
       printf("type=%d\n", g_past_type_desc_list[i].iMsgType);
   }
}
