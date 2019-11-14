
#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include "process.h"
#include <AclAPI.h>

#define QM_INSTANCE
#include "que_mgmt.h"

void gol_msg_process(publk_t pu);

int qm_post_task(QM_SYS_T sys, publk_t task)
{
    publk_t ptgt = NULL;
    int rc = NOERROR;

    // Resource protection is done by 
    // restricting use to singletons running on non-preemptive threads
    // affinized to the same logical core

    // Restricted to singleton use
    // Post task

    ptgt = &(que_mgmt.qm_ary[sys].q);
    while ((ptgt->ob.pob) != NULL)
    {
        ptgt = ptgt->ob.pob;
    }

    task->ob.pob = NULL;
    ptgt->ob.pob = task;

    if (!SetEvent(que_mgmt.qm_ary[sys].h_event))
    {
        rc = EXIT_ERROR;
        printf("SetEvent failed with error: %d\n", WSAGetLastError());
    }

    return rc;

}

int qm_prep_post_task(QM_SYS_T sys, LPWSABUF pb)
{
    publk_t pu;

    qm_get_msg(Q_SOC, &pu);

    pu->un.gcmd.sub_cmd = ((pgol_cmd_buf_t)(pb->buf))->gol_cmd;

    pu->un.gcmd.pdata = pb->buf;

    qm_post_task(Q_GOL, pu);

    return NOERROR;

}


int qm_wait_task(QM_SYS_T sys, publk_t *pv)
{
    int rc = NOERROR, hope = TRUE;
    const pque_blk_t pq = &(que_mgmt.qm_ary[sys]);
    BOOL alert = TRUE;

    if (sys == Q_GOL)
    {
        alert = TRUE;
    }

    do
    {
        // Wait on que event
        switch (WaitForMultipleObjectsEx(1, &(pq->h_event), FALSE, /*500*/INFINITE, alert))
        {
        case WAIT_OBJECT_0:

            // System processing
            if (qm_retrieve_msg(sys, pv))
            {
                rc = EXIT_ERROR;
            }

            hope = FALSE;
            break;

        case WAIT_ABANDONED_0:
            rc = EXIT_ERROR;
            hope = FALSE;
            break;

        case WAIT_TIMEOUT:
            break;

        case WAIT_IO_COMPLETION:
            break;

        case WAIT_FAILED:
            rc = EXIT_ERROR;
            hope = FALSE;
            break;

        default:
            rc = EXIT_ERROR;
            hope = FALSE;
            break;
        }
    } while (hope);

    // return so qblk can be read
    return rc;
}

int qm_retrieve_msg(QM_SYS_T sys, publk_t *po)
{
    if (((*po = que_mgmt.qm_ary[sys].q.ob.pob) != NULL)
        )
    {
        que_mgmt.qm_ary[sys].q.ob.pob = (*po)->ob.pob;
    }
    // Reset Event if Manual
    return NOERROR;
}

// Helper function to count set bits in the processor mask.
DWORD CountSetBits(ULONG_PTR bitMask)
{
    DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
    DWORD bitSetCount = 0;
    ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
    DWORD i;

    for (i = 0; i <= LSHIFT; ++i)
    {
        bitSetCount += ((bitMask & bitTest) ? 1 : 0);
        bitTest /= 2;
    }

    return bitSetCount;
}


int qm_get_proc_info()
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION proc_info_ary[16];
    DWORD returnLength = 0;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = proc_info_ary;
    DWORD byteOffset = 0;
    int rc = NOERROR;

    sys_blk.glpi = (LPFN_GLPI)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),
        "GetLogicalProcessorInformation");

    if (!(sys_blk.glpi((PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)proc_info_ary, &returnLength)))
    {
        printf("get proc info failed with error: %u\n", WSAGetLastError());
        return EXIT_ERROR;
    }


    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
    {
        switch (ptr->Relationship)
        {
        case RelationNumaNode:
            // Non-NUMA systems report a single record of this type.
            sys_blk.numaNodeCount++;
            break;

        case RelationProcessorCore:
            sys_blk.processorCoreCount++;

            // A hyperthreaded core supplies more than one logical processor.
            sys_blk.logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
            break;

        case RelationCache:
            // Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
            sys_blk.Cache = &ptr->Cache;
            if (sys_blk.Cache->Level == 1)
            {
                sys_blk.processorL1CacheCount++;
            }
            else if (sys_blk.Cache->Level == 2)
            {
                sys_blk.processorL2CacheCount++;
            }
            else if (sys_blk.Cache->Level == 3)
            {
                sys_blk.processorL3CacheCount++;
            }
            break;


        case RelationProcessorPackage:
            // Logical processors share a physical package.
            sys_blk.processorPackageCount++;
            break;

        default:
            break;
        }
        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }

    return rc;

}

