/*******************************************************************************
 *  Copyright @ 2023
 *
 *  tuner&demod control extention
 *
 *
 *******************************************************************************/

/**
 * @brief   tuner&demod control extention
 * @file    stbhwtun_ex.c
 * @date    2023-01-29
 */

#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/poll.h>

//---#includes for this file---------------------------------------------------


#include "frontend.h"

/* third party header files */

#include "techtype.h"

#include "stbheap.h"
#include "stbhwos.h"
#include "stbos_timer.h"

#include "fsm_base.h"

#include "stbhwtun.h"
#include "stbhwtun_inner.h"
#include "stbhwtun_ex.h"

#include "cert_log.h"

#define TAG "TUNER"

//---external function declare---
extern E_TUNER_EVENT GetTunerLockStatus(U32BIT frontend_fd);
extern BOOLEAN StartTune(S_TUNER_STATUS *tstatus);
extern U8BIT* GetSysTypeDebugString(E_STB_TUNE_SYSTEM_TYPE sys_type);
extern BOOLEAN SetFeProperty(int fe_fd, E_STB_TUNE_SYSTEM_TYPE tuned_sys_type);



//static STATE *sg_cur_state_ptr = NULL;

// Local Structure Define
typedef struct
{
    U32BIT msg_type;
    U32BIT msg_event;
    U8BIT path;
    S_TUNER_STATUS *tstatus;
} STRU_TIMEOUT_MSG;

// Local Function Declare
static FSM_INSTANCE **sg_tune_fsm_ptr_array     = NULL;
static void **sg_tune_task_msg_queue_ptr_array  = NULL;
static void **sg_tune_task_ptr_array            = NULL;
static Timer_s sg_tune_fsm_timer;
static STRU_TIMEOUT_MSG sg_timeout_msg;
static U32BIT sg_start_time = 0;

//----------Local FSM Function Declare------------
static char* tune_fsm_GetMsgTypeString(U32BIT type);
static char* tune_fsm_GetMsgEventString(U32BIT type, U32BIT event);
static char* tune_fsm_GetCurStateString(U32BIT state);
static void* tune_fsm_task(void *param);
static BOOLEAN _fsm_send_msg(U8BIT path, U32BIT type, U32BIT event, BOOLEAN free_para1, void *param1_ptr, BOOLEAN free_para2, void *param2_ptr);
static void _set_tstatus_state(S_TUNER_STATUS *tstatus, E_TUNER_STATE state);

static void _add_fsm_timer(ENUM_TIMERMODE timermode, U32BIT interval_ms, U32BIT timeout_msg_type, U32BIT timeout_msg_event, U8BIT path, S_TUNER_STATUS *tstatus);
static void _send_fsm_timeout_msg(void *arg);
static void _restart_fsm_timer();
static void _delete_fsm_timer();

static BOOLEAN _check_hw_lock_status(int frontend_fd, BOOLEAN *locked);

//----------FSM Function Declare-------------
static BOOLEAN default_state_enter(void *param_ptr)
{
    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;

    return fsm_DefaultStateEnterFunc(sg_tune_fsm_ptr_array[msg_ptr->path], param_ptr);
}

static BOOLEAN default_state_exit(void *param_ptr)
{
    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;

    return fsm_DefaultStateExitFunc(sg_tune_fsm_ptr_array[msg_ptr->path], param_ptr);
}

// state 1: TUNER_IDLE
static BOOLEAN idle_state_enter(void *param_ptr);
static BOOLEAN idle_state_exit(void *param_ptr);


// 1.1 target state: TUNER_TUNING
static BOOLEAN idle_to_tuning_check(void *param_ptr);
static BOOLEAN idle_to_tuning_transition(void *param_ptr);

static STATE_TRANSITION_TABLE idle_state_trans_tbl[] = {
        {TUNER_TUNING, idle_to_tuning_check, idle_to_tuning_transition},
        {INVALID_STATE, NULL, NULL}
    };

// state 2: TUNER_TUNING
static BOOLEAN tuning_state_enter(void *param_ptr);
static BOOLEAN tuning_state_exit(void *param_ptr);

// 2.1 target state: STATE_TUNER_TRACKING
static BOOLEAN tuning_to_tracking_check(void *param_ptr);
static BOOLEAN tuning_to_tracking_transition(void *param_ptr);

// 2.2 target state: STATE_TUNER_RELOCKING
static BOOLEAN tuning_to_relocking_check(void *param_ptr);
static BOOLEAN tuning_to_relocking_transition(void *param_ptr);

// 2.3 target state: STATE_TUNER_STOPPING
static BOOLEAN tuning_to_stopping_check(void *param_ptr);
static BOOLEAN tuning_to_stopping_transition(void *param_ptr);

// 2.4 target state: STATE_TUNER_TUNING
static BOOLEAN tuning_to_tuning_check(void *param_ptr);
static BOOLEAN tuning_to_tuning_transition(void *param_ptr);


static STATE_TRANSITION_TABLE tuning_state_trans_tbl[] = {
        {STATE_TUNER_TRACKING, tuning_to_tracking_check, tuning_to_tracking_transition},
        {STATE_TUNER_RELOCKING, tuning_to_relocking_check, tuning_to_relocking_transition},
        {STATE_TUNER_STOPPING, tuning_to_stopping_check, tuning_to_stopping_transition},
        {STATE_TUNER_TUNING, tuning_to_tuning_check, tuning_to_tuning_transition},
        {INVALID_STATE, NULL, NULL}
    };


// state 3: TUNER_TRACKING
static BOOLEAN tracking_state_enter(void *param_ptr);
static BOOLEAN tracking_state_exit(void *param_ptr);

// 3.1 target state: STATE_TUNER_STOPPING
static BOOLEAN tracking_to_stopping_check(void *param_ptr);
static BOOLEAN tracking_to_stopping_transition(void *param_ptr);

