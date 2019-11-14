// csgol.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "intrin.h"

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
#include "command_msgs.h"
#include "que_mgmt.h"

#pragma comment(lib, "Ws2_32.lib")

accept_rb_t ar;

void CALLBACK recv_cr(
    IN DWORD dwError,
    IN DWORD cbTransferred,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN DWORD dwFlags
)
{
    int s_err = NOERROR, read_loop = FALSE;
    DWORD Flags = 0;

    do
    {
        if (cbTransferred == 0)
        {
            printf("WSARecv failed with error: %d\n", (s_err=WSAGetLastError()));
            //cmd_socket_close(&(((paccept_rb_t)lpOverlapped)->accept_socket));
            //cmd_socket_close(&(((paccept_rb_t)lpOverlapped)->listen_socket));
            read_loop = FALSE;
            s_err = NOERROR;

            //break;
        }
        else
        {
            // Post Buffer to Cmd Engine
            cmd_msg_prep_post(Q_SOC, Q_GOL, (uint8 *)((paccept_rb_t)lpOverlapped)->p_buffers->buf);
        }

        read_loop = FALSE;

        /*
        // Get new buffer 
        rc = qm_get_buffer(Q_SOC, ((paccept_rb_t)lpOverlapped)->p_buffers);

        // Do Recv
        DWORD Flags = 0;
        int s_err;
        rc = WSARecv(((paccept_rb_t)lpOverlapped)->accept_socket, ((paccept_rb_t)lpOverlapped)->p_buffers, 1,
            &(((paccept_rb_t)lpOverlapped)->bytes_read), &Flags,
            &(((paccept_rb_t)lpOverlapped)->overlapped_blk), &recv_cr);
        if ((rc == SOCKET_ERROR) && (WSA_IO_PENDING != (s_err = WSAGetLastError()))) {
            printf("WSARecv failed with error: %d\n", s_err);
            cmd_socket_close(&(((paccept_rb_t)lpOverlapped)->accept_socket));
            cmd_socket_close(&(((paccept_rb_t)lpOverlapped)->listen_socket));
            read_loop = false;
        }
        */

        // Do I need to reset event ???
    } while (read_loop);

}

int server_init(publk_t ob)
{
    int rc = NOERROR, trc = NOERROR;
    WSADATA wsa_data;
    paccept_rb_t pa = (paccept_rb_t)ob->un.scmd.pdata;

    cmd_nw_prep(Q_SOC, pa);

    pa->listen_socket = INVALID_SOCKET;
    pa->accept_socket = INVALID_SOCKET;

    do
    {

        // Initialize Winsock
        rc = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (rc != NOERROR) {
            rc = EXIT_ERROR;
            break;
        }

        // Create a connect socket
        pa->listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (pa->listen_socket == INVALID_SOCKET) {
            printf("Create of pa->listen_socket socket failed with error: %u\n", WSAGetLastError());
            WSACleanup();
            rc = EXIT_ERROR;
            break;
        }

        inet_pton(AF_INET, LOOPBACK_ADDR, &(pa->saddr_srvr_in.sin_addr));
        pa->saddr_srvr_in.sin_family = AF_INET;
        pa->saddr_srvr_in.sin_port = htons(TEST_PORT);

        if (bind(pa->listen_socket, (SOCKADDR *)& pa->saddr_srvr_in, sizeof(pa->saddr_srvr_in)) == SOCKET_ERROR) {
            printf("bind failed with error: %u\n", WSAGetLastError());
            cmd_socket_close(&(pa->listen_socket));
            WSACleanup();
            rc = EXIT_ERROR;
            break;
        }

        // Start listening 
        rc = listen(pa->listen_socket, 100000);
        if (rc == SOCKET_ERROR) {
            printf("listen failed with error: %u\n", WSAGetLastError());
            cmd_socket_close(&(pa->listen_socket));
            WSACleanup();
            rc = EXIT_ERROR;
            break;
        }

        pa->accept_socket = accept(pa->listen_socket, NULL, NULL);

        if (pa->accept_socket == INVALID_SOCKET) {
            printf("accept failed with error: %ld\n", WSAGetLastError());
            cmd_socket_close(&(pa->listen_socket));
            WSACleanup();
            rc = EXIT_ERROR;
            break;;
        }

        cmd_nw_prep_s1(Q_SOC, pa);

        // Wait for connection
        pa->bytes_read = 0;
        pa->this_msg = ob;

        if (pa->p_buffers == NULL)
        {
            printf("bm_get() failed \n");
            cmd_socket_close(&(pa->accept_socket));
            cmd_socket_close(&(pa->listen_socket));
            WSACleanup();
            rc = EXIT_ERROR;
            break;;
        }

        DWORD Flags = 0;
        int s_err;

        trc = WSARecv(pa->accept_socket, pa->p_buffers, BM_IN_NUM_POST,
            &(pa->bytes_read), &Flags,
            &(pa->overlapped_blk), &recv_cr);
        if ((trc == SOCKET_ERROR) && (WSA_IO_PENDING != (s_err = WSAGetLastError()))) {
            printf("WSARecv failed with error: %d\n", s_err);
            rc = EXIT_ERROR;
            break;
        }


        /*
        trc = WSARecv(pa->accept_socket, pa->p_buffers, BM_IN_NUM_POST,
            &(pa->bytes_read), &Flags, NULL, NULL);

        if (trc == SOCKET_ERROR) {
            s_err = WSAGetLastError();
            printf("WSARecv failed with error: %d\n", s_err);
            rc = EXIT_ERROR;
            break;
        }
        */


        break;
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

        pu->un.scmd.pdata = qm_srvr_sckt();

        if (rc = server_init(pu)) break;

        if (rc = qm_task_loop(Q_SOC)) break;

    } while (SUPPORT_FOR_C_EXCEPTIONS);

    return rc;
}

