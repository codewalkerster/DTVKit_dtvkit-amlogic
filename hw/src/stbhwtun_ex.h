/*******************************************************************************
 *  Copyright @ 2023
 *
 *  tuner&demod control extention
 *
 *
 *******************************************************************************/

/**
 * @brief   tuner&demod control extention
 * @file    stbhwtun_ex.h
 * @date    2023-01-29
 */

#ifndef __STBHWTUNE_EX_H__
#define __STBHWTUNE_EX_H__



//---Constant and macro definitions for public use-----------------------------
#define TUNE_FSM_TASK_QUEUE_MAX_ENTRIES     20
#define TUNE_FSM_TASK_STACK_SIZE            4096
#define TUNE_FSM_TASK_PRIORITY              11      // 1 lower than UI

#define TUNE_FSM_MSG_TIMEOUT                100

#define TUNE_CHECK_LOCK_TIME_INTERVAL       100     // ms
#define TUNE_TRACKING_CHECK_TIME_INTERVAL   999     // ms
#define TUNE_RELOCKING_CHECK_TIME_INTERVAL  500     // ms

#define STATE_TRACKING_LOCKLOST_MAXCOUNT    3

//---Enumerations for public use-----------------------------------------------
typedef enum
{
    STATE_TUNER_IDLE        = TUNER_IDLE,
    STATE_TUNER_TUNING      = TUNER_TUNING,
    STATE_TUNER_TRACKING    = TUNER_LOCKED,
    STATE_TUNER_RELOCKING   = TUNER_RELOCKING,
    STATE_TUNER_EXITED      = TUNER_EXITED,
    STATE_TUNER_STOPPING
} E_TUNE_FSM_STATE;

typedef enum tune_msg_type
{
    EN_TUNE_CNTRL_MSG = 0,
    EN_TUNE_INNER_MSG,
    EN_TUNE_TIMER_MSG
} ENUM_TUNE_MSG_TYPE;

typedef enum tune_cntrl_msg_event
{
    EN_TUNE_CNTRL_EVENT_START_TUNE = 0,
    EN_TUNE_CNTRL_EVENT_STOP_TUNE
} ENUM_TUNE_MSG_CNTRL_EVENT;

typedef enum tune_inner_msg_event
{
    EN_TUNE_INNER_EVENT_INIT = 0,
    EN_TUNE_INNER_EVENT_LOCKED,
    EN_TUNE_INNER_EVENT_UNLOCKED,
    EN_TUNE_INNER_EVENT_STOPPED,
} ENUM_TUNE_MSG_INNER_EVENT;

typedef enum tune_timer_msg_event
{
    EN_TUNE_TIMER_EVENT_CHKLOCK = 0,
    EN_TUNE_TIMER_EVENT_TRACKING_CHK,
    EN_TUNE_TIMER_EVENT_RELOCKING_CHK
} ENUM_TUNE_MSG_TIMEOUT_EVENT;

//---External Function Declare---

/*-----for tune fsm-----*/
BOOLEAN stb_tune_fsm_init(U8BIT total_path);

BOOLEAN stb_tune_fsm_create(U8BIT path, S_TUNER_STATUS *tstatus, U32BIT state);

BOOLEAN stb_tune_fsm_msg_handle(void *param_ptr);

BOOLEAN stb_tune_fsm_send_msg(U8BIT path, U32BIT type, U32BIT event, void *param1_ptr, void *param2_ptr);


/*-----for tune control----*/
BOOLEAN stb_tune_is_support_tune_type(E_STB_TUNE_SIGNAL_TYPE signal_type, E_STB_TUNE_SYSTEM_TYPE sys_type, fe_delivery_system_t delivery_system);

BOOLEAN stb_tune_isdiff_systype(S_TUNER_STATUS *tstatus);

BOOLEAN stb_tune_update_tune_parameter(S_TUNER_STATUS *tstatus, U32BIT freq, U32BIT srate, E_STB_TUNE_FEC fec,
                            S8BIT freq_off, E_STB_TUNE_TMODE tmode, E_STB_TUNE_TBWIDTH tbwidth,
                            E_STB_TUNE_CMODE cmode, E_STB_TUNE_ANALOG_VIDEO_TYPE anlg_vtype);


void stb_tune_start_tuner(S_TUNER_STATUS *tstatus, U32BIT freq, U32BIT srate, E_STB_TUNE_FEC fec,
                            S8BIT freq_off, E_STB_TUNE_TMODE tmode, E_STB_TUNE_TBWIDTH tbwidth,
                            E_STB_TUNE_CMODE cmode, E_STB_TUNE_ANALOG_VIDEO_TYPE anlg_vtype);

void stb_tune_stop_tuner(S_TUNER_STATUS *tstatus);




#endif