// 3.2 target state: STATE_TUNER_TUNING
static BOOLEAN tracking_to_tuning_check(void *param_ptr);
static BOOLEAN tracking_to_tuning_transition(void *param_ptr);

// 3.3 target state: STATE_TUNER_RELOCKING
static BOOLEAN tracking_to_relocking_check(void *param_ptr);
static BOOLEAN tracking_to_relocking_transition(void *param_ptr);

// 3.4 target state: STATE_TUNE_TRACKING
static BOOLEAN tracking_to_tracking_check(void *param_ptr);
static BOOLEAN tracking_to_tracking_transition(void *param_ptr);


static STATE_TRANSITION_TABLE tracking_state_trans_tbl[] = {
        {STATE_TUNER_STOPPING, tracking_to_stopping_check, tracking_to_stopping_transition},
        {STATE_TUNER_TUNING, tracking_to_tuning_check, tracking_to_tuning_transition},
        {STATE_TUNER_RELOCKING, tracking_to_relocking_check, tracking_to_relocking_transition},
        {STATE_TUNER_TRACKING, tracking_to_tracking_check, tracking_to_tracking_transition},
        {INVALID_STATE, NULL, NULL}
    };


// state 4: STATE_TUNER_RELOCKING
static BOOLEAN relocking_state_enter(void *param_ptr);
static BOOLEAN relocking_state_exit(void *param_ptr);

// 4.1 target state: STATE_TUNER_STOPPING
static BOOLEAN relocking_to_stopping_check(void *param_ptr);
static BOOLEAN relocking_to_stopping_transition(void *param_ptr);

// 4.2 target state: STATE_TUNER_TUNING
static BOOLEAN relocking_to_tuning_check(void *param_ptr);
static BOOLEAN relocking_to_tuning_transition(void *param_ptr);

// 4.3 target state: STATE_TUNER_TRACKING
static BOOLEAN relocking_to_tracking_check(void *param_ptr);
static BOOLEAN relocking_to_tracking_transition(void *param_ptr);

// 4.4 target state: STATE_TUNER_RELOCKING
static BOOLEAN relocking_to_relocking_check(void *param_ptr);
static BOOLEAN relocking_to_relocking_transition(void *param_ptr);



static STATE_TRANSITION_TABLE relocking_state_trans_tbl[] = {
        {STATE_TUNER_STOPPING, relocking_to_stopping_check, relocking_to_stopping_transition},
        {STATE_TUNER_TUNING, relocking_to_tuning_check, relocking_to_tuning_transition},
        {STATE_TUNER_TRACKING, relocking_to_tracking_check, relocking_to_tracking_transition},
        {STATE_TUNER_RELOCKING, relocking_to_relocking_check, relocking_to_relocking_transition},
        {INVALID_STATE, NULL, NULL}
    };


// state 5: TUNER_EXITED

// state 6: TUNER_STOPPING
static BOOLEAN stopping_state_enter(void *param_ptr);
static BOOLEAN stopping_state_exit(void *param_ptr);

// 6.1 target state: STATE_TUNER_IDLE
static BOOLEAN stopping_to_idle_check(void *param_ptr);
static BOOLEAN stopping_to_idle_transition(void *param_ptr);

static STATE_TRANSITION_TABLE stopping_state_trans_tbl[] = {
        {STATE_TUNER_IDLE, stopping_to_idle_check, stopping_to_idle_transition},
        {INVALID_STATE, NULL, NULL}
    };


//--------State MAP---------------
static STATE sg_state_idle              = {STATE_TUNER_IDLE, idle_state_enter, idle_state_exit, idle_state_trans_tbl};
static STATE sg_state_tuning            = {STATE_TUNER_TUNING, tuning_state_enter, tuning_state_exit, tuning_state_trans_tbl};
static STATE sg_state_tracking          = {STATE_TUNER_TRACKING, tracking_state_enter, tracking_state_exit, tracking_state_trans_tbl};
static STATE sg_state_stopping          = {STATE_TUNER_STOPPING, stopping_state_enter, stopping_state_exit, stopping_state_trans_tbl};
static STATE sg_state_relocking         = {STATE_TUNER_RELOCKING, relocking_state_enter, relocking_state_exit, relocking_state_trans_tbl};

static STATE_MAP sg_tune_state_map[] = {
    {STATE_TUNER_IDLE,      "TUNER_IDLE",       &sg_state_idle},
    {STATE_TUNER_TUNING,    "TUNER_TUNING",     &sg_state_tuning},
    {STATE_TUNER_TRACKING,  "TUNER_TRACKING",   &sg_state_tracking},
    {STATE_TUNER_STOPPING,  "TUNER_STOPPING",   &sg_state_stopping},
    {STATE_TUNER_RELOCKING, "STATE_TUNER_RELOCKING", &sg_state_relocking},
    {INVALID_STATE,         "INVALID_STATE",    NULL}
};




//---External Function Declare---
BOOLEAN stb_tune_fsm_init(U8BIT total_path)
{
    BOOLEAN ret = TRUE;
    U8BIT loop = 0;

    sg_tune_task_msg_queue_ptr_array    = (void **)STB_GetMemory(sizeof(void *) * total_path);
    sg_tune_task_ptr_array              = (void **)STB_GetMemory(sizeof(void *) * total_path);
    sg_tune_fsm_ptr_array               = (FSM_INSTANCE **)STB_GetMemory(sizeof(FSM_INSTANCE *) * total_path);

    memset(&sg_tune_fsm_timer, 0, sizeof(sg_tune_fsm_timer));

    if (NULL == sg_tune_task_msg_queue_ptr_array ||
        NULL == sg_tune_task_ptr_array ||
        NULL == sg_tune_fsm_ptr_array)
    {
        ret = FALSE;

        if (NULL != sg_tune_task_msg_queue_ptr_array)
        {
            STB_FreeMemory(sg_tune_task_msg_queue_ptr_array);
        }

        if (NULL != sg_tune_task_ptr_array)
        {
            STB_FreeMemory(sg_tune_task_ptr_array);
        }

        if (NULL != sg_tune_fsm_ptr_array)
        {
            STB_FreeMemory(sg_tune_fsm_ptr_array);
        }
    }
    else
    {
        #if 0
        for (loop = 0; loop < total_path; loop++)
        {
            ret &= stb_tune_fsm_create(loop, TUNER_IDLE);
        }
        #endif
    }

    CERT_LOG_INFO(TAG, "[%s] ret:%u", __FUNCTION__, ret);

    return ret;
}