int qm_set_thread_aff(HANDLE h_thread, DWORD affinity_mask)
{
    DWORD_PTR pdw;

    pdw = SetThreadAffinityMask(h_thread, affinity_mask);

    if (pdw == FALSE) {
        printf("get proc info failed with error: %u\n", WSAGetLastError());
        return EXIT_ERROR;
    }

    return	NOERROR;

}

int qm_start_thread(QM_SYS_T sys)
{
    int rc = NOERROR;

    pthrd_cb_t pt = &(que_mgmt.qm_ary[sys].thrd_cb);
    pque_blk_t pq = &(que_mgmt.qm_ary[sys]);

    pt->h_thrd = (HANDLE)_beginthread(pq->thrd_func, 0,
        pt->arg);

    if (pt->h_thrd == (HANDLE)HFILE_ERROR) rc = EXIT_ERROR;

    return rc;

}

int qm_init()
{
    int rc = NOERROR;


    memset((void *)(&que_mgmt), 0, sizeof(que_mgmt));
    que_mgmt.process = GetCurrentProcess();
    if (SetPriorityClass(que_mgmt.process, REALTIME_PRIORITY_CLASS) == ERROR)
    {
        rc = EXIT_ERROR;
    }

    return rc;
}

int qm_init_sys(QM_SYS_T sys, psys_init_blk_t sib)
{
    int rc = NOERROR;

    que_mgmt.process = GetCurrentProcess();
    if (SetPriorityClass(que_mgmt.process, REALTIME_PRIORITY_CLASS) == ERROR)
    {
        rc = EXIT_ERROR;
    }

    que_mgmt.qm_ary[sys].thrd_cb.aff_mask = sib->aff_mask;
    que_mgmt.qm_ary[sys].thrd_func = sib->thrd_func;

    return rc;
}

int qm_init_runtime(QM_SYS_T sys)
{
    int rc = NOERROR;
    pthrd_cb_t p = &(que_mgmt.qm_ary[sys].thrd_cb);

    p->h_thrd = GetCurrentThread();

    do
    {

        if (qm_create_open_sec_descriptor(sys, SEC_LO))
        {
            rc = EXIT_ERROR;
            break;
        }

        if (!(SetThreadAffinityMask(p->h_thrd, p->aff_mask)))
        {
            rc = EXIT_ERROR;
            break;
        }

        if (!(SetThreadPriority(p->h_thrd, THREAD_PRIORITY_LOWEST)))
        {
            rc = EXIT_ERROR;
            break;
        }

    } while (SUPPORT_FOR_C_EXCEPTIONS);

    return rc;

}

LPWSABUF qm_get_init(QM_SYS_T sys, LPWSABUF bp)
{
    bp->buf = (CHAR *)(&(que_mgmt.qm_ary[sys].bm_mgmt.bm_ibuf));
    bp->len = sizeof(gol_cmd_buf_t);
    return (bp);
}

int qm_put_buffer(QM_SYS_T sys, LPWSABUF bp)
{
    bm_put_buffer(1, bp);
    return NOERROR;
}

int qm_get_buffer(QM_SYS_T sys, LPWSABUF bp)
{
    bp =
        bm_get_buffer(
            &(que_mgmt.qm_ary[sys].bm_mgmt.bm_buf_mgmt),
            bp);

    return NOERROR;
}

int qm_get_msg(QM_SYS_T sys, publk_t *po)
{
    return bm_get_msg(
        &(que_mgmt.qm_ary[sys].bm_mgmt.bm_msg_mgmt),
        po);
}

int qm_put_msg(QM_SYS_T sys, publk_t po)
{
    bm_put_msg(po);
    return NOERROR;
}

int qm_get_mtd(QM_SYS_T sys, LPWSABUF *bp)
{
    bm_get_mtd(
        &(que_mgmt.qm_ary[sys].bm_mgmt.bm_mtd_mgmt),
        bp);

    return NOERROR;
}

int qm_task_loop(QM_SYS_T sys)
{
    int rc = NOERROR;
    publk_t pu;
    BOOL loop_active = true;

    while (loop_active)
    {
        // Wait on que event
        if (qm_wait_task(sys, &pu))
        {
            rc = EXIT_ERROR;
            loop_active = FALSE;
            break;
        }
        else
        {
            // no error and null, sys processing was done
            if (pu != NULL)
            {
                // What other commands can the socket que get
                switch (sys)
                {
                case Q_SOC:
                    soc_msg_process(pu);
                    break;
                case Q_GOL:
                    gol_msg_process(pu);
                    break;
                default:
                    loop_active = FALSE;
                    break;
                }
            }
        }
    } 
    return rc;
}

paccept_rb_t qm_srvr_sckt()
{
    return (&(que_mgmt.sckt_mgmt.ao));
}

pconnect_rb_t qm_clnt_sckt()
{
    return (&(que_mgmt.sckt_mgmt.co));
}

