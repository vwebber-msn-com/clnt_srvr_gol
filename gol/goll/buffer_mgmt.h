#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <ws2spi.h>
#include <stdio.h>

#include "xplatform.h"
#include "command_msgs.h"

#define BM_IN_NUM_POST 1
#define BM_RT_NUM_POST 5

#define     BM_MSG_CNT  100

#define		BM_BUF_CMD_CNT	2

#define BM_BUF_CNT 10

typedef struct _bm_msg_mgmt_t
{
    ublk_t       bm_msg_hdrs[BM_MSG_CNT];
    uint32	av_i;
} bm_msg_mgmt_t, *pbm_msg_mgmt_t;

typedef struct _bm_buf_hdr
{
#pragma pack(push, SLN_PACK)
	struct _bm_buf_hdr *link;
	uint8	used;
#pragma pack(pop)
} bm_buf_hdr, *pbm_buf_hdr;

typedef struct _bm_buf
{
#pragma pack(push, SLN_PACK)
	gol_cmd_buf_t		buf_ary[BM_BUF_CMD_CNT];
#pragma pack(pop)
} bm_buf, *pbm_buf;

typedef struct _bm_buf_wrap
{
#pragma pack(push, SLN_PACK)
	bm_buf_hdr		hdr;
	bm_buf			cbuf;
#pragma pack(pop)
} bm_buf_wrap, *pbm_buf_wrap;

typedef struct _bm_buf_mgmt_t
{
	bm_buf_wrap		bm_buf_list[BM_BUF_CNT];
	uint32			av_i;
} bm_buf_mgmt_t, *pbm_buf_mgmt_t;

typedef struct _bm_mtd_mgmt_t
{
	WSABUF	mtd[10];
	uint32	av_i;
} bm_mtd_mgmt_t, *pbm_mtd_mgmt_t;

typedef struct _bm_mgmt_t
{
    bm_mtd_mgmt_t   bm_mtd_mgmt;
    bm_msg_mgmt_t   bm_msg_mgmt;
    bm_buf_mgmt_t   bm_buf_mgmt;
    gol_cmd_buf_t   bm_ibuf;
} bm_mgmt_t, *pbm_mgmt_t;

/*
typedef struct _bm_sys_mgmt_t
{
    bm_mgmt_t   bm[Q_CNT];
} bm_sys_mgmt_t, *pbm_sys_mgmt_t;

*/

#ifdef BM_INSTANCE
/*
bm_sys_mgmt_t   bm_sys_mgmt;

bm_buf_mgmt_t	bm_buf_instance;
bm_mtd_mgmt_t	bm_mtd_instance;
cmd_fields_t		bm_ibuf;
*/
#else
//extern bm_sys_mgmt_t   bm_sys_mgmt;
#endif // BM_INSTANCE


LPWSABUF bm_get_mtd(pbm_mtd_mgmt_t pb, LPWSABUF *bp);

LPWSABUF bm_get_buffer(pbm_buf_mgmt_t pb, LPWSABUF bp);

void bm_put_buffer(uint16 bm_cnt, LPWSABUF bp);

int bm_init();

void bm_put_msg(publk_t pm);

int bm_get_msg(pbm_msg_mgmt_t pm, publk_t *pb);

