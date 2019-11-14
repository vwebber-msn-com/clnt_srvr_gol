// scgol.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
//#include <ws2spi.h>
#include <stdio.h>
#include "xplatform.h"
#include "que_mgmt.h"

#include "buffer_mgmt.h"
#include "command_msgs.h"
#include "gol.h"

#pragma comment(lib, "Ws2_32.lib")

#define ovl ((pconnect_rb_t)lpOverlapped)

void CALLBACK recv_cr(
    IN DWORD dwError,
    IN DWORD cbTransferred,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN DWORD dwFlags
)
{
    int rc;
    int s_err = NOERROR;
    BOOL do_post = FALSE;
    DWORD Flags = 0;

    if (cbTransferred == 0)
    {
        printf("WSARecv failed with error: %d\n", s_err);
    }
    else do_post = TRUE;

    // Do Recv
    do
    {
        if (do_post)
        {
            qm_prep_post_task(Q_GOL, ovl->p_buffers);
            rc = qm_get_buffer(Q_SOC, ovl->p_buffers);
        }
        else do_post = TRUE;
        ovl->bytes_read = 0;
        rc = WSARecv(ovl->connect_socket, ovl->p_buffers, 1,
            &(ovl->bytes_read), &Flags,
            &(ovl->overlapped_blk), &recv_cr);
        if ((rc == SOCKET_ERROR) && (WSA_IO_PENDING != (s_err = WSAGetLastError())))
        {
            printf("WSARecv failed with error: %d\n", s_err);
            cmd_socket_close(&(ovl->connect_socket));
            break;
        }
    } while (ovl->bytes_read != 0);

}

int client_init(publk_t ub)
{
    int rc = NOERROR, trc = NOERROR;
    int s_err;
    DWORD Flags = 0;
    const pconnect_rb_t cmd = (pconnect_rb_t)ub->un.scmd.pdata;

    cmd->connect_socket = INVALID_SOCKET;

    do
    {
        // Initialize Winsock
        rc = WSAStartup(MAKEWORD(2, 2), &(cmd->wsa_data));
        if (rc != NOERROR) {
            rc = EXIT_ERROR;
            break;
        }

        // Create a connect socket
        cmd->connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (cmd->connect_socket == INVALID_SOCKET) {
            printf("Create of cmd->connect_socket socket failed with error: %u\n", WSAGetLastError());
            WSACleanup();
            rc = EXIT_ERROR;
            break;
        }

        // Bind loopback 
        inet_pton(AF_INET, LOOPBACK_ADDR_CLNT, &(cmd->saddr_srvr_out.sin_addr));
        cmd->saddr_srvr_out.sin_family = AF_INET;
        cmd->saddr_srvr_out.sin_port = htons(TEST_PORT + 1);

        if (bind(cmd->connect_socket, (SOCKADDR *)&(cmd->saddr_srvr_out), sizeof(cmd->saddr_srvr_out)) == SOCKET_ERROR) {
            printf("bind failed with error: %u\n", WSAGetLastError());
            cmd_socket_close(&(cmd->connect_socket));
            WSACleanup();
            rc = EXIT_ERROR;
            break;
        }

        cmd->saddr_srvr_in.sin_port = htons(TEST_PORT);
        cmd->saddr_srvr_in.sin_family = AF_INET;
        inet_pton(AF_INET, LOOPBACK_ADDR, &(cmd->saddr_srvr_in.sin_addr));

        rc = connect(cmd->connect_socket, (const sockaddr *)&(cmd->saddr_srvr_in), (int)sizeof(cmd->saddr_srvr_in));
        if (rc == SOCKET_ERROR) {
            printf("connect failed with error: %u\n", WSAGetLastError());
            WSACleanup();
            rc = EXIT_ERROR;
            break;
        }

        qm_get_mtd(Q_SOC, &(cmd->p_buffers));
        qm_get_init(Q_SOC, cmd->p_buffers);
        gol_fill_cmd_buf(CMG_STRT_RUN, (pgol_cmd_buf_t)cmd->p_buffers->buf);
        trc = WSASend(cmd->connect_socket, cmd->p_buffers, 1, &(cmd->bytes_sent), 0, NULL, NULL);
        if ((trc == SOCKET_ERROR) && (WSA_IO_PENDING != (s_err = WSAGetLastError()))) {
            printf("WSASend failed with error: %d\n", s_err);
            rc = EXIT_ERROR;
            break;
        }

        qm_put_msg(Q_SOC, ub);

        // Get new buffer 
        cmd_cl_prep(Q_SOC, cmd);

        // Do Recv
        trc = WSARecv(cmd->connect_socket, cmd->p_buffers, 1,
            &(cmd->bytes_read), &Flags,
            &(cmd->overlapped_blk), &recv_cr);
        if ((trc == SOCKET_ERROR) && (WSA_IO_PENDING != (s_err = WSAGetLastError()))) {
            printf("WSARecv failed with error: %d\n", s_err);
            printf("Error at line %d in function %s \n", __LINE__, __FUNCTION__);
            rc = EXIT_ERROR;
            cmd_socket_close(&(cmd->connect_socket));
            break;
        }

    } while (SUPPORT_FOR_C_EXCEPTIONS);
    return rc;
}


int main()
{
    int rc = NOERROR;
    sys_init_blk_t sib;
    publk_t pu;

    // Init systems
    do
    {
        if (rc = cmd_init()) break;
        if (rc = bm_init()) break;
        if (rc = qm_init()) break;

        sib.aff_mask = 1;
        sib.thrd_func = gol_thrd;
        if (rc = qm_init_sys(Q_GOL, &sib)) break;

        // Create and run game engine thread
        if (rc = qm_start_thread(Q_GOL)) break;

        // Morph into socket thread
        sib.aff_mask = 4;
        sib.thrd_func = NULL;
        if (rc = qm_init_sys(Q_SOC, &sib)) break;

        // No need to start thread
        // Init runtime vars

        if (rc = qm_init_runtime(Q_SOC)) break;

        qm_get_msg(Q_SOC, &pu);

        pu->un.scmd.pdata = (void *)qm_clnt_sckt();

        if (rc = client_init(pu)) break;

        qm_task_loop(Q_SOC);

    } while (SUPPORT_FOR_C_EXCEPTIONS);

    return rc;

}