BOOLEAN stb_tune_fsm_create(U8BIT path, S_TUNER_STATUS *tstatus, U32BIT state)
{
    BOOLEAN ret = TRUE;

    do
    {
        if (NULL != sg_tune_task_msg_queue_ptr_array)
        {
            sg_tune_task_msg_queue_ptr_array[path] = STB_OSCreateQueue(sizeof(STRU_FSM_TASK_MSG), TUNE_FSM_TASK_QUEUE_MAX_ENTRIES);
            if (NULL == sg_tune_task_msg_queue_ptr_array[path])
            {
                CERT_LOG_ERROR(TAG, "[%s] Create Queue Err!", __FUNCTION__);
                ret = FALSE;

                break;
            }
        }
        else
        {
            ret = FALSE;
            break;
        }

        if (NULL != sg_tune_task_ptr_array)
        {
            U8BIT taskname[16];

            sprintf(taskname, "tune%u", path);
            sg_tune_task_ptr_array[path] = STB_OSCreateTask(tune_fsm_task, (void *)&path, TUNE_FSM_TASK_STACK_SIZE, TUNE_FSM_TASK_PRIORITY, taskname);
            if (NULL == sg_tune_task_ptr_array[path])
            {
                CERT_LOG_ERROR(TAG, "[%s] Create Task Err!", __FUNCTION__);

                ret = FALSE;
                break;
            }
        }
        else
        {
            ret = FALSE;
            break;
        }

        if (NULL != sg_tune_fsm_ptr_array)
        {
            sg_tune_fsm_ptr_array[path] = fsm_CreateInstance(sg_tune_state_map);
            if (NULL == sg_tune_fsm_ptr_array[path])
            {
                CERT_LOG_ERROR(TAG, "[%s] Create FSM Err!", __FUNCTION__);

                ret = FALSE;
                break;
            }
            else
            {
                STRU_FSM_TASK_MSG msg;

                msg.path    = 0;
                msg.type    = EN_TUNE_INNER_MSG;
                msg.event   = EN_TUNE_INNER_EVENT_INIT;

                msg.free_para1  = FALSE;
                msg.para1_ptr   = tstatus;
                msg.free_para2  = FALSE;
                msg.para2_ptr   = NULL;

                fsm_SetInitState(sg_tune_fsm_ptr_array[path], state, &msg);
            }
        }
        else
        {
            ret = FALSE;
            break;
        }
    } while (0);

    return ret;
}


BOOLEAN stb_tune_fsm_msg_handle(void *param_ptr)
{
    BOOLEAN ret = TRUE;


    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;

    CERT_LOG_DEBG(TAG, "[%s] cur-state[%s] msg: %s, %s", __FUNCTION__,
                    tune_fsm_GetCurStateString(sg_tune_fsm_ptr_array[msg_ptr->path]->current_state_ptr->state),
                    tune_fsm_GetMsgTypeString(msg_ptr->type),
                    tune_fsm_GetMsgEventString(msg_ptr->type, msg_ptr->event));

    ret = fsm_FsmMsgHandle(sg_tune_fsm_ptr_array[msg_ptr->path], param_ptr);

    return ret;
}

BOOLEAN stb_tune_fsm_send_msg(U8BIT path, U32BIT type, U32BIT event, void *param1_ptr, void *param2_ptr)
{
    return _fsm_send_msg(path, type, event, FALSE, param1_ptr, FALSE, param2_ptr);
}

BOOLEAN stb_tune_is_support_tune_type(E_STB_TUNE_SIGNAL_TYPE signal_type, E_STB_TUNE_SYSTEM_TYPE sys_type, fe_delivery_system_t delivery_system)
{
    BOOLEAN ret = FALSE;

    switch (signal_type)
    {
        case TUNE_SIGNAL_COFDM:
        {
            if ((sys_type == TUNE_SYSTEM_TYPE_DVBT) ||
                ((sys_type == TUNE_SYSTEM_TYPE_DVBT2) && (delivery_system == SYS_DVBT2)))
            {
                ret = TRUE;
            }

            break;
        }

        case TUNE_SIGNAL_QAM:
        {
            if (delivery_system == SYS_DVBC_ANNEX_A)
            {
                ret = TRUE;
            }

            break;
        }

        case TUNE_SIGNAL_QPSK:
        {
            if ((sys_type == TUNE_SYSTEM_TYPE_DVBS) ||
                ((sys_type == TUNE_SYSTEM_TYPE_DVBS2) && (delivery_system == SYS_DVBS2)))
            {
                ret = TRUE;
            }

            break;
        }

        case TUNE_SIGNAL_ISDBT:
        {
            if (((sys_type == TUNE_SYSTEM_TYPE_ISDBT) && (delivery_system == SYS_ISDBT)))
            {
                ret = TRUE;
            }

            break;
        }

        default:
            break;
    }

    return ret;
}

