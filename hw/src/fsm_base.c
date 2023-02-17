/*******************************************************************************
 *  Copyright @ 2023
 *
 *  Finite State Machine Basic Framework
 *
 *
 *******************************************************************************/

/**
 * @brief   Finite State Machine Basic Framework
 * @file    fsm_base.c
 * @date    2023-01-29
 */

#include <string.h>

#include "techtype.h"

#include "stbheap.h"


#include "cert_log.h"
#include "fsm_base.h"

#define TAG "FSM"

//----------B: Local Function Declare-------------
static STATE* GetState(FSM_INSTANCE *fsm_instance_ptr, U32BIT state);
static char* GetStateName(FSM_INSTANCE *fsm_instance_ptr, U32BIT state);
static BOOLEAN TransState(FSM_INSTANCE *fsm_instance_ptr, U32BIT target_state, void *param_ptr);

//static BOOLEAN default_state_enter(void *param_ptr);
//static BOOLEAN default_state_exit(void *param_ptr);
//----------E: Local Function Declare-------------


FSM_INSTANCE* fsm_CreateInstance(STATE_MAP *state_map_ptr)
{
    FSM_INSTANCE *new_fsm_ptr = NULL;

    if (NULL == state_map_ptr)
    {
        CERT_LOG_ERROR(TAG, "[%s] Failed:No State Map!", __FUNCTION__);

        return new_fsm_ptr;
    }

    new_fsm_ptr = (FSM_INSTANCE *)STB_GetMemory(sizeof(FSM_INSTANCE));
    if (NULL == new_fsm_ptr)
    {
        CERT_LOG_ERROR(TAG, "[%s] Failed:No State Map!", __FUNCTION__);
    }
    else
    {
        new_fsm_ptr->current_state_ptr  = NULL;
        new_fsm_ptr->state_map_ptr      = state_map_ptr;
    }

    return new_fsm_ptr;
}

BOOLEAN fsm_SetInitState(FSM_INSTANCE *fsm_instance_ptr, U32BIT state, STRU_FSM_TASK_MSG *msg_ptr)
{
    BOOLEAN ret = FALSE;
    STRU_FSM_TASK_MSG msg;

    if (NULL == fsm_instance_ptr)
    {
        CERT_LOG_ERROR(TAG, "[%s] fsm_instance is NULL!", __FUNCTION__);

        return ret;
    }

    fsm_instance_ptr->current_state_ptr = GetState(fsm_instance_ptr, state);

    if (NULL != fsm_instance_ptr->current_state_ptr)
    {
        if (NULL != fsm_instance_ptr->current_state_ptr->state_enter_ptr)
        {
            fsm_instance_ptr->current_state_ptr->state_enter_ptr(msg_ptr);

            ret = TRUE;
        }
    }
    else
    {
        CERT_LOG_ERROR(TAG, "[%s] failed!", __FUNCTION__);
    }

    return ret;
}

BOOLEAN fsm_FsmMsgHandle(FSM_INSTANCE *fsm_instance_ptr, void *param_ptr)
{
    U32BIT loop = 0;

    BOOLEAN ret1 = TRUE;
    BOOLEAN ret2 = TRUE;

    if (NULL == fsm_instance_ptr)
    {
       CERT_LOG_ERROR(TAG, "[%s] fsm_instance is NULL!", __FUNCTION__);

       return FALSE;
    }

    if (NULL != fsm_instance_ptr->current_state_ptr)
    {
        for (loop = 0; INVALID_STATE != fsm_instance_ptr->current_state_ptr->state_trans_tbl_ptr[loop].target_state; loop++)
        {
            if (NULL != fsm_instance_ptr->current_state_ptr->state_trans_tbl_ptr[loop].trans_determination_ptr)
            {
                if (fsm_instance_ptr->current_state_ptr->state_trans_tbl_ptr[loop].trans_determination_ptr(param_ptr))
                {
                    if (NULL != fsm_instance_ptr->current_state_ptr->state_trans_tbl_ptr[loop].transition_ptr)
                    {
                        ret1 = fsm_instance_ptr->current_state_ptr->state_trans_tbl_ptr[loop].transition_ptr(param_ptr);
                    }

                    ret2 = TransState(fsm_instance_ptr, fsm_instance_ptr->current_state_ptr->state_trans_tbl_ptr[loop].target_state, param_ptr);

                    break;
                }
            }
        }
    }

    return (ret1 && ret2);
}

