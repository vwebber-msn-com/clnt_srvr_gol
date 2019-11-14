
#include "stdafx.h"

#define GOL_SRVR_INSTANCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pch.h"
#include "command_msgs.h"
#include "gol.h"
#include "que_mgmt.h"

#define WIN32_LEAN_AND_MEAN

////////////////////////////////////////////////////////////////////////////////////////
// OBJECT UTILITIES
////////////////////////////////////////////////////////////////////////////////////////

board_t BLINKER = { 0,0,0,0x10,0x10,0x10,0,0 };
board_t TOAD = { 0,0,0,0x38,0x1C,0,0,0 };
board_t BEACON = { 0,0x60,0x60,0x18,0x18,0,0,0 };

uint8 lookup_bit_count[0x10] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };

////////////////////////////////////////////////////////////////////////////////////////
// GENERATION MASK OBJECT
////////////////////////////////////////////////////////////////////////////////////////

#define GEN_MASK_C0   (0x83)
#define GEN_MASK_C0T  (0x82)
#define GEN_MASK_C1   (0x07)
#define GEN_MASK_C1T  (0x05)
#define GEN_MASK_C7   (0xC1)
#define GEN_MASK_C7T  (0x41)


////////////////////////////////////////////////////////////////////////////////////////
// GOL OBJECT FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////


void rotate_bit_mask(ptable_t pt)
{
    pt->b_mask = (pt->b_mask == 0x8000000000000000 ? 1 : pt->b_mask << 1);
}

void rotate_gen_mask_obj(pboard_t pt, int i)
{
    int row_base = ((N + ((i / N) - 1)) % N);

    switch (i%N)
    {
    case 0:
        pt->board_raw = 0;
        for (int j = 0; j < 3; j++)
        {
            if (j != 1) pt->board_type_a[row_base] = GEN_MASK_C0;
            else pt->board_type_a[row_base] = GEN_MASK_C0T;
            row_base = (++row_base) % N;
        }
        break;

    case 1:
        pt->board_raw = 0;
        for (int j = 0; j < 3; j++)
        {
            if (j != 1) pt->board_type_a[row_base] = GEN_MASK_C1;
            else pt->board_type_a[row_base] = GEN_MASK_C1T;
            row_base = (++row_base) % N;
        }
        break;

    case 7:
        pt->board_raw = 0;
        for (int j = 0; j < 3; j++)
        {
            if (j != 1) pt->board_type_a[row_base] = GEN_MASK_C7;
            else pt->board_type_a[row_base] = GEN_MASK_C7T;
            row_base = (++row_base) % N;
        }
        break;

    default:
        pt->board_raw = pt->board_raw << 1;
        break;

    }

}

void init_gen_mask(pboard_t pb)
{
    pb->board_raw = 0;
    pb->board_type_a[7] = GEN_MASK_C0;
    pb->board_type_a[0] = GEN_MASK_C0T;
    pb->board_type_a[1] = GEN_MASK_C0;
}

int init_board(pboard_t pb, const char *pattern)
{
    int rc = EXIT_ERROR;
    // 0x8000000000000000;
    if (_stricmp(pattern, "RANDOM") == 0) pb->board_raw = 0x8034099556782305;
    else if (_stricmp(pattern, "TOAD") == 0) pb->board_raw = TOAD.board_raw;
    else if (_stricmp(pattern, "BLINKER") == 0) pb->board_raw = BLINKER.board_raw;
    else if (_stricmp(pattern, "BEACON") == 0) pb->board_raw = BEACON.board_raw;
    else rc = EXIT_ERROR;

    return rc;

}

int init_table(ptable_t pt, const char *pattern, uint16 id)
{
    int rc = EXIT_ERROR;
    pt->b_i = 0;
    pt->b_mask = 1;
    pt->id = id;
    rc = init_board(&(pt->board_a[pt->b_i]), pattern);
    return rc;
}

void print_board_raw(pboard_t pb)
{
    uint64 mask = 1;

    printf("\n");

    for (uint8 r = 0; r < N; ++r)
    {
        for (uint8 c = 0; c < N; ++c)
        {
            (pb->board_raw & mask) ? printf("X ") : printf(". ");
            mask = mask << 1;
        }
        printf("\n");
    }

}

void print_board(ptable_t pt)
{
    uint64 mask = 1;

    printf("\n");

    for (uint8 r = 0; r < N; ++r)
    {
        for (uint8 c = 0; c < N; ++c)
        {
            (pt->board_a[pt->b_i].board_raw & mask) ? printf("X ") : printf(". ");
            mask = mask << 1;
        }
        printf("\n");
    }

}

void set_bit(ptable_t pt, uint8 cnt)
{
    switch (cnt)
    {
    case 3:
        pt->board_a[PK_TURN(pt->b_i)].board_raw = (pt->board_a[PK_TURN(pt->b_i)].board_raw) ^ pt->b_mask;
        break;
    default:
        break;
    };

}

void reset_bit(ptable_t pt, uint8 cnt)
{
    switch (cnt)
    {
    case 3:
    case 2:
        break;
    default:
        pt->board_a[PK_TURN(pt->b_i)].board_raw = pt->board_a[PK_TURN(pt->b_i)].board_raw & ~(pt->b_mask);
        break;
    };
}