BOOLEAN stb_tune_isdiff_systype(S_TUNER_STATUS *tstatus)
{
    BOOLEAN is_diff = TRUE;

    struct dtv_property p = {.cmd = DTV_DELIVERY_SYSTEM, .u.data = 0};
    struct dtv_properties props = {.num = 1, .props = &p};

    if (ioctl(tstatus->frontend_fd, FE_GET_PROPERTY, &props) != -1)
    {
        if (((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT) && (p.u.data == SYS_DVBT)) ||
             ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2) && (p.u.data == SYS_DVBT2)) ||
             ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS) && (p.u.data == SYS_DVBS)) ||
             ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS2) && (p.u.data == SYS_DVBS2)) ||
             ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBC)  && ((p.u.data == SYS_DVBC_ANNEX_A) || (p.u.data == SYS_DVBC_ANNEX_B) || (p.u.data == SYS_DVBC_ANNEX_C))) ||
             ((tstatus->sys_type == TUNE_SYSTEM_TYPE_ISDBT) && (p.u.data == SYS_ISDBT)))
        {
            CERT_LOG_INFO(TAG, "[%s] same sys_type %s, delivery system is %d", __FUNCTION__,
                                GetSysTypeDebugString(tstatus->sys_type), p.u.data);
            is_diff = FALSE;
        }
        else
        {
            CERT_LOG_INFO(TAG, "[%s] different sys_type %s, delivery system is %d", __FUNCTION__,
                                GetSysTypeDebugString(tstatus->sys_type), p.u.data);
            is_diff = TRUE;
        }
    }
    else
    {
        CERT_LOG_ERROR(TAG, "[%s] ERROR: FE_GET_PROPERTY is failed!", __FUNCTION__);
        is_diff = TRUE;
    }

    return is_diff;
}



BOOLEAN stb_tune_update_tune_parameter(S_TUNER_STATUS *tstatus, U32BIT freq, U32BIT srate, E_STB_TUNE_FEC fec,
                            S8BIT freq_off, E_STB_TUNE_TMODE tmode, E_STB_TUNE_TBWIDTH tbwidth,
                            E_STB_TUNE_CMODE cmode, E_STB_TUNE_ANALOG_VIDEO_TYPE anlg_vtype)
{
    BOOLEAN is_update = FALSE;

    STB_OSMutexLock(tstatus->mutex);

    if (stb_tune_isdiff_systype(tstatus))
    {
        is_update = TRUE;
        tstatus->tuned_sys_type = tstatus->sys_type;
        SetFeProperty(tstatus->frontend_fd, tstatus->tuned_sys_type);
    }

    if (tstatus->freq != freq)
    {
       is_update = TRUE;
       tstatus->freq = freq;
    }

    switch (tstatus->signal_type)
    {
       case TUNE_SIGNAL_COFDM:
           if ((tstatus->u.terr.tmode != tmode) || (tstatus->u.terr.tbwidth != tbwidth))
           {
               is_update = TRUE;
               tstatus->u.terr.tmode = tmode;
               tstatus->u.terr.tbwidth = tbwidth;
           }

           break;

       case TUNE_SIGNAL_QAM:
           if ((tstatus->u.cab.cmode != cmode) || (tstatus->u.cab.srate != srate))
           {
               is_update = TRUE;
               tstatus->u.cab.cmode = cmode;
               tstatus->u.cab.srate = srate;
           }

           break;

       case TUNE_SIGNAL_QPSK:
           if ((tstatus->u.sat.fec != fec) || (tstatus->u.sat.srate != srate))
           {
               is_update = TRUE;
               tstatus->u.sat.fec = fec;
               tstatus->u.sat.srate = srate;
           }

           break;

       case TUNE_SIGNAL_ISDBT:
           if (tstatus->u.isdbt.tbwidth != tbwidth)
           {
               is_update = TRUE;
               tstatus->u.isdbt.tbwidth = tbwidth;
           }

           break;

       default:
           break;
    }


    STB_OSMutexUnlock(tstatus->mutex);

    CERT_LOG_INFO(TAG, "[%s] is_update:%d", __FUNCTION__, is_update);

    return is_update;
}


void stb_tune_start_tuner(S_TUNER_STATUS *tstatus, U32BIT freq, U32BIT srate, E_STB_TUNE_FEC fec,
                            S8BIT freq_off, E_STB_TUNE_TMODE tmode, E_STB_TUNE_TBWIDTH tbwidth,
                            E_STB_TUNE_CMODE cmode, E_STB_TUNE_ANALOG_VIDEO_TYPE anlg_vtype)
{
    BOOLEAN start_tuning = FALSE;
    BOOLEAN ret = FALSE;

    if (stb_tune_is_support_tune_type(tstatus->signal_type, tstatus->sys_type, tstatus->delivery_system))
    {
        STB_TimeConsumeDebug("Tune lock start");

        start_tuning = stb_tune_update_tune_parameter(tstatus, freq, srate, fec,
                                                      freq_off, tmode, tbwidth,
                                                      cmode, anlg_vtype);

        CERT_LOG_INFO(TAG, "[%s] tuning:%d, changded:%d, status:%d, state:%d", __FUNCTION__,
                            start_tuning, tstatus->tuning_params_changed,
                            GetTunerLockStatus(tstatus->frontend_fd),
                            tstatus->state);

        if (start_tuning ||
            tstatus->tuning_params_changed ||
            GetTunerLockStatus(tstatus->frontend_fd) != TUNER_STATE_LOCKED ||
            tstatus->state == TUNER_IDLE)
        {
            // start tune
            ret = stb_tune_fsm_send_msg(tstatus->path, EN_TUNE_CNTRL_MSG, EN_TUNE_CNTRL_EVENT_START_TUNE, tstatus, NULL);
            if (!ret)
            {
                STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path, sizeof(U8BIT));
            }
        }

    }
    else
    {
        CERT_LOG_ERROR(TAG, "[%s] %u: system type %u not supported", __FUNCTION__,
                                tstatus->path, tstatus->sys_type);

        //STB_OSMutexLock(tstatus->mutex);
        //state = tstatus->state;
        //STB_OSMutexUnlock(tstatus->mutex);

        // Stop Tuner
        // To be done ...
        /*
        if (state != TUNER_IDLE && state != TUNER_EXITED)
        {
            TuneStopTuner(tstatus);
        }
        */


        STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path, sizeof(U8BIT));
    }
}

