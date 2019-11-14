#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
//#include <ws2spi.h>
#include <stdio.h>

#include "xplatform.h"
#include "gol.h"

#define LOOPBACK_ADDR "128.0.0.2"
#define LOOPBACK_ADDR_CLNT "128.0.0.2"
#define TEST_PORT	80

typedef enum _QM_SYS_T
{
    Q_SOC = 0,
    Q_GOL,
    Q_CNT
} QM_SYS_T;

typedef enum _CM_SYS_CMD_T : uint16
{
    CMS_SHT_DWN = 0,
    CMS_SCKT_RESTART,
    CMS_QT_DISC,
    CMS_QT_SHUTDWN,
    CMS_SCKT_OP,
    CMS_SCKT_SEND,
    CMS_CNT
} CM_SYS_CMD_T;

typedef enum _CM_ENV_T : uint8
{
    CME_DATA_ATT,
    CME_DATA_SEP
} CM_ENV_T;

typedef struct _cmd_bflds_t
{
    uint16      cm_env : 2, used : 1;
} cmd_bflds_t, *pcmd_bflds_t;

typedef struct _cmd_fields_t
{
#pragma pack(push, SLN_PACK)
    QM_SYS_T		cmd;
    uint16          sub_cmd;
    cmd_bflds_t     cm_bflds;
#pragma pack(pop)
} cmd_fields_t, *pcmd_fields_t;

typedef struct _cmd_sckt_t
{
#pragma pack(push, SLN_PACK)
    void            *pdata;
    uint32          data_len;
    CM_SYS_CMD_T	cmd_sb;
#pragma pack(pop)
} cmd_sckt_t, *pcmd_sckt_t;

typedef union _cmd_fld2_t
{
    cmd_sckt_t      scmd;
    gol_cmd_t       gcmd;
} cmd_fld2_t, pcmd_fld2_t;

typedef struct _accept_rb_t
{
    WSAOVERLAPPED	overlapped_blk;
    DWORD			bytes_read;
    SOCKET			listen_socket;
    SOCKET			accept_socket;
    sockaddr_in		saddr_srvr_in;
    HANDLE			h_event;
    LPWSABUF		p_buffers;
    DWORD			cnt_buffers;
    DWORD			cr_context;
    struct _ublk_t *this_msg;
} accept_rb_t, *paccept_rb_t;

typedef struct _connect_rb
{
    WSAOVERLAPPED	overlapped_blk;
    WSADATA			wsa_data;
    DWORD			bytes_read;
    DWORD			bytes_sent;
    SOCKET			connect_socket;
    sockaddr_in     saddr_srvr_in;
    sockaddr_in     saddr_srvr_out;
    HANDLE			h_event;
    HANDLE			h_thread;
    LPWSABUF		p_buffers;
    DWORD			cnt_buffers;
    DWORD			cr_context;
    struct _ublk_t *this_msg;
} connect_rb_t, *pconnect_rb_t;

typedef union _nw_cmds_t
{
    accept_rb_t  ao;
    connect_rb_t co;
    BOOL         srvr;
} nw_cmds_t;

typedef struct _oblk_t
{
#pragma pack(push,SLN_PACK)
    struct _ublk_t	*pob;
    cmd_fields_t    cmd_fields;
    
#pragma pack(pop)
} oblk_t, *poblk_t;

typedef struct _ublk_t
{
#pragma pack(push, SLN_PACK)
    _oblk_t			ob;
    cmd_fld2_t       un;
#pragma pack(pop)
} ublk_t, *publk_t;

void cmd_nw_prep(QM_SYS_T sys, paccept_rb_t po);
void cmd_nw_prep_s1(QM_SYS_T sys, paccept_rb_t po);

void cmd_socket_close(SOCKET *s);
void cmd_msg_prep_post(QM_SYS_T src, QM_SYS_T dst, uint8 *buf);
int  cmd_init();
void cmd_cl_prep(QM_SYS_T sys, pconnect_rb_t pa);
int  cmd_server_send(QM_SYS_T src, LPWSABUF pb);

void soc_msg_process(publk_t pu);



