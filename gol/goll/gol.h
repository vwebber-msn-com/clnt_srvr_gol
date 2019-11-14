#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
//#include <ws2spi.h>
#include <stdio.h>
#include "xplatform.h"
#include "command_msgs.h"
//#include "buffer_mgmt.h"

////////////////////////////////////////////////////////////////////////////////////////
// PROGRAM MACROS
////////////////////////////////////////////////////////////////////////////////////////

// Macros Controlling Execution
enum board_sz { N = 8 };
#define TURN(a) (a=a^0b00000001)
#define PK_TURN(a) (a^0b00000001)
#define BRD_MAX N*N
#define PROG_ITERS 3

// Board Views
typedef struct {
    uint8 row : N;
} board_row_bit_t;

// Basic Addressable Unit (BAU) structure which must be 
// changed for different hardware architectures
// which have a different BAU
typedef struct {
    uint8 nib0 : 4, nib1 : 4;
} board_bau_t;

////////////////////////////////////////////////////////////////////////////////////////
// GOL OBJECT MEMBERS
////////////////////////////////////////////////////////////////////////////////////////

typedef union {
    board_row_bit_t			board_bit[N];
    unsigned __int64		board_raw;
    uint8 			board_type_a[N];
    board_bau_t				board_bau_a[N];
} board_t, *pboard_t;

typedef struct _table_t
{
    board_t			board_a[2];
    unsigned __int64	b_mask;
    uint16	b_i;
    uint16  id;
} table_t, *ptable_t;

typedef enum _CM_GOL_CMD : uint16
{
    CMG_DSPLY,
    CMG_STRT_RUN,
    CMG_STEP,
    CMG_FLIP,
    CMG_INIT_TOAD,
    CMG_INIT_BEACON,
    CMG_INIT_RANDOM,
    CMG_INIT_BLINKER,
    CMG_RUN,
    CMG_STOP
} CM_GOL_CMD;

typedef struct _gol_cmd_t
{
#pragma pack(push, SLN_PACK)
    void        *pdata;
    uint32      data_len;
    CM_GOL_CMD  sub_cmd;
#pragma pack(pop)
} gol_cmd_t, *pgol_cmd_t;

typedef struct _gol_cbuf_run_t
{
    uint64          dsply_i;
} gol_cbuf_run_t, *pgol_cbuf_run_t;

typedef struct _gol_cbuf_dsply_t
{
    board_t     board;
    uint16      id;
} gol_cbuf_dsply_t, *pgol_cbuf_dsply_t;

typedef union _cmd_buf_ut
{
    gol_cbuf_dsply_t    dsply;
    gol_cbuf_run_t      run;
} cmd_buf_ut, pcmd_buf_ut;


typedef struct _gol_cmd_buf_t
{
#pragma pack(push, SLN_PACK)
    CM_GOL_CMD      gol_cmd;
    cmd_buf_ut      cmd_u;
#pragma pack(pop)
} gol_cmd_buf_t, *pgol_cmd_buf_t;

void gol_thrd(void *p);

void gol_fill_cmd_buf(CM_GOL_CMD cmd, pgol_cmd_buf_t pc);


////////////////////////////////////////////////////////////////////////////////////////
// TABLE MANAGEMENT OBJECTS
////////////////////////////////////////////////////////////////////////////////////////

typedef struct _gol_tbl_mgmt_t
{
    table_t     tm[GM_INSTANCE_CNT];
} gol_tbl_mgmt_t, *pgol_tbl_mgmt_t;

#ifdef GOL_SRVR_INSTANCE

gol_tbl_mgmt_t gol_tbl_mgmt;

#else

extern gol_tbl_mgmt_t gol_tbl_mgmt;

#endif // GOL_SRVR_INSTANCE

#ifdef GOL_CLNT_INSTANCE

//gol_tbl_mgmt_t gol_tbl_mgmt;

#else

//extern gol_tbl_mgmt_t gol_tbl_mgmt;

#endif // GOL_SRVR_INSTANCE