void stb_tune_stop_tuner(S_TUNER_STATUS *tstatus)
{
    BOOLEAN ret = FALSE;

    ret = stb_tune_fsm_send_msg(tstatus->path, EN_TUNE_CNTRL_MSG, EN_TUNE_CNTRL_EVENT_STOP_TUNE, tstatus, NULL);
    if (!ret)
    {
        CERT_LOG_ERROR(TAG, "[%s] FAILED", __FUNCTION__);
    }
}


//---------Local Function Define------------------
static void* tune_fsm_task(void *param)
{
    U8BIT path = *((U8BIT *)(param));

    BOOLEAN ret = TRUE;
    BOOLEAN msg_ready = TRUE;

    STRU_FSM_TASK_MSG msg;

    CERT_LOG_INFO(TAG, "START tune fsm task [%d]", path);

    while (1)
    {
        // Waits for an event to be reported, or the next 100 ms to elapse
        msg_ready = STB_OSReadQueue(sg_tune_task_msg_queue_ptr_array[path], (void *)&msg, sizeof(STRU_FSM_TASK_MSG), TIMEOUT_NEVER);

        if (msg_ready)
        {
            ret = stb_tune_fsm_msg_handle((void *)&msg);
        }

        if (msg.free_para1 && NULL != msg.para1_ptr)
        {
            STB_FreeMemory(msg.para1_ptr);
        }

        if (msg.free_para2 && NULL != msg.para2_ptr)
        {
            STB_FreeMemory(msg.para2_ptr);
        }
    }

    CERT_LOG_INFO(TAG, "EXIST tune fsm task [%d]", path);

    return NULL;
}


static BOOLEAN _fsm_send_msg(U8BIT path, U32BIT type, U32BIT event, BOOLEAN free_para1, void *param1_ptr, BOOLEAN free_para2, void *param2_ptr)
{
    BOOLEAN ret = TRUE;
    STRU_FSM_TASK_MSG msg;

    msg.path    = path;
    msg.type    = type;
    msg.event   = event;

    msg.free_para1  = free_para1;
    msg.para1_ptr   = param1_ptr;
    msg.free_para2  = free_para2;
    msg.para2_ptr   = param2_ptr;

    ret = STB_OSWriteQueue(sg_tune_task_msg_queue_ptr_array[path], (void *)&msg, sizeof(STRU_FSM_TASK_MSG), TIMEOUT_NEVER);
    if (!ret)
    {
        CERT_LOG_ERROR(TAG, "[%s] Msg[%u, %u] Send Err", __FUNCTION__, type, event);
    }
    else
    {
        //CERT_LOG_DEBG(TAG, "[%s] Msg[%u, %u] Send", __FUNCTION__, type, event);
    }

    return ret;
}

static void _set_tstatus_state(S_TUNER_STATUS *tstatus, E_TUNER_STATE state)
{
    STB_OSMutexLock(tstatus->mutex);
    tstatus->state = state;
    STB_OSMutexUnlock(tstatus->mutex);
}

static void _add_fsm_timer(ENUM_TIMERMODE timermode, U32BIT interval_ms, U32BIT timeout_msg_type, U32BIT timeout_msg_event, U8BIT path, S_TUNER_STATUS *tstatus)
{
    U32BIT min = interval_ms / 60000;
    U32BIT us  = (interval_ms - 60000 * min) * 1000;

    struct timeval tm = {min, us};

    if (sg_tune_fsm_timer.id)
    {
        STB_OSDeleteTimer(&sg_tune_fsm_timer);
        sg_tune_fsm_timer.id = NULL;
    }

    sg_timeout_msg.msg_type     = timeout_msg_type;
    sg_timeout_msg.msg_event    = timeout_msg_event;
    sg_timeout_msg.path         = path;
    sg_timeout_msg.tstatus      = tstatus;

    sg_tune_fsm_timer.id                    = NULL;
    sg_tune_fsm_timer.flag                  = timermode;
    sg_tune_fsm_timer.expire_value.tv_sec   = tm.tv_sec;
    sg_tune_fsm_timer.expire_value.tv_usec  = tm.tv_usec;
    sg_tune_fsm_timer.expireCB              = _send_fsm_timeout_msg;
    sg_tune_fsm_timer.userptr               = &sg_timeout_msg;

    //CERT_LOG_DEBG(TAG, "[%s:%d] timerid:%u, min:%u, us:%u", __FUNCTION__, __LINE__,
    //               sg_tune_fsm_timer.id, min, us);

    STB_OSAddTimer(&sg_tune_fsm_timer);

    //CERT_LOG_DEBG(TAG, "[%s:%d] timerid:%u, min:%u, us:%u", __FUNCTION__, __LINE__,
    //                sg_tune_fsm_timer.id, min, us);
}

static void _send_fsm_timeout_msg(void *arg)
{
    STRU_TIMEOUT_MSG *msg_ptr = (STRU_TIMEOUT_MSG *)arg;

    //CERT_LOG_DEBG(TAG, "[%s]", __FUNCTION__);

    stb_tune_fsm_send_msg(msg_ptr->path, msg_ptr->msg_type,
                          msg_ptr->msg_event, msg_ptr->tstatus, NULL);


}

static void _restart_fsm_timer()
{
    STB_OSRestartTimer(&sg_tune_fsm_timer);
}

static void _delete_fsm_timer()
{
    //CERT_LOG_DEBG(TAG, "[%s] timerid:%u", __FUNCTION__, sg_tune_fsm_timer.id);

    if (sg_tune_fsm_timer.id)
    {
        STB_OSDeleteTimer(&sg_tune_fsm_timer);
        sg_tune_fsm_timer.id = NULL;
    }
}