uint8 count_set_bits_row(board_t set, uint8 row)
{
    uint8 c_total = 0;

    c_total += lookup_bit_count[set.board_bau_a[row].nib0];
    c_total += lookup_bit_count[set.board_bau_a[row].nib1];

    return c_total;
}

uint8 count_set_bits(board_t set, uint8 i)
{

    uint8 c_total = 0;
    uint8 row_base = ((N + ((i / N) - 1)) % N);

    for (uint8 j = 0; j < 3; j++)
    {
        c_total += count_set_bits_row(set, row_base);
        row_base = (++row_base) % N;
    }

    return c_total;
}

void generation(ptable_t pt)
{
    uint8 i = 0;
    uint8 c_total = 0;
    board_t set_bits;
    board_t gen_mask;

    pt->board_a[PK_TURN(pt->b_i)].board_raw = pt->board_a[pt->b_i].board_raw;

    init_gen_mask(&gen_mask);

    while (i < (N*N))
    {
        set_bits.board_raw = 0;
        set_bits.board_raw = pt->board_a[pt->b_i].board_raw & gen_mask.board_raw;

        c_total = count_set_bits(set_bits, i);

        (pt->board_a[pt->b_i].board_raw & pt->b_mask) ? reset_bit(pt, c_total) : set_bit(pt, c_total);
        rotate_bit_mask(pt);
        rotate_gen_mask_obj(&gen_mask, ++i);
    }
    TURN(pt->b_i);
}


void gol_init_all(void *pd, uint32 cnt)
{
    // get base of table array
    for (int i = 0; i < cnt; ++i)
    {
        init_table(&(gol_tbl_mgmt.tm[i]), "TOAD", i + 1);
    }
}

void gol_start_all(pgol_cmd_t pg, uint32 cnt)
{
    // Post reoccuring msg to step all and display all games

    for (int i = 0; i < cnt; ++i)
    {
        // Get a nw cmd buffer
        for (int j = 0; j < 20; ++j)
        {
            // Fill buffer with display data
        }
    }

}

void gol_display_all( pgol_cmd_buf_t pg, uint32 cnt)
{
    WSABUF wb;

    // Display info from all commands in buffer
    for (int i = 0; i < cnt; ++i)
    {
        printf("Board ID: %d \n", pg[i].cmd_u.dsply.id);
        print_board_raw(&(pg[i].cmd_u.dsply.board));

    }

    wb.buf = (CHAR *)pg;
    wb.len = sizeof(bm_buf);
    bm_put_buffer(Q_SOC, &wb);

}

void gol_step_all()
{
    // Step all active game tables in game table array



}

#define GM_RUN_ITER_CNT 1

void gol_msg_process(publk_t pu)
{
    uint64 run_i;
    WSABUF wb;


    switch (pu->un.gcmd.sub_cmd)
    {
    case CMG_STRT_RUN:
        gol_init_all(NULL, GM_INSTANCE_CNT);
        // Send CMG_RUN Msg to Keep it Going
        pu->un.gcmd.sub_cmd = CMG_RUN;
        ((pgol_cmd_buf_t)(pu->un.gcmd.pdata))->cmd_u.run.dsply_i = 0;
        qm_post_task(Q_GOL, pu);
        break;

    case CMG_RUN:
        run_i = ((pgol_cmd_buf_t)(pu->un.gcmd.pdata))->cmd_u.run.dsply_i;

        qm_get_buffer(Q_SOC, &wb);

        for (uint32 i = 0; i < BM_BUF_CMD_CNT; ++i)
        {
            for (uint8 j = 0; j < GM_RUN_ITER_CNT; ++j)
            {
                generation(&(gol_tbl_mgmt.tm[i + run_i]));
            }

            ((pgol_cmd_buf_t)wb.buf)[i].cmd_u.dsply.board.board_raw 
                = gol_tbl_mgmt.tm[i + run_i].board_a[gol_tbl_mgmt.tm[i + run_i].b_i].board_raw;

            ((pgol_cmd_buf_t)wb.buf)[i].cmd_u.dsply.id = gol_tbl_mgmt.tm[i + run_i].id;
        }

        qm_srvr_send_buf(&wb );

        if ((run_i + BM_BUF_CMD_CNT) < GM_INSTANCE_CNT)
        {
            ((pgol_cmd_buf_t)(pu->un.gcmd.pdata))->cmd_u.run.dsply_i += BM_BUF_CMD_CNT;
            qm_post_task(Q_GOL, pu);
        }
        else
        {
            ((pgol_cmd_buf_t)(pu->un.gcmd.pdata))->cmd_u.run.dsply_i = 0;
            qm_post_task(Q_GOL, pu);
//            qm_put_msg(Q_SOC, pu);
        }

        break;

    case CMG_DSPLY:
        gol_display_all((pgol_cmd_buf_t)(pu->un.gcmd.pdata), BM_BUF_CMD_CNT);
        qm_put_msg(Q_SOC, pu);
        break;
    }
}

void gol_fill_cmd_buf(CM_GOL_CMD cmd, pgol_cmd_buf_t pc)
{
    pc->gol_cmd = cmd;
    pc->cmd_u.run.dsply_i = 0;
}


void gol_thrd(void *p)
{

    qm_init_runtime(Q_GOL);

    qm_task_loop(Q_GOL);

}


