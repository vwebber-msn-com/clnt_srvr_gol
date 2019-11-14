#pragma once
#include <windows.h>
#include <stdio.h>
#include "command_msgs.h"
#include "buffer_mgmt.h"

typedef BOOL(WINAPI *LPFN_GLPI)(
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
	PDWORD);

typedef struct _sys_blk_t
{
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION	proc_info;
	DWORD logicalProcessorCount = 0;
	DWORD numaNodeCount = 0;
	DWORD processorCoreCount = 0;
	DWORD processorL1CacheCount = 0;
	DWORD processorL2CacheCount = 0;
	DWORD processorL3CacheCount = 0;
	DWORD processorPackageCount = 0;
	PCACHE_DESCRIPTOR Cache;
	LPFN_GLPI	glpi;
} sys_blk_t, *psys_blk_t;

typedef struct _thrd_cb_t
{
    DWORD		aff_mask;
    HANDLE		h_thrd;
    void        *arg;
} thrd_cb_t, *pthrd_cb_t;

typedef struct _sys_init_blk_t
{
    void(__cdecl *thrd_func)(void *);
    DWORD		aff_mask;
} sys_init_blk_t, *psys_init_blk_t;

typedef struct _que_blk_t
{
	ublk_t		q;
	thrd_cb_t   thrd_cb;
    HANDLE      h_event;
    void(__cdecl *thrd_func)(void *);
    struct _bm_mgmt_t   bm_mgmt;
    SECURITY_ATTRIBUTES sec_attributes;
    SECURITY_DESCRIPTOR sec_descriptor;
} que_blk_t, *pque_blk_t;

typedef struct _que_mgmt_t
{
	que_blk_t	qm_ary[Q_CNT];
    HANDLE      process;
    nw_cmds_t   sckt_mgmt;

	
} que_mgmt_t, *pque_mgmt_t;


#ifdef QM_INSTANCE
sys_blk_t		sys_blk;
que_mgmt_t		que_mgmt;
#else
extern sys_blk_t		sys_blk;
extern que_mgmt_t		que_mgmt;
#endif // QM_INSTANCE

int qm_start_thread(QM_SYS_T sys);

int qm_init();

int qm_init_sys(QM_SYS_T sys, psys_init_blk_t sib);

int qm_init_runtime(QM_SYS_T sys);

int qm_retrieve_msg(QM_SYS_T sys, publk_t *po);

int qm_get_msg(QM_SYS_T sys, publk_t *po);

LPWSABUF qm_get_init(QM_SYS_T sys, LPWSABUF bp);

int qm_put_msg(QM_SYS_T sys, publk_t po);

int qm_wait_task(QM_SYS_T sys, publk_t  *pv);

int qm_put_buffer(QM_SYS_T sys, LPWSABUF bp);

int qm_get_buffer(QM_SYS_T sys, LPWSABUF bp);

int qm_get_mtd(QM_SYS_T sys, LPWSABUF *bp);

int qm_post_task(QM_SYS_T sys, publk_t task);

int qm_prep_post_task(QM_SYS_T sys, LPWSABUF pb);

int qm_task_loop(QM_SYS_T sys);

paccept_rb_t qm_srvr_sckt();

pconnect_rb_t qm_clnt_sckt();

typedef enum _SID_SEC_T : uint8
{
    SEC_LO,
    SEC_HI
} SID_SEC_T;

int qm_create_open_sec_descriptor(QM_SYS_T sys, SID_SEC_T sec_level);

void qm_srvr_send_buf(LPWSABUF pb);