BOOLEAN fsm_DefaultStateEnterFunc(FSM_INSTANCE *fsm_instance_ptr, void *param_ptr)
{
    char *state_name = NULL;

    if (NULL == fsm_instance_ptr)
    {
        CERT_LOG_ERROR(TAG, "[%s] fsm_instance is NULL!", __FUNCTION__);

        return FALSE;
    }


    if (NULL != fsm_instance_ptr->current_state_ptr)
        state_name = GetStateName(fsm_instance_ptr, fsm_instance_ptr->current_state_ptr->state);

    if (NULL != state_name)
        CERT_LOG_INFO(TAG, "[%s] enter state: <%s>", __FUNCTION__, (char *)state_name);
    else
        CERT_LOG_INFO(TAG, "[%s] enter state: <%s>", __FUNCTION__, (char *)"unknown");

    return TRUE;
}

BOOLEAN fsm_DefaultStateExitFunc(FSM_INSTANCE *fsm_instance_ptr, void *param_ptr)
{
    char *state_name = NULL;

    if (NULL == fsm_instance_ptr)
    {
        CERT_LOG_ERROR(TAG, "[%s] fsm_instance is NULL!", __FUNCTION__);

        return FALSE;
    }

    if (NULL != fsm_instance_ptr->current_state_ptr)
    state_name = GetStateName(fsm_instance_ptr, fsm_instance_ptr->current_state_ptr->state);

    if (NULL != state_name)
        CERT_LOG_INFO(TAG, "[%s] exit state: <%s>", __FUNCTION__, (char *)state_name);
    else
        CERT_LOG_INFO(TAG, "[%s] exit state: <%s>", __FUNCTION__, (char *)"unknown");

    return TRUE;
}



//----------B: Local Function Define-------------
static STATE* GetState(FSM_INSTANCE *fsm_instance_ptr, U32BIT state)
{
    U32BIT loop = 0;
    STATE_MAP *state_map = fsm_instance_ptr->state_map_ptr;

    for (loop = 0; INVALID_STATE != state_map[loop].state; loop++)
    {
        if (state == state_map[loop].state)
            return state_map[loop].state_ptr;
    }

    return NULL;
}

static char* GetStateName(FSM_INSTANCE *fsm_instance_ptr, U32BIT state)
{
    U32BIT loop;
    STATE_MAP *state_map = fsm_instance_ptr->state_map_ptr;

    for (loop = 0; INVALID_STATE != state_map[loop].state; loop++)
    {
        if (state == state_map[loop].state)
            return state_map[loop].state_name;
    }

    return NULL;
}

static BOOLEAN TransState(FSM_INSTANCE *fsm_instance_ptr, U32BIT target_state, void *param_ptr)
{
    BOOLEAN ret1 = TRUE;
    BOOLEAN ret2 = TRUE;

    if (fsm_instance_ptr->current_state_ptr->state == target_state)
    {
        // Do nothing!
        ;
    }
    else
    {
        if (NULL != fsm_instance_ptr->current_state_ptr->state_exit_ptr)
        {
            ret1 = fsm_instance_ptr->current_state_ptr->state_exit_ptr(param_ptr);
        }

        fsm_instance_ptr->current_state_ptr = GetState(fsm_instance_ptr, target_state);

        if (NULL != fsm_instance_ptr->current_state_ptr && NULL != fsm_instance_ptr->current_state_ptr->state_enter_ptr)
        {
            ret2 = fsm_instance_ptr->current_state_ptr->state_enter_ptr(param_ptr);
        }
    }

    return (ret1 && ret2);
}


//static BOOLEAN default_state_enter(void *param_ptr);
//static BOOLEAN default_state_exit(void *param_ptr);
//----------E: Local Function Define-------------