static BOOLEAN _check_hw_lock_status(int frontend_fd, BOOLEAN *locked)
{
    BOOLEAN ret = FALSE;
    struct pollfd pfd;
    struct dvb_frontend_event fe_event;


    pfd.fd = frontend_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    *locked = FALSE;

    if (poll(&pfd, 1, TUNER_POLLING_TIMEOUT) == 1)
    {
       if (ioctl(frontend_fd, FE_GET_EVENT, &fe_event) >= 0)
       {
           CERT_LOG_INFO(TAG, "[%s] status=0x%02x", __FUNCTION__, fe_event.status);

           if ((fe_event.status & FE_HAS_LOCK) != 0)
           {
                *locked = TRUE;
                ret     = TRUE;
           }
           else if ((fe_event.status & FE_TIMEDOUT) != 0)
           {
                *locked = FALSE;
                ret     = TRUE;
           }
       }
    }
    else
    {
        ret = FALSE;
    }

    return ret;
}



// state 1: TUNER_IDLE
static BOOLEAN idle_state_enter(void *param_ptr)
{
    BOOLEAN ret = TRUE;
    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    CERT_LOG_INFO(TAG, "[%s]", __FUNCTION__);
    _set_tstatus_state(tstatus, TUNER_IDLE);

    return ret;

}

static BOOLEAN idle_state_exit(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    CERT_LOG_INFO(TAG, "[%s]", __FUNCTION__);

    return ret;
}


// 1.1 target state: TUNER_TUNING
static BOOLEAN idle_to_tuning_check(void *param_ptr)
{
    BOOLEAN ret = FALSE;
    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_CNTRL_MSG && msg_ptr->event == EN_TUNE_CNTRL_EVENT_START_TUNE)
    {
        if (StartTune(tstatus))
        {
            ret = TRUE;
        }
        else
        {
            STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path, sizeof(U8BIT));
        }
    }

    return ret;
}


static BOOLEAN idle_to_tuning_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    return ret;
}



// state 2: TUNER_TUNING
static BOOLEAN tuning_state_enter(void *param_ptr)
{
    BOOLEAN ret = TRUE;
    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;


    BOOLEAN locked = FALSE;

    CERT_LOG_INFO(TAG, "[%s]", __FUNCTION__);
    _set_tstatus_state(tstatus, TUNER_TUNING);

    sg_start_time = STB_OSGetClockMilliseconds();

    if (_check_hw_lock_status(tstatus->frontend_fd, &locked))
    {
        if (locked)
        {
            while (!stb_tune_fsm_send_msg(tstatus->path, EN_TUNE_INNER_MSG, EN_TUNE_INNER_EVENT_LOCKED, tstatus, NULL));
        }
        else
        {
            while (!stb_tune_fsm_send_msg(tstatus->path, EN_TUNE_INNER_MSG, EN_TUNE_INNER_EVENT_UNLOCKED, tstatus, NULL));
        }
    }
    else
    {
        // Start Check Timer & Try Again
        _add_fsm_timer(EN_TIMERMODE_ONESHOT, TUNE_CHECK_LOCK_TIME_INTERVAL, EN_TUNE_TIMER_MSG, EN_TUNE_TIMER_EVENT_CHKLOCK,
                        tstatus->path, tstatus);
    }

    return ret;
}

static BOOLEAN tuning_state_exit(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    CERT_LOG_INFO(TAG, "[%s]", __FUNCTION__);

    _delete_fsm_timer();

    return ret;
}



// 2.1 target state: TUNER_TRACKING
static BOOLEAN tuning_to_tracking_check(void *param_ptr)
{
    BOOLEAN ret = FALSE;

    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_INNER_MSG && msg_ptr->event == EN_TUNE_INNER_EVENT_LOCKED)
    {
        ret = TRUE;
    }

    return ret;
}


static BOOLEAN tuning_to_tracking_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    return ret;
}

// 2.2 target state: STATE_TUNER_RELOCKING
static BOOLEAN tuning_to_relocking_check(void *param_ptr)
{
    BOOLEAN ret = FALSE;

    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_INNER_MSG && msg_ptr->event == EN_TUNE_INNER_EVENT_UNLOCKED)
    {
        ret = TRUE;
    }

    return ret;
}


static BOOLEAN tuning_to_relocking_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    return ret;
}

// 2.3 target state: STATE_TUNER_STOPPING
static BOOLEAN tuning_to_stopping_check(void *param_ptr)
{
    BOOLEAN ret = FALSE;

    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_CNTRL_MSG && msg_ptr->event == EN_TUNE_CNTRL_EVENT_STOP_TUNE)
    {
        ret = TRUE;
    }

    return ret;
}


static BOOLEAN tuning_to_stopping_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    return ret;
}

// 2.4 target state: STATE_TUNER_TUNING
static BOOLEAN tuning_to_tuning_check(void *param_ptr)
{
    BOOLEAN ret     = FALSE;
    BOOLEAN locked  = FALSE;

    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_TIMER_MSG && msg_ptr->event == EN_TUNE_TIMER_EVENT_CHKLOCK)
    {
        ret = TRUE;

        if (_check_hw_lock_status(tstatus->frontend_fd, &locked))
        {
            if (locked)
            {
                while (!stb_tune_fsm_send_msg(tstatus->path, EN_TUNE_INNER_MSG, EN_TUNE_INNER_EVENT_LOCKED, tstatus, NULL));
            }
            else
            {
                while (!stb_tune_fsm_send_msg(tstatus->path, EN_TUNE_INNER_MSG, EN_TUNE_INNER_EVENT_UNLOCKED, tstatus, NULL));
            }
        }
        else
        {
            if ((STB_OSGetClockDiff(sg_start_time) < WAIT_LOCK_TIMEOUT))
            {
                _restart_fsm_timer();
            }
            else
            {
                while (!stb_tune_fsm_send_msg(tstatus->path, EN_TUNE_INNER_MSG, EN_TUNE_INNER_EVENT_UNLOCKED, tstatus, NULL));
            }
        }
    }

    return ret;
}

