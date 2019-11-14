
#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN

#include "windows.h"
#include "process.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <ws2spi.h>
#include <stdio.h>
#include "xplatform.h"

#include "buffer_mgmt.h"
#include "que_mgmt.h"
#include "command_msgs.h"

void cmd_nw_prep_s1(QM_SYS_T sys, paccept_rb_t pa)
{
    pa->overlapped_blk.hEvent = que_mgmt.qm_ary[sys].h_event;
}

void cmd_nw_prep(QM_SYS_T sys, paccept_rb_t pa)
{
    ZeroMemory(&(pa->overlapped_blk), sizeof(WSAOVERLAPPED));
    qm_get_mtd(sys, &(pa->p_buffers));
    qm_get_init(sys, pa->p_buffers);
    pa->overlapped_blk.hEvent = que_mgmt.qm_ary[sys].h_event;
}

void cmd_cl_prep(QM_SYS_T sys, pconnect_rb_t pa)
{
    ZeroMemory(&(pa->overlapped_blk), sizeof(WSAOVERLAPPED));
    qm_get_buffer(sys, pa->p_buffers);
    pa->overlapped_blk.hEvent = que_mgmt.qm_ary[sys].h_event;
}



void cmd_socket_close(SOCKET *s)
{
    if (*s != NULL)
    {
        closesocket(*s);
        *s = NULL;
    }
}

void cmd_msg_prep_post(QM_SYS_T src, QM_SYS_T dst, uint8 *buf)
{
    publk_t pu;

    qm_get_msg(src, &pu);

    pu->un.gcmd.sub_cmd = ((pgol_cmd_buf_t)buf)->gol_cmd;

    pu->un.gcmd.pdata = buf;

    qm_post_task(dst, pu);
    
}

int cmd_server_send(QM_SYS_T src, LPWSABUF pb)
{
    int rc = NOERROR, s_err = NOERROR;

    paccept_rb_t pa = qm_srvr_sckt();

    pa->cnt_buffers;
    pa->p_buffers->buf = pb->buf;
    pa->p_buffers->len = pb->len;

    rc = WSASend(pa->accept_socket, pa->p_buffers, 1, &(pa->bytes_read), 0, NULL, NULL);
    if (rc == SOCKET_ERROR) {
        s_err = WSAGetLastError();
        printf("WSASend failed with error: %d\n", s_err);
        rc = EXIT_ERROR;
    }

    qm_put_buffer(Q_SOC, pb);

    return rc;
}

void soc_msg_process(publk_t pu)
{
    WSABUF wb;

    switch (pu->un.scmd.cmd_sb)
    {
    case CMS_SCKT_SEND:

        wb.buf = (CHAR *)pu->un.scmd.pdata;
        wb.len = sizeof(bm_buf);

        cmd_server_send(Q_SOC, &wb);

        qm_put_msg(Q_SOC, pu);

        break;

    default:

        break;
    }
}






int cmd_init()
{
    return NOERROR;

}

