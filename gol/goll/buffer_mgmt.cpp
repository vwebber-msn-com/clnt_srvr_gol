
#include "stdafx.h"
#define BM_INSTANCE
#include "buffer_mgmt.h"

int bm_init()
{ 
    return NOERROR;
}

void bm_put_msg(publk_t pm)
{
    pm->ob.pob = NULL;
    pm->ob.cmd_fields.cm_bflds.used = FALSE;
}

int bm_get_msg(pbm_msg_mgmt_t pm, publk_t *pb)
{
    int rc = NOERROR;

	while (
        (pm->bm_msg_hdrs[pm->av_i].ob.cmd_fields.cm_bflds.used)
		)
	{
        pm->av_i = ((++pm->av_i) % BM_MSG_CNT);
	}

    pm->bm_msg_hdrs[pm->av_i].ob.cmd_fields.cm_bflds.used = TRUE;

    *pb = &(pm->bm_msg_hdrs[pm->av_i]);

	return rc;
}

void bm_put_buffer(uint16 bm_cnt, LPWSABUF bp)
{
    ((pbm_buf_wrap)(bp->buf - sizeof(bm_buf_hdr)))->hdr.used = FALSE;
}


LPWSABUF bm_get_buffer(pbm_buf_mgmt_t pb, LPWSABUF bp)
{
    while (
        (pb->bm_buf_list[pb->av_i].hdr.used)
        )
    {
        pb->av_i = ((++pb->av_i) % BM_BUF_CNT);
    }

    pb->bm_buf_list[pb->av_i].hdr.used = TRUE;

    bp->buf = (CHAR *)&(pb->bm_buf_list[pb->av_i].cbuf);
    bp->len = sizeof(bm_buf);

    return bp;
}

LPWSABUF bm_get_mtd(pbm_mtd_mgmt_t pb, LPWSABUF *bp)
{
    *bp = &(pb->mtd[pb->av_i++]);

	return &(pb->mtd[pb->av_i++]);
}