void qm_srvr_send_buf(LPWSABUF pb)
{
    publk_t pu;

    qm_get_msg(Q_SOC, &pu);

    pu->un.scmd.cmd_sb = CMS_SCKT_SEND;
    pu->un.scmd.pdata = pb->buf;

    qm_post_task(Q_SOC, pu);

}

int qm_create_open_sec_descriptor(QM_SYS_T sys, SID_SEC_T sec_level)
{
#define SID_SIZE 1024 //sizeof(SECURITY_DESCRIPTOR)
    //DWORD sid_size = SID_SIZE;
    //CHAR sid[SID_SIZE];
    PSID sid=NULL;
    EXPLICIT_ACCESS ea = { 0 };
    PACL acl = NULL;
    PSECURITY_DESCRIPTOR sd = NULL;
    int rc = NOERROR;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;

    switch (sec_level)
    {
    case SEC_LO:
        do
        {
            
            sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,
                SECURITY_DESCRIPTOR_MIN_LENGTH);
            if (NULL == sd)
            {
                printf("LocalAlloc Error %u\n", GetLastError());
                break;
            }

            if (InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION) == 0)
            {
                printf("Init SD failed with error: %ld\n", WSAGetLastError());
                rc = EXIT_ERROR;
                break;
            }

            if (SetSecurityDescriptorDacl(sd, TRUE, NULL, FALSE) == 0)
            {
                printf("Set Dacl failed with error: %ld\n", WSAGetLastError());
                rc = EXIT_ERROR;
                break;
            }
            

            que_mgmt.qm_ary[sys].sec_attributes.bInheritHandle = TRUE;
            que_mgmt.qm_ary[sys].sec_attributes.lpSecurityDescriptor = sd;
            que_mgmt.qm_ary[sys].sec_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);

            que_mgmt.qm_ary[sys].h_event = CreateEventEx( &(que_mgmt.qm_ary[sys].sec_attributes),
                NULL, 0, EVENT_ALL_ACCESS /*STANDARD_RIGHTS_ALL*/);

            if ((que_mgmt.qm_ary[sys].h_event) == NULL)
            {
                rc = EXIT_ERROR;
                printf("Create Event failed with error: %ld\n", WSAGetLastError());
                break;
            }
        } while (SUPPORT_FOR_C_EXCEPTIONS);

        break;

    case SEC_HI:

        do
        {            
            // Create a well-known SID for the Everyone group.
            if (!AllocateAndInitializeSid(&SIDAuthWorld, 1,
                SECURITY_WORLD_RID,
                0, 0, 0, 0, 0, 0, 0,
                &sid))
            {
                printf("AllocateAndInitializeSid Error %u\n", GetLastError());
                break;
            }
            

            ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
            //ea.grfAccessPermissions = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
            ea.grfAccessPermissions = GENERIC_ALL;
            ea.grfAccessMode = SET_ACCESS;
            ea.grfInheritance = NO_INHERITANCE;//CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
            ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
            ea.Trustee.ptstrName = (LPTSTR)sid;

            if (SetEntriesInAcl(1, &ea, NULL, &acl) != ERROR_SUCCESS)
            {
                printf("SetAcl failed with error: %ld\n", WSAGetLastError());
                rc = EXIT_ERROR;
                break;
            }

            // Initialize a security descriptor.  
            sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,
                SECURITY_DESCRIPTOR_MIN_LENGTH);
            if (NULL == sd)
            {
                printf("LocalAlloc Error %u\n", GetLastError());
                break;
            }

            if (InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION) == 0)
            {
                printf("Init SD failed with error: %ld\n", WSAGetLastError());
                rc = EXIT_ERROR;
                break;
            }

            if (SetSecurityDescriptorDacl(sd, TRUE, acl, TRUE) == 0)
            {
                printf("Set Dacl failed with error: %ld\n", WSAGetLastError());
                rc = EXIT_ERROR;
                break;
            }

            que_mgmt.qm_ary[sys].sec_attributes.bInheritHandle = TRUE;
            que_mgmt.qm_ary[sys].sec_attributes.lpSecurityDescriptor = &sd;
            que_mgmt.qm_ary[sys].sec_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);

            que_mgmt.qm_ary[sys].h_event = CreateEventEx(&(que_mgmt.qm_ary[sys].sec_attributes),
                NULL, 0, STANDARD_RIGHTS_ALL);

            if ((que_mgmt.qm_ary[sys].h_event) == NULL)
            {
                rc = EXIT_ERROR;
                printf("Create Event failed with error: %ld\n", WSAGetLastError());
                break;
            }

        } while (SUPPORT_FOR_C_EXCEPTIONS);

        break;
    }


    if (sid) FreeSid(sid);
    if (acl) LocalFree(acl);

    que_mgmt.qm_ary[sys].sec_attributes.lpSecurityDescriptor = NULL;

    return rc;

}