static BOOLEAN tuning_to_tuning_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    return ret;
}


// state 3: TUNER_TRACKING
static BOOLEAN tracking_state_enter(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    CERT_LOG_INFO(TAG, "[%s]", __FUNCTION__);
    _set_tstatus_state(tstatus, TUNER_LOCKED);

    STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_LOCKED, &tstatus->path,
                                        sizeof(U8BIT));

    // Start Check Timer & Try Again
    _add_fsm_timer(EN_TIMERMODE_ONESHOT, TUNE_TRACKING_CHECK_TIME_INTERVAL, EN_TUNE_TIMER_MSG, EN_TUNE_TIMER_EVENT_TRACKING_CHK,
                    tstatus->path, tstatus);

    return ret;
}

static BOOLEAN tracking_state_exit(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    CERT_LOG_INFO(TAG, "[%s]", __FUNCTION__);

    _delete_fsm_timer();

    return ret;
}


// 3.1 target state: TUNER_STOPPING
static BOOLEAN tracking_to_stopping_check(void *param_ptr)
{
    BOOLEAN ret = FALSE;

    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_CNTRL_MSG && msg_ptr->event == EN_TUNE_CNTRL_EVENT_STOP_TUNE)
    {
        ret = TRUE;
    }

    return ret;
}


static BOOLEAN tracking_to_stopping_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    return ret;
}

// 3.2 target state: STATE_TUNER_TUNING
static BOOLEAN tracking_to_tuning_check(void *param_ptr)
{
    BOOLEAN ret = FALSE;
    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_CNTRL_MSG && msg_ptr->event == EN_TUNE_CNTRL_EVENT_START_TUNE)
    {
        if (StartTune(tstatus))
        {
            ret = TRUE;
        }
        else
        {
            STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path, sizeof(U8BIT));
        }
    }

    return ret;
}


static BOOLEAN tracking_to_tuning_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    return ret;
}

// 3.3 target state: STATE_TUNER_RELOCKING
static BOOLEAN tracking_to_relocking_check(void *param_ptr)
{
    BOOLEAN ret = FALSE;
    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_INNER_MSG && msg_ptr->event == EN_TUNE_INNER_EVENT_UNLOCKED)
    {
        ret = TRUE;
    }

    return ret;
}


static BOOLEAN tracking_to_relocking_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    return ret;
}


// 3.4 target state: STATE_TUNE_TRACKING
static BOOLEAN tracking_to_tracking_check(void *param_ptr)
{
    BOOLEAN ret = FALSE;
    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_TIMER_MSG)
    {
        ret = TRUE;
    }

    return ret;
}

static BOOLEAN tracking_to_tracking_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;
    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    BOOLEAN locked = FALSE;

    //CERT_LOG_DEBG(TAG, "[%s:%d]", __FUNCTION__, __LINE__);

    if (msg_ptr->type == EN_TUNE_TIMER_MSG && msg_ptr->event == EN_TUNE_TIMER_EVENT_TRACKING_CHK)
    {
        //CERT_LOG_DEBG(TAG, "[%s:%d]", __FUNCTION__, __LINE__);

        if (_check_hw_lock_status(tstatus->frontend_fd, &locked))
        {
            //CERT_LOG_DEBG(TAG, "[%s:%d]", __FUNCTION__, __LINE__);

            if (!locked)
            {
                while (!stb_tune_fsm_send_msg(tstatus->path, EN_TUNE_INNER_MSG, EN_TUNE_INNER_EVENT_UNLOCKED, tstatus, NULL));

                //CERT_LOG_DEBG(TAG, "[%s:%d]", __FUNCTION__, __LINE__);
            }
            else
            {
                _restart_fsm_timer();

                //CERT_LOG_DEBG(TAG, "[%s:%d]", __FUNCTION__, __LINE__);
            }
        }
        else
        {
            _restart_fsm_timer();

            //CERT_LOG_DEBG(TAG, "[%s:%d]", __FUNCTION__, __LINE__);
        }
    }

    return ret;
}






// state 4: STATE_TUNER_RELOCKING
static BOOLEAN relocking_state_enter(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    CERT_LOG_INFO(TAG, "[%s]", __FUNCTION__);
    _set_tstatus_state(tstatus, TUNER_RELOCKING);

    STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path,
                                        sizeof(U8BIT));

    // Start Check Timer & Try Again
    _add_fsm_timer(EN_TIMERMODE_ONESHOT, TUNE_RELOCKING_CHECK_TIME_INTERVAL, EN_TUNE_TIMER_MSG, EN_TUNE_TIMER_EVENT_RELOCKING_CHK,
                    tstatus->path, tstatus);

    return ret;
}


static BOOLEAN relocking_state_exit(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    CERT_LOG_INFO(TAG, "[%s]", __FUNCTION__);

    _delete_fsm_timer();

    return ret;
}


// 4.1 target state: STATE_TUNER_TRACKING
static BOOLEAN relocking_to_stopping_check(void *param_ptr)
{
    BOOLEAN ret = FALSE;

    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_CNTRL_MSG && msg_ptr->event == EN_TUNE_CNTRL_EVENT_STOP_TUNE)
    {
        ret = TRUE;
    }

    return ret;
}


static BOOLEAN relocking_to_stopping_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    return ret;
}

// 4.2 target state: STATE_TUNER_TUNING
static BOOLEAN relocking_to_tuning_check(void *param_ptr)
{
    BOOLEAN ret = FALSE;
    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_CNTRL_MSG && msg_ptr->event == EN_TUNE_CNTRL_EVENT_START_TUNE)
    {
        if (StartTune(tstatus))
        {
            ret = TRUE;
        }
        else
        {
            STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path, sizeof(U8BIT));
        }
    }

    return ret;
}


static BOOLEAN relocking_to_tuning_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    return ret;
}

