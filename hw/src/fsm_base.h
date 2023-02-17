/*******************************************************************************
 *  Copyright @ 2023
 *
 *  Finite State Machine Basic Framework
 *
 *
 *******************************************************************************/

/**
 * @brief   Finite State Machine Basic Framework
 * @file    fsm_base.h
 * @date    2023-01-29
 */

#ifndef __FSM_BASE_H__
#define __FSM_BASE_H__


#define INVALID_STATE   0xFFFFFFFF
//#define STATE_INIT      0

typedef BOOLEAN (* STATE_ENTER_FUNC)(void *param_ptr);
typedef BOOLEAN (* STATE_EXIT_FUNC)(void *param_ptr);

typedef BOOLEAN(* TRANSITION_DETERMINATION_FUNC)(void *param_ptr);
typedef BOOLEAN(* TRANSITION_FUNC)(void *param_ptr);

typedef struct state_transition_table
{
    U32BIT target_state;
    TRANSITION_DETERMINATION_FUNC trans_determination_ptr;
    TRANSITION_FUNC transition_ptr;
}STATE_TRANSITION_TABLE, *STATE_TRANSITION_TABLE_PTR;

typedef struct state
{
    U32BIT state;
    STATE_ENTER_FUNC state_enter_ptr;
    STATE_EXIT_FUNC state_exit_ptr;
    STATE_TRANSITION_TABLE_PTR state_trans_tbl_ptr;
}STATE;

typedef struct state_map
{
    U32BIT state;
    char *state_name;
    STATE *state_ptr;
}STATE_MAP;

typedef struct fsm_instance
{
    STATE *current_state_ptr;
    STATE_MAP *state_map_ptr;
}FSM_INSTANCE;

typedef struct fsm_task_msg
{
    U8BIT path;
    U32BIT type;
    U32BIT event;
    BOOLEAN free_para1;
    BOOLEAN free_para2;
    void *para1_ptr;
    void *para2_ptr;
} STRU_FSM_TASK_MSG;


/**
 * @brief   Create a FSM instance
 * @param   STATE_MAP *state_map_ptr: the STATE array of the FSM
 *
 * @attention !!! The last item of STATE array must be:
 *            "{INVALID_STATE, "INVALID_STATE", NULL}"
 */
FSM_INSTANCE* fsm_CreateInstance(STATE_MAP *state_map_ptr);


/**
 * @brief   Set the FSM initial state
 * @param   FSM_INSTANCE *fsm_instance_ptr: IN, the fsm instance
 * @param   U32BIT state                  : IN, the initial state value
 */
BOOLEAN fsm_SetInitState(FSM_INSTANCE *fsm_instance_ptr, U32BIT state, STRU_FSM_TASK_MSG *msg_ptr);



/**
 * @brief   Fsm handle the msg Function
 * @param   FSM_INSTANCE *fsm_instance_ptr: IN, the fsm instance
 * @param   void *param_ptr: IN, pointer to msg structure
 */
BOOLEAN fsm_FsmMsgHandle(FSM_INSTANCE *fsm_instance_ptr, void *param_ptr);


BOOLEAN fsm_DefaultStateEnterFunc(FSM_INSTANCE *fsm_instance_ptr, void *param_ptr);
BOOLEAN fsm_DefaultStateExitFunc(FSM_INSTANCE *fsm_instance_ptr, void *param_ptr);

#endif
