/* Copyright (C) 
* 2016 - langyj, lyj051031448@163.com
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
* 
*/
#ifndef __NS_WEB_EVENT_H__
#define __NS_WEB_EVENT_H__

#define HTTPREQ_EVTDIR_LEN (128)
typedef struct httpReq_evtDir_info
{
    DCL_NODE_S stNode;
    CHAR       acDir[HTTPREQ_EVTDIR_LEN];
    UCHAR      aucDirHash[SHA_DIGEST_LENGTH];
}HTTPREQ_EVTDIR_INFO_ST;

ULONG HTTPREQ_EvtDir_Init(VOID);

#endif //__NS_WEB_EVENT_H__