// 4.3 target state: STATE_TUNER_TRACKING
static BOOLEAN relocking_to_tracking_check(void *param_ptr)
{
    BOOLEAN ret = FALSE;
    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_INNER_MSG && msg_ptr->event == EN_TUNE_INNER_EVENT_LOCKED)
    {
        ret = TRUE;
    }

    return ret;
}


static BOOLEAN relocking_to_tracking_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    return ret;
}



// 4.4 target state: STATE_TUNER_RELOCKING
static BOOLEAN relocking_to_relocking_check(void *param_ptr)
{
    BOOLEAN ret = FALSE;
    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_TIMER_MSG && msg_ptr->event == EN_TUNE_TIMER_EVENT_RELOCKING_CHK)
    {
        ret = TRUE;
    }

    return ret;
}


static BOOLEAN relocking_to_relocking_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;
    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    BOOLEAN locked = FALSE;


    if (msg_ptr->type == EN_TUNE_TIMER_MSG && msg_ptr->event == EN_TUNE_TIMER_EVENT_RELOCKING_CHK)
    {
        if (_check_hw_lock_status(tstatus->frontend_fd, &locked))
        {
            if (locked)
            {
                while (!stb_tune_fsm_send_msg(tstatus->path, EN_TUNE_INNER_MSG, EN_TUNE_INNER_EVENT_LOCKED, tstatus, NULL));
            }
            else
            {
                _restart_fsm_timer();
            }
        }
        else
        {
            _restart_fsm_timer();
        }
    }


    return ret;
}


// state 6: TUNER_STOPPING
static BOOLEAN stopping_state_enter(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    CERT_LOG_INFO(TAG, "[%s]", __FUNCTION__);

    // start tune
    while (!stb_tune_fsm_send_msg(tstatus->path, EN_TUNE_INNER_MSG, EN_TUNE_INNER_EVENT_STOPPED, tstatus, NULL));

    return ret;
}

static BOOLEAN stopping_state_exit(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    CERT_LOG_INFO(TAG, "[%s]", __FUNCTION__);

    return ret;
}

// 6.1 target state: STATE_TUNER_IDLE
static BOOLEAN stopping_to_idle_check(void *param_ptr)
{
    BOOLEAN ret = FALSE;

    STRU_FSM_TASK_MSG *msg_ptr = (STRU_FSM_TASK_MSG *)param_ptr;
    S_TUNER_STATUS *tstatus = (S_TUNER_STATUS *)msg_ptr->para1_ptr;

    if (msg_ptr->type == EN_TUNE_INNER_MSG && msg_ptr->event == EN_TUNE_INNER_EVENT_STOPPED)
    {
        ret = TRUE;
    }

    return ret;
}


static BOOLEAN stopping_to_idle_transition(void *param_ptr)
{
    BOOLEAN ret = TRUE;

    return ret;
}


char* tune_fsm_GetMsgTypeString(U32BIT type)
{
    switch (type)
    {
        case EN_TUNE_CNTRL_MSG:
            return "EN_TUNE_CNTRL_MSG";

        case EN_TUNE_INNER_MSG:
            return "EN_TUNE_INNER_MSG";

        case EN_TUNE_TIMER_MSG:
            return "EN_TUNE_TIMER_MSG";

        default:
            return "unknown MSG TYPE";
    }
}

char* tune_fsm_GetMsgEventString(U32BIT type, U32BIT event)
{
    switch (type)
    {
        case EN_TUNE_CNTRL_MSG:
        {
            switch (event)
            {
                case EN_TUNE_CNTRL_EVENT_START_TUNE:
                    return "EN_TUNE_CNTRL_EVENT_START_TUNE";

                case EN_TUNE_CNTRL_EVENT_STOP_TUNE:
                    return "EN_TUNE_CNTRL_EVENT_STOP_TUNE";

                default:
                    return "unknown EVENT";
            }
        }

        case EN_TUNE_INNER_MSG:
        {
            switch (event)
            {
                case EN_TUNE_INNER_EVENT_INIT:
                    return "EN_TUNE_INNER_EVENT_INIT";

                case EN_TUNE_INNER_EVENT_LOCKED:
                    return "EN_TUNE_INNER_EVENT_LOCKED";

                case EN_TUNE_INNER_EVENT_UNLOCKED:
                    return "EN_TUNE_INNER_EVENT_UNLOCKED";

                case EN_TUNE_INNER_EVENT_STOPPED:
                    return "EN_TUNE_INNER_EVENT_STOPPED";

                default:
                    return "unknown EVENT";
            }
        }

        case EN_TUNE_TIMER_MSG:
        {
            switch (event)
            {
                case EN_TUNE_TIMER_EVENT_CHKLOCK:
                    return "EN_TUNE_TIMER_EVENT_CHKLOCK";

                case EN_TUNE_TIMER_EVENT_TRACKING_CHK:
                    return "EN_TUNE_TIMER_EVENT_TRACKING_CHK";

                case EN_TUNE_TIMER_EVENT_RELOCKING_CHK:
                    return "EN_TUNE_TIMER_EVENT_RELOCKING_CHK";

                default:
                    return "unknown EVENT";
            }
        }

        default:
            return "unknown MSG TYPE";
    }
}


static char* tune_fsm_GetCurStateString(U32BIT state)
{
    switch (state)
    {
        case STATE_TUNER_IDLE:
            return "STATE_TUNER_IDLE";

        case STATE_TUNER_TUNING:
            return "STATE_TUNER_TUNING";

        case STATE_TUNER_TRACKING:
            return "STATE_TUNER_TRACKING";

        case STATE_TUNER_RELOCKING:
            return "STATE_TUNER_RELOCKING";

        case STATE_TUNER_EXITED:
            return "STATE_TUNER_EXITED";

        case STATE_TUNER_STOPPING:
            return "STATE_TUNER_STOPPING";

        default:
            return "!ERR unknown STATE";
    }
}

