/*******************************************************************************
 * Copyright (c) 2018 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 *
 * This file is part of a DTVKit Software Component
 * You are permitted to copy, modify or distribute this file subject to the terms
 * of the DTVKit 1.0 Licence which can be found in licence.txt or at www.dtvkit.org
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * If you or your organisation is not a member of DTVKit then you have access
 * to this source code outside of the terms of the licence agreement
 * and you are expected to delete this and any associated files immediately.
 * Further information on DTVKit, membership and terms can be found at www.dtvkit.org
 *******************************************************************************/
/**
 * @brief   Set Top Box - Hardware Layer, Tuning/Front-End functions
 * @file    stbhwtun.c
 * @date    October 2018
 */

#define TUNER_DEBUG

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <pthread.h>
#include "dtv_log.h"
#define TAG  "STBHWTUN"

#include "frontend.h"
/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "cert_log.h"

#include "stbhwdef.h"
#include "stbhwtun.h"
#include "stbhwtun_inner.h"
#include "stbhwtun_ex.h"
#include "stbhwmem.h"
#include "stbhwos.h"
#include "stbhwresm.h"
#include "stbdpc.h"
#include "stbhwc.h"
#include "stbhwini.h"

#include "emu_internal.h"

/*---Macro Definitions for this file-----------------------------------------*/
#ifdef TUNER_DEBUG
#define TUN_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define TUN_DBG(x,...)
#endif

#define TUN_ERR(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#define TUN_INFO(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

/*---local (static) variable declarations for this file----------------------*/
static S_TUNER_STATUS *tuner_status = NULL;
static U8BIT num_paths;
static BOOLEAN resm_adc_requested = FALSE;
static BOOLEAN isTvPlatform = FALSE;
static U32BIT real_srate = SYMBOL_RATE_AUTO;
static E_STB_TUNE_CMODE real_cmode = TUNE_MODE_QAM_UNDEFINED;
void *tune_interface_sem = NULL;


/*---local function prototypes for this file---------------------------------*/
static BOOLEAN OpenTuner(S_TUNER_STATUS *tstatus);
static void CloseTuner(S_TUNER_STATUS *tstatus);
/*static*/ BOOLEAN StartTune(S_TUNER_STATUS *tstatus);
static BOOLEAN IsTunerLocked(S_TUNER_STATUS *tstatus);
static BOOLEAN IsTuningParameterMatched(S_TUNER_STATUS *tstatus, struct dvb_frontend_event event);
static void* TunerTask(void *param);
static void ClearTuner(S_TUNER_STATUS *tstatus);
static BOOLEAN SetSysType(S_TUNER_STATUS *tstatus, E_STB_TUNE_SIGNAL_TYPE sig_type);
/*static*/ U8BIT* GetSysTypeDebugString(E_STB_TUNE_SYSTEM_TYPE sys_type);
static BOOLEAN IsDiffSysType(S_TUNER_STATUS * tstatus);
/*static*/ E_TUNER_EVENT GetTunerLockStatus(U32BIT frontend_fd);
static void SetTunerT2PLP(U32BIT frontend_fd, U8BIT plp_id);
static BOOLEAN dvb_set_prop (U32BIT fd, const struct dtv_properties *prop);
static BOOLEAN dvb_wait_event (U32BIT fd, struct dvb_frontend_event *evt, int timeout);
static BOOLEAN dvbsx_blindscan_setsinglecable(U8BIT fd, struct dvbsx_singlecable_parameters *psinglecablePara);
static BOOLEAN dvb_blindscan_scan(U8BIT fd, struct dvbsx_blindscanpara *pbspara);
static BOOLEAN dvb_blindscan_getscanevent(int frontend_fd, struct dvbsx_blindscanevent *pbsevent);
static BOOLEAN dvb_blindscan_cancel(U8BIT path);
static BOOLEAN dvb_blindscan_continue(U8BIT path);
static BOOLEAN  AM_FEND_IBlindScanAPI_Start(U8BIT path);
static BOOLEAN  AM_FEND_IBlindScanAPI_GetScanEvent(U8BIT path, struct dvbsx_blindscanevent *pbsevent);
static BOOLEAN  AM_FEND_IBlindScanAPI_Exit(U8BIT path);
static BOOLEAN AM_FEND_BlindDump(U8BIT path);
static void* fend_blindscan_thread(void *arg);
static int SysTypeToFeMode(E_STB_TUNE_SYSTEM_TYPE sys_type);
static BOOLEAN SetBlindScanFeProperty(int fe_fd, E_STB_TUNE_SYSTEM_TYPE tuned_sys_type);
/*static*/ BOOLEAN SetFeProperty(int fe_fd, E_STB_TUNE_SYSTEM_TYPE tuned_sys_type);
static E_STB_TUNE_MODULATION GetTuneModulation(enum fe_modulation modulation);
static E_STB_TUNE_TCODERATE TuneGetActualTerrCodeRate(U8BIT path);
static BOOLEAN STB_TuneSetTone(U8BIT path, BOOLEAN use_22khz);
static BOOLEAN GetRealParamFromDriver(U8BIT path);

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialises the tuner component
 * @param   paths number of tuning paths to initialise
 */
void STB_TuneInitialise(U8BIT paths)
{
    char fe_name[24];
    int fe_fd = INVALID_FD;
    struct stat file_status;
    BOOLEAN adapter_found;
    U8BIT i;
    char buf[PROPERTY_VALUE_MAX] = { 0 };

    FUNCTION_START(STB_TuneInitialise);
    USE_UNWANTED_PARAM(paths);

    U8BIT init_try_count = 0;

    if (STB_Get_Prop("ro.vendor.platform.has.tvuimode", buf, sizeof(buf)))
    {
        if (strncmp(buf, "true", 4)==0)
        {
            isTvPlatform = TRUE;
        }
        else
        {
            isTvPlatform = FALSE;
        }
    }

    TUN_ERR("Current isTvPlatform [%s].", isTvPlatform ? "Yes": "No");
    CERT_Log_StartingUp("Current isTvPlatform [%s].", isTvPlatform ? "Yes": "No");

    /* Find out how many tuners are available */
    do
    {
        for (num_paths = 0, adapter_found = TRUE; adapter_found && (num_paths < AML_MAX_TUNER_NUM); )
        {
            // snprintf(fe_name, sizeof(fe_name), "/dev/dvb0.frontend%u", aml_hw_cfg.tuners[num_paths].frontend_idx);
            snprintf(fe_name, sizeof(fe_name), "/dev/dvb0.frontend%u", num_paths);
            TUN_DBG("[Path %u] Before get from driver, ts_input_idx=%u, signal_types=%u, support_dvbt2=%u, support_dvbt2=%u",
                    num_paths,
                    aml_hw_cfg.tuners[num_paths].ts_input_idx,
                    aml_hw_cfg.tuners[num_paths].signal_types,
                    aml_hw_cfg.tuners[num_paths].support_dvbt2,
                    aml_hw_cfg.tuners[num_paths].support_dvbs2);

            if (stat(fe_name, &file_status) == 0)
            {
                fe_fd = open(fe_name, O_RDWR | O_NONBLOCK);
                TUN_DBG("found %s, frontend_fd=%d", fe_name, fe_fd);
                CERT_Log_StartingUp("found %s, frontend_fd=%d", fe_name, fe_fd);
                if (fe_fd >= 0)
                {
                    aml_hw_cfg.tuners[num_paths].frontend_idx = num_paths;
                    STB_TuneSetActualTsInputIdx(num_paths, fe_fd);
                    STB_TuneSetActualSupportedSystemType(num_paths, fe_fd);
                    TUN_DBG("[Path %u] After get from driver, ts_input_idx=%u, signal_types=%u, support_dvbt2=%u, support_dvbt2=%u",
                            num_paths,
                            aml_hw_cfg.tuners[num_paths].ts_input_idx,
                            aml_hw_cfg.tuners[num_paths].signal_types,
                            aml_hw_cfg.tuners[num_paths].support_dvbt2,
                            aml_hw_cfg.tuners[num_paths].support_dvbs2);
                }
                else
                {
                    TUN_ERR("Failed to open tune[%d] %s, errno %d", num_paths, fe_name, errno);
                }

                close(fe_fd);
                fe_fd = INVALID_FD;

                num_paths++;
            }
            else
            {
                TUN_DBG("not found %s", fe_name);
                CERT_Log_StartingUp("not found %s", fe_name);

                adapter_found = FALSE;
            }
        }

        if ((!adapter_found) && (init_try_count < 10) && (num_paths == 0))
        {
            init_try_count++;
            sleep(1);

            TUN_DBG("retry: %d", init_try_count);
            CERT_Log_StartingUp("retry: %d", init_try_count);
        }
        else
        {
            break;
        }
    } while (1);

    if (num_paths != 0)
    {
        TUN_DBG("Assign num_paths=%d to aml_hw_cfg.tuner_num", num_paths);
        aml_hw_cfg.tuner_num = num_paths;

        tuner_status = (S_TUNER_STATUS *)STB_MEMGetSysRAM(sizeof(S_TUNER_STATUS) * num_paths);

        if (tuner_status != NULL)
        {
            memset(tuner_status, 0, sizeof(S_TUNER_STATUS) * num_paths);

            if (HW_ISDB_SYSTEM == STB_HWGetDtvSystem())
            {
                stb_tune_fsm_init(num_paths);
            }

            /* Check the status of each tuner */
            for (i = 0; i != num_paths; i++)
            {
                tuner_status[i].path = i;
                tuner_status[i].frontend_fd = INVALID_FD;
                tuner_status[i].state = TUNER_IDLE;
                tuner_status[i].stop = FALSE;
                tuner_status[i].sys_type = TUNE_SYSTEM_TYPE_UNKNOWN;
                tuner_status[i].tuned_sys_type = TUNE_SYSTEM_TYPE_UNKNOWN;
                tuner_status[i].auto_relock = FALSE;
                tuner_status[i].tuner_types = aml_hw_cfg.tuners[i].signal_types;
                tuner_status[i].signal_type = TUNE_SIGNAL_NONE;
                tuner_status[i].tuning_params_changed = FALSE;
                tuner_status[i].mutex = STB_OSCreateMutex();
                tuner_status[i].tune_sem = STB_OSCreateCountSemaphore(0);
                tuner_status[i].tune_sem_lock = STB_OSCreateCountSemaphore(0);
                tuner_status[i].tunertask_sem = STB_OSCreateCountSemaphore(0);
                tuner_status[i].plp_id = -1;
                tuner_status[i].frontend_usage = 0;
                tuner_status[i].lock_flags = 0;
                tuner_status[i].search_mode = FALSE;
                pthread_mutex_init(&tuner_status[i].lock, NULL);

                #if 0
                if (STB_OSCreateTask(TunerTask, (void *)&tuner_status[i], TUNE_TASK_STACK_SIZE,
                                         TUNE_TASK_PRIORITY, (U8BIT *)"TunerTask") == NULL)
                {
                    TUN_ERR("Failed to create task for tuner %u", i);
                    CERT_Log_StartingUp("Failed to create task for tuner %u", i);
                }
                #else
                if (HW_ISDB_SYSTEM != STB_HWGetDtvSystem())
                {
                    if (STB_OSCreateTask(TunerTask, (void *)&tuner_status[i], TUNE_TASK_STACK_SIZE,
                                         TUNE_TASK_PRIORITY, (U8BIT *)"TunerTask") == NULL)
                    {
                        TUN_ERR("Failed to create task for tuner %u", i);
                        CERT_Log_StartingUp("Failed to create task for tuner %u", i);
                    }
                }
                else
                {
                    stb_tune_fsm_create(i, &tuner_status[i], STATE_TUNER_IDLE);
                }
                #endif
            }
        }
    }
    else
    {
        TUN_ERR("No tuners found!");
        CERT_Log_StartingUp("No tuners found!");
    }
    if (NULL == tune_interface_sem)
    {
        tune_interface_sem= STB_OSCreateSemaphore();
    }

    if (NULL == tune_interface_sem)
    {
        CERT_Log_StartingUp("tune_interface_sem create error");
    }

    FUNCTION_FINISH(STB_TuneInitialise);
}

/**
 * @brief   Enables or disabled auto tuner relocking
 * @param   path the tuner path to configure
 * @param   state TRUE enables relocking, FALSE disables it
 */
void STB_TuneAutoRelock(U8BIT path, BOOLEAN state)
{
    FUNCTION_START(STB_TuneAutoRelock);

    if (path < num_paths)
    {
        tuner_status[path].auto_relock = state;
    }

    FUNCTION_FINISH(STB_TuneAutoRelock);
}


/**
 * @brief Assign actual ts_input_idx (obtained from driver) to aml_hw_cfg
 * @param path The tuner path to set up
 * @param frontend_fd The FD identifying the fe_name, which is required to
 *                    perform I/O control operation
 */
void STB_TuneSetActualTsInputIdx(U8BIT path, S32BIT frontend_fd)
{
    struct dtv_property cmd;
    struct dtv_properties props;

    FUNCTION_START(STB_TuneSetActualTsInputIdx);

    if (frontend_fd != INVALID_FD)
    {
        memset(&cmd, 0, sizeof(struct dtv_property));

        cmd.cmd = DTV_TS_INPUT;
        props.num = 1;
        props.props = &cmd;

        if (ioctl(frontend_fd, FE_GET_PROPERTY, &props) >= 0)
        {
            aml_hw_cfg.tuners[path].ori_tsinput_idx =
            aml_hw_cfg.tuners[path].ts_input_idx = cmd.u.data;
            TUN_DBG("%u: ts_input_idx=%lu", path, cmd.u.data);
        }
        else
        {
            TUN_ERR("%u: Failed to read ts_input from driver, errno %d", path, errno);
        }
    }
    else
    {
        TUN_DBG("path=%u, num_paths=%u, aml_hw_cfg.tuner_num=%u, frontend_fd=%u",
                path, num_paths, aml_hw_cfg.tuner_num, frontend_fd);
    }

    FUNCTION_FINISH(STB_TuneSetActualTsInputIdx);
}

/**
 * @brief Assign actual supported delivery system type (obtained from driver)
 *        to aml_hw_cfg
 * @param path The tuner path to set up
 * @param frontend_fd The FD identifying the fe_name, which is required to
 *                    perform I/O control operation
 */
void STB_TuneSetActualSupportedSystemType(U8BIT path, S32BIT frontend_fd)
{
    enum fe_delivery_system delsys;
    struct dtv_property cmd;
    struct dtv_properties props;

    FUNCTION_START(STB_TuneSetActualSupportedSystemType);

    delsys = SYS_UNDEFINED;
    if (frontend_fd != INVALID_FD)
    {
        memset(&cmd, 0, sizeof(struct dtv_property));

        cmd.cmd = DTV_ENUM_DELSYS;
        props.num = 1;
        props.props = &cmd;

        if (ioctl(frontend_fd, FE_GET_PROPERTY, &props) >= 0)
        {
            aml_hw_cfg.tuners[path].signal_types  = 0;
            aml_hw_cfg.tuners[path].support_dvbt2 = 0;
            aml_hw_cfg.tuners[path].support_dvbs2 = 0;
            for (U16BIT i = 0; i < cmd.u.buffer.len; i++)
            {
                delsys = cmd.u.buffer.data[i];
                switch (delsys)
                {
                    case SYS_DVBT:
                    case SYS_DVBT2:
                        aml_hw_cfg.tuners[path].signal_types |= TUNE_SIGNAL_COFDM;
                        if (delsys == SYS_DVBT2)
                        {
                            aml_hw_cfg.tuners[path].support_dvbt2 = 1;
                        }
                        break;
                    case SYS_DVBS:
                    case SYS_DVBS2:
                        aml_hw_cfg.tuners[path].signal_types |= TUNE_SIGNAL_QPSK;
                        if (delsys == SYS_DVBS2)
                        {
                            aml_hw_cfg.tuners[path].support_dvbs2 = 1;
                        }
                        break;
                    case SYS_DVBC_ANNEX_A:
                    case SYS_DVBC_ANNEX_C:
                        aml_hw_cfg.tuners[path].signal_types |= TUNE_SIGNAL_QAM;
                        break;
                    default:
                        break;
                }
                TUN_DBG("%u: supported delsys[%u]=%u", path, i, cmd.u.buffer.data[i]);
            }
        }
        else
        {
            TUN_ERR("%u: Failed to read ts_input from driver, errno %d", path, errno);
        }
    }

    FUNCTION_FINISH(STB_TuneSetActualSupportedSystemType);
}

/**
 * @brief   Gets the signal types of the given tuner path.
 *          This will be a bitmask of supported types defined by E_STB_TUNE_SIGNAL_TYPE
 * @param   path tuner path
 * @return  the signal types supported by the given tuner
 */
U16BIT STB_TuneGetSignalType(U8BIT path)
{
    U16BIT sig_type;

    FUNCTION_START(STB_TuneGetSignalType);

    if (path < num_paths)
    {
        if (tuner_status[path].signal_type == TUNE_SIGNAL_NONE)
        {
            sig_type = tuner_status[path].tuner_types;
        }
        else
        {
            sig_type = tuner_status[path].signal_type;
        }
    }
    else
    {
        sig_type = TUNE_SIGNAL_NONE;
    }

    FUNCTION_FINISH(STB_TuneGetSignalType);

    return sig_type;
}

U16BIT STB_TuneGetActualSignalType(U8BIT path)
{
    U16BIT sig_type;

    FUNCTION_START(STB_TuneGetSignalType);

    if (path < num_paths)
    {
        sig_type = tuner_status[path].signal_type;
    }
    else
    {
        sig_type = TUNE_SIGNAL_NONE;
    }

    TUN_DBG("%u: current signal_type=%u", path, sig_type);

    FUNCTION_FINISH(STB_TuneGetSignalType);

    return sig_type;
}

/**
 * @brief   This function is only relevant for tuners that support more than one signal type;
 *          for tuners that don't support more than one signal type it can be a blank function.
 *          It will be called to inform the platform which of the supported signal types is being
 *          used.
 * @param   path tuner path
 * @param   type signal type that is being used for this tuner
 */
void STB_TuneSetSignalType(U8BIT path, E_STB_TUNE_SIGNAL_TYPE type)
{
    S_TUNER_STATUS *tstatus;
    E_TUNER_STATE state;

    FUNCTION_START(STB_TuneSetSignalType);

    if (path < num_paths)
    {
        tstatus = &tuner_status[path];
        pthread_mutex_lock(&tstatus->lock);
        TUN_DBG("%u: current type=%u, new type=%u frontend_fd:%d", path, tstatus->signal_type, type, tstatus->frontend_fd);

        if (tstatus->signal_type != type)
        {
            if (tstatus->frontend_fd != INVALID_FD)
            {
                STB_OSMutexLock(tstatus->mutex);
                state = tstatus->state;
                STB_OSMutexUnlock(tstatus->mutex);

                if (state != TUNER_IDLE && state != TUNER_EXITED)
                {
                    STB_TuneStopTuner(path);

                    /* after search and if have no channel, need release FE. */
                    if (STB_TuneIsTvPlatform() && type == TUNE_SIGNAL_NONE && STB_TuneIsSearchMode(path))
                    {
                        CloseTuner(tstatus);
                    }
                }

                tstatus->signal_type = TUNE_SIGNAL_NONE;
            }

            if (type != TUNE_SIGNAL_NONE && ((tstatus->tuner_types & type) != 0) && SetSysType(tstatus, type))
            {
                tstatus->signal_type = type;
            }
        }

        pthread_mutex_unlock(&tstatus->lock);
    }

    FUNCTION_FINISH(STB_TuneSetSignalType);
}

static int SysTypeToFeMode(E_STB_TUNE_SYSTEM_TYPE sys_type)
{
    int fe_mode = SYS_UNDEFINED;
    switch (sys_type)
    {
        case TUNE_SYSTEM_TYPE_DVBT:
            fe_mode = SYS_DVBT;
            break;

        case TUNE_SYSTEM_TYPE_DVBT2:
            fe_mode = SYS_DVBT2;
            break;

        case TUNE_SYSTEM_TYPE_DVBS:
            fe_mode = SYS_DVBS;
            break;

        case TUNE_SYSTEM_TYPE_DVBS2:
            fe_mode = SYS_DVBS2;
            break;

        case TUNE_SYSTEM_TYPE_DVBC:
            fe_mode = SYS_DVBC_ANNEX_A;
            break;

        case TUNE_SYSTEM_TYPE_ISDBT:
            fe_mode = SYS_ISDBT;
            break;

        case TUNE_SYSTEM_TYPE_ANALOG:
            fe_mode = SYS_ANALOG;
            break;

        default:
            TUN_ERR("not support type:%d", sys_type);
            break;
    }

    return fe_mode;
}

static BOOLEAN SetBlindScanFeProperty(int fe_fd, E_STB_TUNE_SYSTEM_TYPE tuned_sys_type)
{
    if (fe_fd == INVALID_FD)
        return FALSE;

    int fe_mode = SysTypeToFeMode(tuned_sys_type);
    if (fe_mode == SYS_UNDEFINED)
        return FALSE;

    struct dtv_property *property = STB_MEMGetSysRAM(sizeof(struct dtv_property));
    if (NULL == property)
        return FALSE;

    memset(property, 0, sizeof(struct dtv_property));

    property->cmd = DTV_DELIVERY_SYSTEM;
    property->u.data = fe_mode;
    TUN_DBG("set blind fe prop cmd[%d] data[%d]", property->cmd, property->u.data);

    struct dtv_properties prop;
    prop.num = 1;
    prop.props = property;

    BOOLEAN ret = dvb_set_prop(fe_fd, &prop);
    if (!ret)
    {
        TUN_DBG( "set blind fe prop cmd error");
    }

    if (property != NULL)
    {
        STB_MEMFreeSysRAM(property);
        property = NULL;
    }

    return ret;
}

/*static*/ BOOLEAN SetFeProperty(int fe_fd, E_STB_TUNE_SYSTEM_TYPE tuned_sys_type)
{
    if (fe_fd == INVALID_FD)
        return FALSE;

    int fe_mode = SysTypeToFeMode(tuned_sys_type);
    if (fe_mode == SYS_UNDEFINED)
        return FALSE;

    struct dtv_property *property = STB_MEMGetSysRAM(sizeof(struct dtv_property));
    if (NULL == property)
        return FALSE;

    memset(property, 0, sizeof(struct dtv_property));

    property->cmd = DTV_DELIVERY_SYSTEM;
    property->u.data = fe_mode;

    struct dtv_properties prop;
    prop.num = 1;
    prop.props = property;
    TUN_DBG("set fe prop cmd[%d] data[%d]", property->cmd, property->u.data);

    BOOLEAN ret = dvb_set_prop(fe_fd, &prop);
    if (!ret)
    {
        TUN_DBG( "set fe prop cmd error");
    }

    if (property != NULL)
    {
        STB_MEMFreeSysRAM(property);
        property = NULL;
    }

    return ret;

}

static E_STB_TUNE_MODULATION GetTuneModulation(enum fe_modulation modulation)
{
    switch (modulation)
    {
    case QPSK:
        return TUNE_MOD_QPSK;
    case PSK_8:
        return TUNE_MOD_8PSK;
    case QAM_16:
        return TUNE_MOD_16QAM;
    case APSK_16:
        return TUNE_MOD_16APSK;
    case APSK_32:
        return TUNE_MOD_32APSK;
    default:
        return TUNE_MOD_AUTO;
    }

    return TUNE_MOD_AUTO;
}

 /**
 * @brief   Stops any locking attempt, or unlocks if locked
 * @param   path the tuner path to stop
 */
static void TuneStopTuner(S_TUNER_STATUS *tstatus)
{
    E_TUNER_STATE state;

    FUNCTION_START(TuneStopTuner);

    if (NULL != tstatus)
    {
        STB_OSMutexLock(tstatus->mutex);
        state = tstatus->state;
        STB_OSMutexUnlock(tstatus->mutex);

        if (state != TUNER_IDLE && state != TUNER_EXITED)
        {
            TUN_DBG("%u: Stopping tuning...", tstatus->path);

            STB_OSMutexLock(tstatus->mutex);
            tstatus->stop = TRUE;
            STB_OSMutexUnlock(tstatus->mutex);

            while (state != TUNER_IDLE && state != TUNER_EXITED)
            {
                STB_OSTaskDelay(30);

                STB_OSMutexLock(tstatus->mutex);
                state = tstatus->state;
                STB_OSMutexUnlock(tstatus->mutex);
            }

            /* The tuner state can change to idle due to it losing lock, in which case
             * the stop flag will still be set, so reset now to be sure */
            STB_OSMutexLock(tstatus->mutex);
            tstatus->stop = FALSE;
            STB_OSMutexUnlock(tstatus->mutex);

            //ClearTuner(tstatus);

            //tstatus->tuned_sys_type = TUNE_SYSTEM_TYPE_UNKNOWN;
        }

        TUN_DBG("%u: Tuner stopped", tstatus->path);
    }

    FUNCTION_FINISH(TuneStopTuner);
}
/**
 * @brief   Starts the tuner, it will then attempt to lock specified signal
 * @param   path the tuner path to start
 * @param   freq the frequency to tune to
 * @param   srate the symbol rate to lock
 * @param   fec The forward error correction rate
 * @param   freq_off The frequency offset to use
 * @param   tmode The COFDM mode
 * @param   tbwidth The signal bandwidth
 * @param   cmode The QAM mode
 * @param   anlg_vtype The type of video for analogue tuner
 * @note     unrequired parameters can be passed as 0 (zero)
 */
void STB_TuneStartTuner(U8BIT path, U32BIT freq, U32BIT srate, E_STB_TUNE_FEC fec,
                        S8BIT freq_off, E_STB_TUNE_TMODE tmode, E_STB_TUNE_TBWIDTH tbwidth,
                        E_STB_TUNE_CMODE cmode, E_STB_TUNE_ANALOG_VIDEO_TYPE anlg_vtype)
{
    S_TUNER_STATUS *tstatus;
    E_TUNER_STATE state;
    BOOLEAN start_tuning;
    BOOLEAN sem_ret;

    FUNCTION_START(STB_TuneStartTuner);
    USE_UNWANTED_PARAM(freq_off);
    USE_UNWANTED_PARAM(anlg_vtype);

    TUN_ERR("%s enter",__FUNCTION__);

    if (path < num_paths)
    {
        if (HW_ISDB_SYSTEM == STB_HWGetDtvSystem())
        {
            stb_tune_start_tuner(&tuner_status[path],
                                freq, srate, fec,
                                freq_off, tmode, tbwidth,
                                cmode, anlg_vtype);

            goto EXIT;
        }

        STB_OSSemaphoreWait(tune_interface_sem);
        tstatus = &tuner_status[path];

        while (STB_TuneIsTvPlatform() && tstatus->state == TUNER_EXITED && !STB_TuneIsSearchMode(path))
        {
            TUN_DBG("%u: tunertask_sem entry sem_wait:%p.",
                    tstatus->path, tstatus->tunertask_sem);
            sem_ret = STB_OSSemaphoreWaitTimeout(tstatus->tunertask_sem, 1000);
            TUN_DBG("%u: tunertask_sem exit sem_timedwait:%p, sem_ret:%d, tstatus->state:%d.",
                    tstatus->path, tstatus->tunertask_sem, sem_ret, tstatus->state);
        }

        TUN_DBG("%u: freq %lu, srate %lu fec %d sys_type %s, signal_type %d", path, freq, srate, fec,
                    GetSysTypeDebugString(tstatus->sys_type), tstatus->signal_type);

        if (((tstatus->signal_type == TUNE_SIGNAL_COFDM) &&
                ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT) ||
                 ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2) && (tstatus->delivery_system == SYS_DVBT2)))) ||
                ((tstatus->signal_type == TUNE_SIGNAL_QPSK) &&
                 ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS) ||
                  ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS2) && (tstatus->delivery_system == SYS_DVBS2)))) ||
                ((tstatus->signal_type == TUNE_SIGNAL_QAM) && (tstatus->delivery_system == SYS_DVBC_ANNEX_A)) ||
                ((tstatus->sys_type == TUNE_SYSTEM_TYPE_ISDBT) && (tstatus->delivery_system == SYS_ISDBT)))
        {
            start_tuning = FALSE;
            STB_TimeConsumeDebug("Tune lock start");

            if (IsDiffSysType(tstatus))
            {
                start_tuning = TRUE;
                tstatus->tuned_sys_type = tstatus->sys_type;
                SetFeProperty(tstatus->frontend_fd, tstatus->tuned_sys_type);
            }

            if (tstatus->freq != freq)
            {
                start_tuning = TRUE;
                tstatus->freq = freq;
            }

            switch (tstatus->signal_type)
            {
                case TUNE_SIGNAL_COFDM:
                    if ((tstatus->u.terr.tmode != tmode) || (tstatus->u.terr.tbwidth != tbwidth))
                    {
                        start_tuning = TRUE;
                        tstatus->u.terr.tmode = tmode;
                        tstatus->u.terr.tbwidth = tbwidth;
                    }

                    break;

                case TUNE_SIGNAL_QAM:
                    if ((tstatus->u.cab.cmode != cmode) || (tstatus->u.cab.srate != srate))
                    {
                        start_tuning = TRUE;
                        tstatus->u.cab.cmode = cmode;
                        tstatus->u.cab.srate = srate;
                    }

                    break;

                case TUNE_SIGNAL_QPSK:
                    if ((tstatus->u.sat.fec != fec) || (tstatus->u.sat.srate != srate))
                    {
                        start_tuning = TRUE;
                        tstatus->u.sat.fec = fec;
                        tstatus->u.sat.srate = srate;
                    }

                    break;

                case TUNE_SIGNAL_ISDBT:
                    if (tstatus->u.isdbt.tbwidth != tbwidth)
                    {
                        start_tuning = TRUE;
                        tstatus->u.isdbt.tbwidth = tbwidth;
                    }

                    break;

                default:
                    break;
            }

            pthread_mutex_lock(&tstatus->lock); //Keep serial operation that tune on the same path.
            STB_OSMutexLock(tstatus->mutex);
            state = tstatus->state;
            STB_OSMutexUnlock(tstatus->mutex);

            if (start_tuning || tstatus->tuning_params_changed || GetTunerLockStatus(tstatus->frontend_fd) != TUNER_STATE_LOCKED)
            {
                TUN_DBG("start_tuning: %d tuning_params_changed:%d", start_tuning,tstatus->tuning_params_changed);

                if (state != TUNER_IDLE && state != TUNER_EXITED)
                {
                    TuneStopTuner(tstatus);
                }

                tstatus->tuning_params_changed = FALSE;

                if (tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2)
                {
                    SetTunerT2PLP(tstatus->frontend_fd, tstatus->plp_id);
                }

                #ifdef EMUTUNNER_ENABLE
                if (EmuTunerStart(path, freq, tstatus->signal_type))
                {
                    ClearTuner(tstatus);
                    STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_LOCKED, &tstatus->path, sizeof(U8BIT));
                }
                else
                #endif
                {
                    if (StartTune(tstatus))
                    {
                        STB_OSSemaphoreSignal(tstatus->tune_sem);
                    }
                    else
                    {
                        STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path,
                                        sizeof(U8BIT));
                    }
                }
            }
            else
            {
                /* Already tuned to the required transport */
                TUN_DBG("%u: Already tuned", tstatus->path);
                #if EMUTUNNER_ENABLE
                if (EmuTunerGetState(path))
                {
                    EmuTunerReset(path);
                }
                else if (state == TUNER_IDLE)
                #else
                if (state == TUNER_IDLE)
                #endif
                {
                    tstatus->lock_flags |= FEND_FL_LOCK;
                    STB_OSSemaphoreSignal(tstatus->tune_sem);

                }

                STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_LOCKED, &tstatus->path, sizeof(U8BIT));
            }

            pthread_mutex_unlock(&tstatus->lock);
        }
        else
        {
            TUN_ERR("%u: system type %u not supported", tstatus->path, tstatus->sys_type);

            STB_OSMutexLock(tstatus->mutex);
            state = tstatus->state;
            STB_OSMutexUnlock(tstatus->mutex);

            if (state != TUNER_IDLE && state != TUNER_EXITED)
            {
                TuneStopTuner(tstatus);
            }

            STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path, sizeof(U8BIT));
        }
        STB_OSSemaphoreSignal(tune_interface_sem);
    }

EXIT:
    TUN_ERR("%s out",__FUNCTION__);

    FUNCTION_FINISH(STB_TuneStartTuner);
}

/**
 * @brief   Stops any locking attempt, or unlocks if locked
 * @param   path the tuner path to stop
 */
void STB_TuneStopTuner(U8BIT path)
{
    S_TUNER_STATUS *tstatus;
    E_TUNER_STATE state;

    FUNCTION_START(STB_TuneStopTuner);

    TUN_ERR("%s enter",__FUNCTION__);

    if (path < num_paths)
    {
        if (HW_ISDB_SYSTEM == STB_HWGetDtvSystem())
        {
            stb_tune_stop_tuner(&tuner_status[path]);
        }

        STB_OSSemaphoreWait(tune_interface_sem);
        tstatus = &tuner_status[path];

        #ifdef EMUTUNNER_ENABLE
        STB_OSMutexLock(tstatus->mutex);
        EmuTunerStop(path);
        STB_OSMutexUnlock(tstatus->mutex);
        #endif

        TuneStopTuner(tstatus);
        STB_OSSemaphoreSignal(tune_interface_sem);
    }

    TUN_ERR("%s out",__FUNCTION__);

    FUNCTION_FINISH(STB_TuneStopTuner);
}

/**
 * @brief   Returns the minimum tuner symbol rate
 * @param   path the tuner path to query
 * @return  minimum tuner symbol rate
 */
U32BIT STB_TuneGetMinTunerSymbolRate(U8BIT path)
{
    U32BIT symbol_rate = TUNER_MIN_SRATE;

    FUNCTION_START(STB_TuneGetMinTunerSymbolRate);

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        symbol_rate = tuner_status[path].fe_info.symbol_rate_min ? tuner_status[path].fe_info.symbol_rate_min : TUNER_MIN_SRATE;
    }

    FUNCTION_FINISH(STB_TuneGetMinTunerSymbolRate);

    return(symbol_rate);
}

/**
 * @brief   Returns the maxmum tuner symbol rate
 * @param   path the tuner path to query
 * @return  maxmum tuner symbol rate
 */
U32BIT STB_TuneGetMaxTunerSymbolRate(U8BIT path)
{
    U32BIT symbol_rate = TUNER_MAX_SRATE;

    FUNCTION_START(STB_TuneGetMaxTunerSymbolRate);

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        symbol_rate = tuner_status[path].fe_info.symbol_rate_max ? tuner_status[path].fe_info.symbol_rate_max : TUNER_MAX_SRATE;
    }

    FUNCTION_FINISH(STB_TuneGetMaxTunerSymbolRate);

    return(symbol_rate);
}


/**
 * @brief   Returns the minimum tuner frequency in KHz
 * @param   path the tuner path to query
 * @return  minimum frequency in Khz
 */
U32BIT STB_TuneGetMinTunerFreqKHz(U8BIT path)
{
    U32BIT min_freq;

    FUNCTION_START(STB_TuneGetMinTunerFreqKHz);

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        /* Return the frequency in KHz */
        min_freq = tuner_status[path].signal_type == TUNE_SIGNAL_QPSK ? tuner_status[path].fe_info.frequency_min : tuner_status[path].fe_info.frequency_min / 1000;
    }
    else
    {
        min_freq = 0;
    }

    FUNCTION_FINISH(STB_TuneGetMinTunerFreqKHz);

    return(min_freq);
}

/**
 * @brief   Returns the maximum tuner frequency in KHz
 * @param   path the tuner path to query
 * @return  maximum frequency in Khz
 */
U32BIT STB_TuneGetMaxTunerFreqKHz(U8BIT path)
{
    U32BIT max_freq;

    FUNCTION_START(STB_TuneGetMaxTunerFreqKHz);

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        /* Return the frequency in KHz */
        max_freq = tuner_status[path].signal_type == TUNE_SIGNAL_QPSK ? tuner_status[path].fe_info.frequency_max : tuner_status[path].fe_info.frequency_max / 1000;
    }
    else
    {
        max_freq = 0;
    }

    FUNCTION_FINISH(STB_TuneGetMaxTunerFreqKHz);

    return(max_freq);
}

static BOOLEAN IsDataConversionNeeded(U8BIT path)
{
    BOOLEAN retval = TRUE;

    if (ioctl(tuner_status[path].frontend_fd, FE_GET_INFO, &(tuner_status[path].fe_info)) >= 0)
    {
        if (NULL != strstr(tuner_status[path].fe_info.name,"CXD2856"))
        {
            TUN_INFO("fe_info.name=%s", tuner_status[path].fe_info.name);
            retval = FALSE;  //For cxd2856, not need to convert to percentage
        }
    }
    return retval;
}

static U8BIT StrengthToSSI(U8BIT path, S16BIT strength)
{
    int ssi = 0;

    switch (STB_TuneGetSignalType(path))
    {
        case TUNE_SIGNAL_COFDM:
            if ((STB_TuneGetActualTerrHpCodeRate(path) == TUNE_TCODERATE_2_3 &&
                STB_TuneGetActualTerrConstellation(path) == TUNE_TCONST_QAM256) ||
                (STB_TuneGetActualTerrHpCodeRate(path) == TUNE_TCODERATE_3_4 &&
                STB_TuneGetActualTerrConstellation(path) == TUNE_TCONST_QAM64))
            {
                if (strength <= -95)
                    ssi = 0;
                else if (strength <= -85)
                    ssi = 5 * (95 + strength) / 10;
                else if (strength <= -75)
                    ssi = 5 + 17 * (85 + strength) / 10;
                else if (strength <= -65)
                    ssi = 22 + 40 * (75 + strength) / 10;
                else if (strength <= -55)
                    ssi = 62 + 30 * (65 + strength) / 10;
                else if (strength <= -45)
                    ssi = 92 + 8 * (55 + strength) / 10;
                else
                    ssi = 100;
            }
            else if (STB_TuneGetActualTerrHpCodeRate(path) == TUNE_TCODERATE_3_4 &&
                STB_TuneGetActualTerrConstellation(path) == TUNE_TCONST_QAM256)
            {
                if (strength <= -95)
                    ssi = 0;
                else if (strength <= -85)
                    ssi = 3 * (95 + strength) / 10;
                else if (strength <= -75)
                    ssi = 3 + 11 * (85 + strength) / 10;
                else if (strength <= -65)
                    ssi = 14 + 40 * (75 + strength) / 10;
                else if (strength <= -55)
                    ssi = 54 + 35 * (65 + strength) / 10;
                else if (strength <= -45)
                    ssi = 89 + 9 * (55 + strength) / 10;
                else if (strength <= -40)
                    ssi = 98 + 2 * (45 + strength) / 5;
                else
                    ssi = 100;
            }
            else
            {
                if (strength <= -95)
                    ssi = 0;
                else if (strength <= -85)
                    ssi = 7 * (95 + strength) / 10;
                else if (strength <= -75)
                    ssi = 7 + 23 * (85 + strength) / 10;
                else if (strength <= -65)
                    ssi = 30 + 40 * (75 + strength) / 10;
                else if (strength <= -55)
                    ssi = 70 + 23 * (65 + strength) / 10;
                else if (strength <= -45)
                    ssi = 93 + 7 * (55 + strength) / 10;
                else
                    ssi = 100;
            }
            break;

        case TUNE_SIGNAL_QAM:
            if (strength <= -70)
                ssi = 0;
            else if (strength <= -60)
                ssi = 20 * (70 + strength) / 10;
            else if (strength <= -50)
                ssi = 20 + 30 * (60 + strength) / 10;
            else if (strength <= -40)
                ssi = 50 + 40 * (50 + strength) / 10;
            else if (strength <= -30)
                ssi = 90 + 10 * (40 + strength) / 10;
            else
                ssi = 100;
            break;

        case TUNE_SIGNAL_QPSK:
            if (strength <= -93)
                ssi = 0;
            else if (strength <= -90)
                ssi = 3 * (93 + strength) / 3;
            else if (strength <= -85)
                ssi = 3 + 7 * (90 + strength) / 5;
            else if (strength <= -75)
                ssi = 10 + 25 * (85 + strength) / 10;
            else if (strength <= -65)
                ssi = 35 + 45 * (75 + strength) / 10;
            else if (strength <= -55)
                ssi = 80 + 10 * (65 + strength) / 10;
            else if (strength <= -45)
                ssi = 90 + 8 * (55 + strength) / 10;
            else if (strength <= -35)
                ssi = 98 + 2 * (45 + strength) / 10;
            else
                ssi = 100;
            break;

        case TUNE_SIGNAL_ISDBT:

            if (strength <= -95)
                ssi = 0;
            else if (strength <= -85)
                ssi = 7 * (95 + strength) / 10;
            else if (strength <= -75)
                ssi = 7 + 23 * (85 + strength) / 10;
            else if (strength <= -65)
                ssi = 30 + 40 * (75 + strength) / 10;
            else if (strength <= -55)
                ssi = 70 + 23 * (65 + strength) / 10;
            else if (strength <= -45)
                ssi = 93 + 7 * (55 + strength) / 10;
            else
                ssi = 100;

            break;

        default:
            break;
    }

    return (U8BIT)ssi;
}

/**
 * @brief   Returns the current signal dBuV
 * @param   path the tuner path to query
 * @return  the signal dBuV as percentage of maximum (0-100)
 */
U8BIT STB_TuneGetSignaldBuV(U8BIT path)
{
    U8BIT retval = 0;
    S16BIT strength;

    FUNCTION_START(STB_TuneGetSignaldBuV);

#ifdef EMUTUNNER_ENABLE
    retval = EmuTunerGetSignalStrength(path);
    if (retval > 0)
    {
        return retval;
    }
#endif

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        if (IsTunerLocked(&tuner_status[path]))
        {
            /* New method of reading signal strength not supported, so use the old API */
            if (ioctl(tuner_status[path].frontend_fd, FE_READ_SIGNAL_STRENGTH, (U16BIT *)&strength) >= 0)
            {
                retval = (U8BIT)(109 + strength); // plus 108.75(dBm to dBuV for 75 ohms)
                TUN_DBG("%u: dBuV:%u(strength:%d)", path, retval, strength);
            }
            else
            {
                TUN_ERR("%u: Failed to get signal strength, errno %d", path, errno);
            }
        }
    }

    FUNCTION_FINISH(STB_TuneGetSignaldBuV);

    return retval;
}

/**
 * @brief   Returns the current signal strength
 * @param   path the tuner path to query
 * @return  the signal strength as percentage of maximum (0-100)
 */
U8BIT STB_TuneGetSignalStrength(U8BIT path)
{
    U8BIT retval = 0;
    S16BIT strength;

    FUNCTION_START(STB_TuneGetSignalStrength);

    #ifdef EMUTUNNER_ENABLE
    retval = EmuTunerGetSignalStrength(path);
    if (retval > 0)
    {
        return retval;
    }
    #endif

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        if (IsTunerLocked(&tuner_status[path]))
        {
           retval = STB_TuneReadSignalStrength(path);
        }
    }

    FUNCTION_FINISH(STB_TuneGetSignalStrength);

    return retval;
}

U8BIT STB_TuneReadSignalStrength(U8BIT path)
{
    U8BIT retval = 0;
    S16BIT strength;

    FUNCTION_START(STB_TuneGetSignalStrength);

    if (path < num_paths && tuner_status[path].frontend_fd != INVALID_FD)
    {
        if (ioctl(tuner_status[path].frontend_fd, FE_READ_SIGNAL_STRENGTH, (U16BIT *)&strength) >= 0)
        {
            /* Strength is returned as a percentage */
            if (IsDataConversionNeeded(path))
            {
                retval = StrengthToSSI(path, strength);
            }
            else
            {
                retval = strength;
            }

            TUN_DBG("%u: %u%%(strength:%d)", path, retval, strength);
        }
        else
        {
            TUN_ERR("%u: Failed to get signal strength, errno %d", path, errno);
        }
    }

    FUNCTION_FINISH(STB_TuneGetSignalStrength);

    return retval;
}

/**
 * @brief   Returns the current data integrity
 * @param   path the tuner path to query
 * @return  the data integrity as percentage of maximum possible (0-100)
 * @todo     Confirm DVB API BER units
 */
U32BIT STB_TuneGetDataIntegrity(U8BIT path)
{
    U32BIT retval;
    __u32 ber;

    FUNCTION_START(STB_TuneGetDataIntegrity);

    retval = 0;

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        //if (IsTunerLocked(&tuner_status[path]))
        {
            if (ioctl(tuner_status[path].frontend_fd, FE_READ_BER, &ber) >= 0)
            {
                retval = (U32BIT)ber;
                TUN_DBG("%u: BER=%u(e-10)", path, retval);
            }
            else
            {
                TUN_ERR("%u: FE_READ_BER failed, errno %d", path, errno);
            }
        }
    }

    FUNCTION_FINISH(STB_TuneGetDataIntegrity);

    return retval;
}

static U8BIT SNR10ToSQI(U8BIT path, S16BIT snr)
{
    int sqi = 0;

    switch (STB_TuneGetSignalType(path))
    {
        case TUNE_SIGNAL_COFDM:
            if (STB_TuneGetSystemType(path) == TUNE_SYSTEM_TYPE_DVBT2 &&
                STB_TuneGetActualTerrConstellation(path) == TUNE_TCONST_QAM256)
            {
                if (STB_TuneGetActualTerrHpCodeRate(path) == TUNE_TCODERATE_3_5)
                {
                    if (snr <= 160)
                        sqi = 0;
                    else if (snr <= 180)
                        sqi = 24 * (snr - 160) / 20;
                    else if (snr <= 190)
                        sqi = 24 + 20 * (snr - 180) / 10;
                    else if (snr <= 200)
                        sqi = 44 + 16 * (snr - 190) / 10;
                    else if (snr <= 210)
                        sqi = 60 + 20 * (snr - 200) / 10;
                    else if (snr <= 220)
                        sqi = 80 + 15 * (snr - 210) / 10;
                    else if (snr <= 230)
                        sqi = 95 + 5 * (snr - 220) / 10;
                    else
                        sqi = 100;
                }
                else if (STB_TuneGetActualTerrHpCodeRate(path) == TUNE_TCODERATE_2_3)
                {
                    if (snr <= 170)
                        sqi = 0;
                    else if (snr <= 180)
                        sqi = 3 * (snr - 170) / 10;
                    else if (snr <= 230)
                        sqi = 3 + 85 * (snr - 180) / 50;
                    else if (snr <= 240)
                        sqi = 88 + 12 * (snr - 230) / 10;
                    else
                        sqi = 100;
                }
                else//3/4,as default
                {
                    if (snr <= 190)
                        sqi = 0;
                    else if (snr <= 200)
                        sqi = 1 * (snr - 190) / 10;
                    else if (snr <= 250)
                        sqi = 1 + 85 * (snr - 200) / 50;
                    else if (snr <= 260)
                        sqi = 86 + 14 * (snr - 250) / 10;
                    else
                        sqi = 100;
                }
            }
            else if (STB_TuneGetActualTerrHpCodeRate(path) == TUNE_TCODERATE_3_4)
            {
                if (snr <= 160)
                    sqi = 0;
                else if (snr <= 170)
                    sqi = 10 * (snr - 160) / 10;
                else if (snr <= 180)
                    sqi = 10 + 12 * (snr - 170) / 10;
                else if (snr <= 220)
                    sqi = 22 + 64 * (snr - 180) / 40;
                else if (snr <= 230)
                    sqi = 86 + 8 * (snr - 220) / 10;
                else if (snr <= 240)
                    sqi = 94 + 6 * (snr - 230) / 10;
                else
                    sqi = 100;
            }
            else
            {
                if (snr <= 160)
                    sqi = 0;
                else if (snr <= 170)
                    sqi = 31 * (snr - 160) / 10;
                else if (snr <= 180)
                    sqi = 31 + 18 * (snr - 170) / 10;
                else if (snr <= 190)
                    sqi = 49 + 16 * (snr - 180) / 10;
                else if (snr <= 200)
                    sqi = 65 + 15 * (snr - 190) / 10;
                else if (snr <= 210)
                    sqi = 80 + 12 * (snr - 200) / 10;
                else if (snr <= 220)
                    sqi = 92 + 8 * (snr - 210) / 10;
                else
                    sqi = 100;
            }
            break;

        case TUNE_SIGNAL_QAM:
            if (snr <= 220)
                sqi = 0;
            else if (snr >= 320)
                sqi = 100;
            else
                sqi = snr - 220;
            break;

        case TUNE_SIGNAL_QPSK:
            if (STB_TuneGetSystemType(path) == TUNE_SYSTEM_TYPE_DVBS2)
            {
                if (snr <= 75)
                    sqi = 0;
                else if (snr <= 85)
                    sqi = 15 * (snr - 75) / 10;
                else if (snr <= 105)
                    sqi = 15 + 20 * (snr - 85) / 20;
                else if (snr <= 115)
                    sqi = 35 + 17 * (snr - 105) / 10;
                else if (snr <= 125)
                    sqi = 52 + 27 * (snr - 115) / 10;
                else if (snr <= 135)
                    sqi = 79 + 6 * (snr - 125) / 10;
                else if (snr <= 145)
                    sqi = 85 + 15 * (snr - 135) / 10;
                else
                    sqi = 100;
            }
            else
            {
                if (snr <= 55)
                    sqi = 0;
                else if (snr <= 65)
                    sqi = 15 * (snr - 55) / 10;
                else if (snr <= 75)
                    sqi = 15 + 10 * (snr - 65) / 10;
                else if (snr <= 85)
                    sqi = 25 + 25 * (snr - 75) / 10;
                else if (snr <= 105)
                    sqi = 50 + 40 * (snr - 85) / 20;
                else if (snr <= 125)
                    sqi = 90 + 10 * (snr - 105) / 20;
                else
                    sqi = 100;
            }
            break;

        case TUNE_SIGNAL_ISDBT:

            if (snr <= 160)
                sqi = 0;
            else if (snr <= 170)
                sqi = 31 * (snr - 160) / 10;
            else if (snr <= 180)
                sqi = 31 + 18 * (snr - 170) / 10;
            else if (snr <= 190)
                sqi = 49 + 16 * (snr - 180) / 10;
            else if (snr <= 200)
                sqi = 65 + 15 * (snr - 190) / 10;
            else if (snr <= 210)
                sqi = 80 + 12 * (snr - 200) / 10;
            else if (snr <= 220)
                sqi = 92 + 8 * (snr - 210) / 10;
            else
                sqi = 100;

            break;

        default:
            break;
    }

    return (U8BIT)sqi;
}

/**
 * @brief   Returns the current signal quality
 * @param   path the tuner path to query
 * @return  the signal quality
 * @todo     Confirm DVB API BER units
 */
U8BIT STB_TuneGetSignalQuality(U8BIT path)
{
    U8BIT retval = 0;
    S16BIT quality;

    FUNCTION_START(STB_TuneGetSignalQuality);

    #ifdef EMUTUNNER_ENABLE
    retval = EmuTunerGetSignalQuality(path);
    if (retval > 0)
    {
        return retval;
    }
    #endif

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        if (IsTunerLocked(&tuner_status[path]))
        {
            retval = STB_TuneReadSignalQuality(path);
        }
    }

    FUNCTION_FINISH(STB_TuneGetSignalQuality);

    return retval;
}

U8BIT STB_TuneReadSignalQuality(U8BIT path)
{
    U8BIT retval = 0;
    S16BIT quality;

    FUNCTION_START(STB_TuneReadSignalQuality);

    if (path < num_paths && tuner_status[path].frontend_fd != INVALID_FD)
    {
        if (ioctl(tuner_status[path].frontend_fd, FE_READ_SNR, (U16BIT *)&quality) >= 0)
        {
            if (IsDataConversionNeeded(path))
            {
                retval = SNR10ToSQI(path, quality);
            }
            else
            {
                retval = quality;
            }

            TUN_DBG("%u: Quality=%u%%(snr=%d.%d)", path, retval, quality / 10, quality % 10);
        }
        else
        {
            TUN_ERR("%u: FE_READ_SNR failed, errno %d", path, errno);
        }
    }

    FUNCTION_FINISH(STB_TuneReadSignalQuality);

    return retval;
}

/**
 * @brief   Returns the current signal SNR
 * @param   path the tuner path to query
 * @return  the signal quality
 * @todo    Confirm DVB API BER units
 */
U16BIT STB_TuneGetSignalSNR(U8BIT path)
{
    U16BIT retval = 0;
    uint16_t quality;

    FUNCTION_START(STB_TuneGetSignalSNR);

#ifdef EMUTUNNER_ENABLE
    retval = EmuTunerGetSignalQuality(path);
    if (retval > 0)
    {
        return retval;
    }
#endif

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        if (IsTunerLocked(&tuner_status[path]))
        {
            if (ioctl(tuner_status[path].frontend_fd, FE_READ_SNR, &quality) >= 0)
            {
                retval = (U16BIT)quality;
                TUN_DBG("%u: snr=%u", path, retval);
            }
            else
            {
                TUN_ERR("%u: FE_READ_SNR failed, errno %d", path, errno);
            }
        }
    }

    FUNCTION_FINISH(STB_TuneGetSignalSNR);

    return retval;
}

/**
 * @brief   Returns the actual frequency of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the frequency in Hz
 */
U32BIT STB_TuneGetActualTerrFrequency(U8BIT path)
{
    U32BIT freq;
    struct dtv_property cmd;
    struct dtv_properties props;

    FUNCTION_START(STB_TuneGetActualTerrFrequency);

    freq = 0;

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        if (IsTunerLocked(&tuner_status[path]))
        {
            memset(&cmd, 0, sizeof(struct dtv_property));

            cmd.cmd = DTV_FREQUENCY;
            props.num = 1;
            props.props = &cmd;

            if (ioctl(tuner_status[path].frontend_fd, FE_GET_PROPERTY, &props) >= 0)
            {
                freq = cmd.u.data;
                TUN_DBG("%u: freq=%lu", path, freq);
            }
            else
            {
                TUN_ERR("%u: Failed to read frequency, errno %d", path, errno);
            }
        }
        else
        {
            freq = tuner_status[path].freq;
        }
    }

    FUNCTION_FINISH(STB_TuneGetActualTerrFrequency);

    return(freq);
}

/**
 * @brief   Returns the actual freq offset of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the frequency offset in Hz
 */
S8BIT STB_TuneGetActualTerrFreqOffset(U8BIT path)
{
    S8BIT offset;
    struct dtv_property cmd;
    struct dtv_properties props;

    FUNCTION_START(STB_TuneGetActualTerrFreqOffset);

    offset = 0;

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        if (IsTunerLocked(&tuner_status[path]))
        {
            memset(&cmd, 0, sizeof(struct dtv_property));

            cmd.cmd = DTV_FREQUENCY;
            props.num = 1;
            props.props = &cmd;

            if (ioctl(tuner_status[path].frontend_fd, FE_GET_PROPERTY, &props) >= 0)
            {
                offset = cmd.u.data - tuner_status[path].freq;

                TUN_DBG("%u: freq=%lu, actual=%lu, offset=%d", path,
                        tuner_status[path].freq, cmd.u.data, offset);
            }
            else
            {
                TUN_ERR("%u: Failed to read frequency, errno %d", path, errno);
            }
        }
    }

    FUNCTION_FINISH(STB_TuneGetActualTerrFreqOffset);

    return(offset);
}

/**
 * @brief   Returns the actual mode of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the tuning mode
 */
E_STB_TUNE_TMODE STB_TuneGetActualTerrMode(U8BIT path)
{
    E_STB_TUNE_TMODE mode;
    struct dtv_property cmd;
    struct dtv_properties props;

    FUNCTION_START(STB_TuneGetActualTerrMode);

    mode = TUNE_MODE_COFDM_UNDEFINED;

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        if (IsTunerLocked(&tuner_status[path]))
        {
            memset(&cmd, 0, sizeof(struct dtv_property));

            cmd.cmd = DTV_TRANSMISSION_MODE;
            props.num = 1;
            props.props = &cmd;

            if (ioctl(tuner_status[path].frontend_fd, FE_GET_PROPERTY, &props) >= 0)
            {
                TUN_DBG("%u: mode=%lu", path, cmd.u.data);

                switch (cmd.u.data)
                {
                    case TRANSMISSION_MODE_2K:
                        mode = TUNE_MODE_COFDM_2K;
                        break;

                    case TRANSMISSION_MODE_8K:
                        mode = TUNE_MODE_COFDM_8K;
                        break;

                    case TRANSMISSION_MODE_4K:
                        mode = TUNE_MODE_COFDM_4K;
                        break;

                    case TRANSMISSION_MODE_1K:
                        mode = TUNE_MODE_COFDM_1K;
                        break;

                    case TRANSMISSION_MODE_16K:
                        mode = TUNE_MODE_COFDM_16K;
                        break;

                    case TRANSMISSION_MODE_32K:
                        mode = TUNE_MODE_COFDM_32K;
                        break;

                    case TRANSMISSION_MODE_AUTO:
                    default:
                        break;
                }
            }
            else
            {
                TUN_ERR("%u: Failed to read frequency, errno %d", path, errno);
            }
        }
    }

    FUNCTION_FINISH(STB_TuneGetActualTerrMode);

    return(mode);
}

/**
 * @brief   Returns the actual bandwidth of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the signal bandwidth
 */
E_STB_TUNE_TBWIDTH STB_TuneGetActualTerrBwidth(U8BIT path)
{
    FUNCTION_START(STB_TuneGetActualTerrBwidth);
    E_STB_TUNE_TBWIDTH bwidth = tuner_status[path].u.terr.tbwidth;
    FUNCTION_FINISH(STB_TuneGetActualTerrBwidth);
    return bwidth;
}

/**
 * @brief   Returns the constellation of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the constellation
 */
E_STB_TUNE_TCONST STB_TuneGetActualTerrConstellation(U8BIT path)
{
    struct dtv_property cmd;
    struct dtv_properties props;
    E_STB_TUNE_TCONST t_modu = TUNE_TCONST_UNDEFINED;

    FUNCTION_START(STB_TuneGetActualTerrConstellation);

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        cmd.cmd = DTV_DELIVERY_SYSTEM;
        props.num = 1;
        props.props = &cmd;
        if (ioctl(tuner_status[path].frontend_fd, FE_GET_PROPERTY, &props) >= 0)
        {
            if (cmd.u.data == SYS_DVBT2 || cmd.u.data == SYS_DVBT)
            {
                switch (cmd.reserved[0])
                {
                    case QPSK:
                    t_modu = TUNE_TCONST_QPSK;
                    break;

                    case QAM_16:
                    t_modu = TUNE_TCONST_QAM16;
                    break;

                    case QAM_64:
                    t_modu = TUNE_TCONST_QAM64;
                    break;

                    case QAM_256:
                    t_modu = TUNE_TCONST_QAM256;
                    break;

                    default:
                    t_modu = TUNE_TCONST_UNDEFINED;
                    break;
                }
            }
        }
        else
        {
            TUN_ERR("%u: Failed to get property, errno %d", path, errno);
        }
    }

    FUNCTION_FINISH(STB_TuneGetActualTerrConstellation);

    return t_modu;
}

/**
 * @brief   Returns the heirarchy of the current terrestrial signal
 * @param   tuner_id, the tuner index to query
 * @return  the heirarchy
 */
E_STB_TUNE_THIERARCHY STB_TuneGetActualTerrHierarchy(U8BIT tuner_id)
{
    U8BIT retval = TUNE_THIERARCHY_NONE;
    struct dtv_property cmd;
    struct dtv_properties props;
    uint8_t plp_ids[256];

    FUNCTION_START(STB_TuneGetActualTerrHierarchy);

    if (tuner_status[tuner_id].sys_type == TUNE_SYSTEM_TYPE_DVBT2)
    {
        retval = STB_TuneGetMPLPIDList(tuner_id,plp_ids,MAX_PLP_NUMBER);
        if (retval != 0)
        {
            retval--;
        }
    }
    else
    {
        if (tuner_id < num_paths && tuner_status[tuner_id].sys_type == TUNE_SYSTEM_TYPE_DVBT2)
        {
            if (tuner_status[tuner_id].frontend_fd != INVALID_FD &&
                GetTunerLockStatus(tuner_status[tuner_id].frontend_fd) == TUNER_STATE_LOCKED)
            {
                cmd.cmd = DTV_HIERARCHY;
                props.num = 1;
                props.props = &cmd;

                if (ioctl(tuner_status[tuner_id].frontend_fd, FE_GET_PROPERTY, &props) >= 0)
                {
                    TUN_DBG("%u: hierarchy=%lu", tuner_id, cmd.u.data);

                    switch (cmd.u.data)
                    {
                        case HIERARCHY_NONE:
                            retval = TUNE_THIERARCHY_NONE;
                            break;

                        case HIERARCHY_1:
                            retval = TUNE_THIERARCHY_1;
                            break;

                        case HIERARCHY_2:
                            retval = TUNE_THIERARCHY_2;
                            break;

                        case HIERARCHY_4:
                            retval = TUNE_THIERARCHY_4;
                            break;

                        default:
                            retval = TUNE_THIERARCHY_NONE;
                            break;
                    }
                }
                else
                {
                    TUN_ERR("%u: Failed to get hierarchy, errno %d", tuner_id, errno);
                }
            }
        }
    }

    FUNCTION_FINISH(STB_TuneGetActualTerrHierarchy);

    return retval;
}


/**
 * @brief   Returns the heirarchy of the current terrestrial signal.
 * @param   tuner_id, in  param,the tuner index to query
 * @param   plp_list, out param, to store all the pip id in the current freq
 * @param   listlen,  in  param, the max numbers of pipid that can be stored in the list
 * @return  the max pip number of the current frequency.
 */
S32BIT STB_TuneGetMPLPIDList(U8BIT tuner_id, U8BIT *plp_list, U16BIT listlen)
{
    S32BIT retval = 0;
    struct dtv_property cmd;
    struct dtv_properties props;
    uint8_t plp_ids[MAX_PLP_NUMBER];
    U32BIT start_time;
    static const U32BIT timeout_dvbt2 = 5000;

    FUNCTION_START(STB_TuneGetMPLPIDList);

    if (tuner_id < num_paths && tuner_status[tuner_id].sys_type == TUNE_SYSTEM_TYPE_DVBT2)
    {
        if (tuner_status[tuner_id].frontend_fd != INVALID_FD)
        {
            start_time = STB_OSGetClockMilliseconds();
            while (STB_OSGetClockDiff(start_time) <= timeout_dvbt2)
            {
                if (GetTunerLockStatus(tuner_status[tuner_id].frontend_fd) == TUNER_STATE_LOCKED)
                {
                    memset(&cmd, 0, sizeof(struct dtv_property));
                    cmd.cmd = DTV_DVBT2_PLP_ID;
                    cmd.u.buffer.reserved1[1] = MAX_PLP_NUMBER;
                    cmd.u.buffer.reserved2 = plp_ids;

                    props.num = 1;
                    props.props = &cmd;

                    if (ioctl(tuner_status[tuner_id].frontend_fd, FE_GET_PROPERTY, &props) >= 0)
                    {
                        retval = cmd.u.buffer.reserved1[0];

                        if (retval != 0)
                        {
                            if (listlen >= retval)
                            {
                                memcpy(plp_list, plp_ids, retval);
                            }
                            else
                            {
                                memcpy(plp_list, plp_ids, listlen);
                                TUN_ERR("%u: listlen:%d not enough, retval:%d ", tuner_id, listlen, retval);
                            }
                        }

                        TUN_DBG("%u: Num PLPs=%u", tuner_id, retval);
                    }
                    else
                    {
                        TUN_ERR("%u: Failed to get number of PLPs, errno %d", tuner_id, errno);
                        retval = 0;
                    }
                    break;
                }
                else
                {
                    STB_OSTaskDelay(100);
                }
            }
        }
    }
    else
    {
        TUN_ERR("%u: Not MPLP , errno %d", tuner_id, errno);
    }

    FUNCTION_FINISH(STB_TuneGetMPLPIDList);

    return retval;
}

static E_STB_TUNE_TCODERATE TuneGetActualTerrCodeRate(U8BIT path)
{
	struct dtv_property cmd;
    struct dtv_properties props;
    E_STB_TUNE_TCODERATE t_rc = TUNE_TCODERATE_UNDEFINED;

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        cmd.cmd = DTV_DELIVERY_SYSTEM;
        props.num = 1;
        props.props = &cmd;
        if (ioctl(tuner_status[path].frontend_fd, FE_GET_PROPERTY, &props) >= 0)
        {
            if (cmd.u.data == SYS_DVBT2 || cmd.u.data == SYS_DVBT)
            {
                switch (cmd.reserved[1])
                {
                    case FEC_1_2:
                    t_rc = TUNE_TCODERATE_1_2;
                    break;

                    case FEC_3_5:
                    t_rc = TUNE_TCODERATE_3_5;
                    break;

                    case FEC_2_3:
                    t_rc = TUNE_TCODERATE_2_3;
                    break;

                    case FEC_3_4:
                    t_rc = TUNE_TCODERATE_3_4;
                    break;

                    case FEC_4_5:
                    t_rc = TUNE_TCODERATE_4_5;
                    break;

                    case FEC_5_6:
                    t_rc = TUNE_TCODERATE_5_6;
                    break;

                    default:
                    t_rc = TUNE_TCODERATE_UNDEFINED;
                    break;
                }
            }
        }
        else
        {
            TUN_ERR("%u: Failed to get property, errno %d", path, errno);
        }
    }

    return t_rc;
}


/**
 * @brief   Returns the LP code rate of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  The LP code rate
 */
E_STB_TUNE_TCODERATE STB_TuneGetActualTerrLpCodeRate(U8BIT path)
{
    E_STB_TUNE_TCODERATE t_rc;

    FUNCTION_START(STB_TuneGetActualTerrLpCodeRate);

    t_rc = TuneGetActualTerrCodeRate(path);

    FUNCTION_FINISH(STB_TuneGetActualTerrLpCodeRate);

    return t_rc;
}

/**
 * @brief   Returns the HP code rate of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  The HP code rate
 */
E_STB_TUNE_TCODERATE STB_TuneGetActualTerrHpCodeRate(U8BIT path)
{
    E_STB_TUNE_TCODERATE t_rc;

    FUNCTION_START(STB_TuneGetActualTerrHpCodeRate);

    t_rc = TuneGetActualTerrCodeRate(path);

    FUNCTION_FINISH(STB_TuneGetActualTerrHpCodeRate);

    return t_rc;
}

/**
 * @brief   Returns the guard interval of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the guard interval
 */
E_STB_TUNE_TGUARDINT STB_TuneGetActualTerrGuardInt(U8BIT path)
{
    FUNCTION_START(STB_TuneGetActualTerrGuardInt);
    USE_UNWANTED_PARAM(path);
    FUNCTION_FINISH(STB_TuneGetActualTerrGuardInt);
    return(TUNE_TGUARDINT_UNDEFINED);
}

/**
 * @brief   Returns the cell id the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the cell id
 */
U16BIT STB_TuneGetActualTerrCellId(U8BIT path)
{
    struct dtv_property cmd;
    struct dtv_properties props;
    U16BIT cell_id = 0xFFFF;

    FUNCTION_START(STB_TuneGetActualTerrCellId);

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        cmd.cmd = DTV_DELIVERY_SYSTEM;
        props.num = 1;
        props.props = &cmd;
        if (ioctl(tuner_status[path].frontend_fd, FE_GET_PROPERTY, &props) >= 0)
        {
            if (cmd.u.data == SYS_DVBT2 || cmd.u.data == SYS_DVBT)
            {
                cell_id = cmd.reserved[2];
                TUN_DBG("%u: cell_id=0x%04X", path, cell_id);
            }
        }
        else
        {
            TUN_ERR("%u: Failed to get property, errno %d", path, errno);
        }
    }

    FUNCTION_FINISH(STB_TuneGetActualTerrCellId);
    return cell_id;
}

/**
 * @brief   Returns the actual bandwidth of the current isdbt signal
 * @param   path the tuner path to query
 * @return  the signal bandwidth
 */
E_STB_TUNE_TBWIDTH STB_TuneGetActualIsdbtBwidth(U8BIT path)
{
    FUNCTION_START(STB_TuneGetActualIsdbtBwidth);
    E_STB_TUNE_TBWIDTH bwidth = tuner_status[path].u.isdbt.tbwidth;
    FUNCTION_FINISH(STB_TuneGetActualIsdbtBwidth);
    return bwidth;
}

/**
 * @brief   Enables/disables aerial power for DVB-T
 * @param   path tuner path
 * @param   enabled TRUE to enable
 */
void STB_TuneActiveAerialPower(U8BIT path, BOOLEAN enabled)
{
    FUNCTION_START(STB_TuneActiveAerialPower);
    USE_UNWANTED_PARAM(path);
    USE_UNWANTED_PARAM(enabled);
    FUNCTION_FINISH(STB_TuneActiveAerialPower);
}

E_STB_TUNE_LNB_VOLTAGE STB_TuneGetLNBVoltage(U8BIT path)
{
    E_STB_TUNE_LNB_VOLTAGE voltage = LNB_VOLTAGE_OFF;

    FUNCTION_START(STB_TuneGetLNBVoltage);

    if ((path < num_paths) && (tuner_status[path].signal_type == TUNE_SIGNAL_QPSK))
    {
        voltage = tuner_status[path].u.sat.lnb_voltage;
    }

    FUNCTION_FINISH(STB_TuneGetLNBVoltage);

    return voltage;
}

/**
 * @brief   Sets the LNB voltage for the given tuner
 * @param   path tuner path
 * @param   voltage voltage setting
 */
void STB_TuneSetLNBVoltage(U8BIT path, E_STB_TUNE_LNB_VOLTAGE voltage, BOOLEAN retune)
{
    FUNCTION_START(STB_TuneSetLNBVoltage);

    if ((path < num_paths) && (tuner_status[path].signal_type == TUNE_SIGNAL_QPSK))
    {
        if (tuner_status[path].u.sat.lnb_voltage != voltage)
        {
            tuner_status[path].u.sat.lnb_voltage = voltage;
            if (retune)
            {
                tuner_status[path].tuning_params_changed = TRUE;
            }
        }

        STB_TuneSetVoltageInterface(path, voltage);
    }

    FUNCTION_FINISH(STB_TuneSetLNBVoltage);
}

void STB_TuneSetFrontendFd(U8BIT path, U32BIT fe_fd)
{
    tuner_status[path].frontend_fd = fe_fd;
    TUN_DBG("path:%d fd:%d", path, tuner_status[path].frontend_fd);
}

E_STB_TUNE_SYSTEM_TYPE STB_TuneGetActualSysType(U8BIT path)
{
    E_STB_TUNE_SYSTEM_TYPE sys_type = TUNE_SYSTEM_TYPE_UNKNOWN;
    struct dtv_property p = {.cmd = DTV_DELIVERY_SYSTEM, .u.data = 0};
    struct dtv_properties props = {.num = 1, .props = &p};

    if (ioctl(tuner_status[path].frontend_fd, FE_GET_PROPERTY, &props) != -1)
    {
        if (p.u.data == SYS_DVBT)
        {
            sys_type = TUNE_SYSTEM_TYPE_DVBT;
        }
        else if (p.u.data == SYS_DVBT2)
        {
            sys_type = TUNE_SYSTEM_TYPE_DVBT2;
        }
        else if (p.u.data == SYS_DVBS)
        {
            sys_type = TUNE_SYSTEM_TYPE_DVBS;
        }
        else if (p.u.data == SYS_DVBS2)
        {
            sys_type = TUNE_SYSTEM_TYPE_DVBS2;
        }
        // loop more
    }

    TUN_DBG("path:%d fd:%d sys_type:%d", path, tuner_status[path].frontend_fd, sys_type);

    return sys_type;
}


void STB_TuneSetVoltageInterface(U8BIT path, E_STB_TUNE_LNB_VOLTAGE vol)
{
    FUNCTION_START(STB_TuneSetVoltageInterface);
    fe_sec_voltage_t voltage;

    switch (vol)
    {
        case LNB_VOLTAGE_14V:
            voltage = SEC_VOLTAGE_13;
            break;

        case LNB_VOLTAGE_18V:
            voltage = SEC_VOLTAGE_18;
            break;

        case LNB_VOLTAGE_OFF:
        default:
            voltage = SEC_VOLTAGE_OFF;
            break;
    }

    TUN_DBG("path:%d fd:%d voltage:%d", path, tuner_status[path].frontend_fd, voltage);
    if (ioctl(tuner_status[path].frontend_fd, FE_SET_VOLTAGE, voltage) < 0)
    {
        TUN_DBG("ioctl FE_SET_VOLTAGE failed, path:%d fd:%d error:%d", path, tuner_status[path].frontend_fd, errno);
    }

    FUNCTION_FINISH(STB_TuneSetVoltageInterface);
}


/**
 * @brief   Sets the type of modulation for the specified tuner
 * @param   path tuner path
 * @param   modulation type of modulation
 */
void STB_TuneSetModulation(U8BIT path, E_STB_TUNE_MODULATION modulation)
{
    FUNCTION_START(STB_TuneSetModulation);

    if ((path < num_paths) && (tuner_status[path].signal_type == TUNE_SIGNAL_QPSK))
    {
        tuner_status[path].u.sat.modulation = modulation;
    }

    FUNCTION_FINISH(STB_TuneSetModulation);
}

BOOLEAN STB_TuneGet22kState(U8BIT path)
{
    BOOLEAN state = FALSE;

    FUNCTION_START(STB_TuneGet22kState);

    if ((path < num_paths) && (tuner_status[path].signal_type == TUNE_SIGNAL_QPSK))
    {
        state = tuner_status[path].u.sat.use_22khz;
    }

    FUNCTION_FINISH(STB_TuneGet22kState);

    return state;
}

/**
 * @brief   Turns the 22 kHz tone on or off
 * @param   path tuner path
 * @param   state TRUE to turn the tone on, FALSE to turn it off
 */
void STB_TuneSet22kState(U8BIT path, BOOLEAN state, BOOLEAN retune)
{
    FUNCTION_START(STB_TuneSet22kState);

    if ((path < num_paths) && (tuner_status[path].signal_type == TUNE_SIGNAL_QPSK))
    {
        if (tuner_status[path].u.sat.use_22khz != state)
        {
            tuner_status[path].u.sat.use_22khz = state;
            if (retune)
            {
                tuner_status[path].tuning_params_changed = TRUE;
            }
        }

        STB_TuneSetTone(path, state);
    }

    FUNCTION_FINISH(STB_TuneSet22kState);
}

/**
 * @brief   Sets the 12V switch for the given tuner
 * @param   path tuner path
 * @param   state TRUE for on
 */
void STB_TuneSet12VSwitch(U8BIT path, BOOLEAN state)
{
    FUNCTION_START(STB_TuneSet12VSwitch);
    USE_UNWANTED_PARAM(path);
    USE_UNWANTED_PARAM(state);
    FUNCTION_FINISH(STB_TuneSet12VSwitch);
}

/**
 * @brief   Sends the DisEqc message
 * @param   path - tuner path
 * @param   data - message data
 * @param   size - number of bytes in message data
 */
void STB_TuneSendDISEQCMessage(U8BIT path, U8BIT *data, U8BIT size)
{
    FUNCTION_START(STB_TuneSendDISEQCMessage);
    struct dvb_diseqc_master_cmd cmd;
    memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

    for (U8BIT i = 0; i < size; i++)
    {
        cmd.msg[i] = data[i];
        TUN_DBG("cmd:0x%02x", data[i]);
    }

    cmd.msg_len = size;

    if (ioctl(tuner_status[path].frontend_fd, FE_DISEQC_SEND_MASTER_CMD, &cmd) < 0)
    {
        TUN_DBG("ioctl FE_DISEQC_SEND_MASTER_CMD failed, path:%d error:%d", path, errno);
    }

    FUNCTION_FINISH(STB_TuneSendDISEQCMessage);
}

/**
 * @brief   Receives the DisEqc reply
 * @param   path - tuner path
 * @param   data - message data
 * @param   size - number of bytes in message data
 * @param   timneout - ioctl timeout
 */
void STB_TuneReceiveDISEQCReply(U8BIT path, U8BIT *data, U8BIT size, U32BIT timeout)
{
    FUNCTION_START(STB_TuneReceiveDISEQCReply);
    struct dvb_diseqc_slave_reply reply;
    memset(&reply, 0, sizeof(struct dvb_diseqc_slave_reply));

    reply.timeout = (int)timeout;
    if (ioctl(tuner_status[path].frontend_fd, FE_DISEQC_RECV_SLAVE_REPLY, &reply) < 0)
    {
        TUN_DBG("ioctl FE_DISEQC_RECV_SLAVE_REPLY failed, path:%d error:%d", path, errno);
    }
    else
    {
        if (data == NULL || size < reply.msg_len)
        {
            TUN_DBG("ioctl FE_DISEQC_RECV_SLAVE_REPLY failed, data is incorrect");
        }
        else
        {
            for (U8BIT i = 0; i < reply.msg_len; i++)
            {
                data[i] = reply.msg[i];
                TUN_DBG("reply:0x%02x", data[i]);
            }
        }
    }

    FUNCTION_FINISH(STB_TuneReceiveDISEQCReply);
}

/**
 * @brief   Sends the Burst message
 * @param   path - tuner path
 * @param   data - message data
 */
void STB_TuneSendBurstMessage(U8BIT path, U8BIT data)
{
    FUNCTION_START(STB_TuneSendBurstMessage);
    fe_sec_mini_cmd_t cmd;

    TUN_DBG("cmd:0x%x", data);

    if (data == 0x00 || data == 0xFF)
    {
        if (data == 0x00)
        {
            cmd = SEC_MINI_A;
        }
        else
        {
            cmd = SEC_MINI_B;
        }

        if (ioctl(tuner_status[path].frontend_fd, FE_DISEQC_SEND_BURST, cmd) < 0)
        {
            TUN_DBG("ioctl FE_DISEQC_SEND_BURST failed, path:%d fd:%d error:%d",
                    path, tuner_status[path].frontend_fd, errno);
        }
    }

    FUNCTION_FINISH(STB_TuneSendBurstMessage);
}

/**
 * @brief   Sets the pulse limit for the east
 * @param   path tuner path
 * @param   count east limit count
 */
void STB_TuneSetPulseLimitEast(U8BIT path, U16BIT count)
{
    FUNCTION_START(STB_TuneSetPulseLimitEast);
    // count, support for drive owner
    U8BIT dmsg_data[3];

    dmsg_data[0] = 0xE0;
    dmsg_data[1] = 0x31;
    dmsg_data[2] = 0x66;

    STB_TuneSendDISEQCMessage(path, dmsg_data, 3);

    FUNCTION_FINISH(STB_TuneSetPulseLimitEast);
}

/**
 * @brief   Sets the pulse limit for the west
 * @param   path tuner path
 * @param   count west limit count
 */
void STB_TuneSetPulseLimitWest(U8BIT path, U16BIT count)
{
    FUNCTION_START(STB_TuneSetPulseLimitWest);
    // count, support for drive owner
    U8BIT dmsg_data[3];

    dmsg_data[0] = 0xE0;
    dmsg_data[1] = 0x31;
    dmsg_data[2] = 0x67;

    STB_TuneSendDISEQCMessage(path, dmsg_data, 3);

    FUNCTION_FINISH(STB_TuneSetPulseLimitWest);
}

void STB_TuneChangePulsePosition(U8BIT path, U16BIT count)
{
    FUNCTION_START(STB_TuneChangePulsePosition);
    USE_UNWANTED_PARAM(path);
    //do nothing, now
    FUNCTION_FINISH(STB_TuneChangePulsePosition);
}

/**
 * @brief   Returns the current pulse position
 * @param   path tuner path
 * @return  Current puls position
 */
U16BIT STB_TuneGetPulsePosition(U8BIT path)
{
    FUNCTION_START(STB_TuneGetPulsePosition);
    USE_UNWANTED_PARAM(path);
    //do nothing, now
    FUNCTION_FINISH(STB_TuneGetPulsePosition);

    return(0);
}

void STB_TuneAtPulsePosition(U8BIT path, U16BIT position)
{
    FUNCTION_START(STB_TuneAtPulsePosition);
    USE_UNWANTED_PARAM(path);
    USE_UNWANTED_PARAM(position);
    FUNCTION_FINISH(STB_TuneAtPulsePosition);
}

/**
 * @brief Changes the value of skew position count
 * @param path tuner path
 * @param count skew position count
 */
void STB_TuneChangeSkewPosition(U8BIT path, U16BIT count)
{
    FUNCTION_START(STB_TuneChangeSkewPosition);
    USE_UNWANTED_PARAM(path);
    USE_UNWANTED_PARAM(count);
    FUNCTION_FINISH(STB_TuneChangeSkewPosition);
}

/**
 * @brief   Sets the local oscillator frequency used by the LNB
 * @param   path the tuner path to query
 */
void STB_TuneSetLOFrequency(U8BIT path, U16BIT lo_freq)
{
    FUNCTION_START(STB_TuneSetLOFrequency);

    if (path < num_paths)
    {
        tuner_status[path].u.sat.lo_freq = lo_freq;
    }

    FUNCTION_FINISH(STB_TuneSetLOFrequency);
}

/**
 * @brief   Returns the carrier signal strength as a percentage
 * @param   path tuner path
 * @param   freq carrier frequency
 * @return  Strength as a percentage
 */
U8BIT STB_TuneSatGetCarrierStrength(U8BIT path, U32BIT freq)
{
    FUNCTION_START(STB_TuneSatGetCarrierStrength);
    USE_UNWANTED_PARAM(path);
    USE_UNWANTED_PARAM(freq);
    FUNCTION_FINISH(STB_TuneSatGetCarrierStrength);
    return(0);
}

/**
 * @brief   Set the demodulator's signal type. This function must be called
 *          before each call to STB_TuneStartTuner in a dvb-t2 system and
 *          never in a dvb-t system.
 * @param   U8BIT path - the tuner path to set up
 * @param   E_STB_TUNE_TERR_TYPE type: TUNE_TERR_TYPE_DVBT,
 *          TUNE_TERR_TYPE_DVBT2 or TUNE_TERR_TYPE_UNKNOWN. When the signal
 *          type has been set to TUNE_TERR_TYPE_UNKNOWN, a call to
 *          STB_TuneStartTuner will force the driver to try with DVB-T first,
 *          and if no signal is found, with DVB-T2. When a signal has been
 *          found, STB_TuneGetTerrType will return the actual signal type.
 */
void STB_TuneSetSystemType(U8BIT path, E_STB_TUNE_SYSTEM_TYPE type)
{
    FUNCTION_START(STB_TuneSetSystemType);

    if (path < num_paths)
    {
        tuner_status[path].sys_type = type;
    }

    FUNCTION_FINISH(STB_TuneSetTerrType);
}

/**
 * @brief   Returns the signal type as set by STB_TuneSetTerrType or as
 *          re-written by the driver.
 * @param   path the tuner path to query
 * @return  Signal type.
 */
E_STB_TUNE_SYSTEM_TYPE STB_TuneGetSystemType(U8BIT path)
{
    E_STB_TUNE_SYSTEM_TYPE type;

    FUNCTION_START(STB_TuneGetSystemType);

    if (path < num_paths)
    {
        type = tuner_status[path].sys_type;
    }
    else
    {
        type = TUNE_SYSTEM_TYPE_UNKNOWN;
    }

    FUNCTION_FINISH(STB_TuneGetSystemType);

    return(type);
}

/**
 * @brief   Returns the type of modulation for the specified tuner
 * @param   path tuner path
 * @return  type of modulation
 */
E_STB_TUNE_MODULATION STB_TuneGetModulation(U8BIT path)
{
    FUNCTION_START(STB_TuneGetModulation);

    if ((path < num_paths) && (tuner_status[path].signal_type == TUNE_SIGNAL_QPSK))
    {
        return tuner_status[path].u.sat.modulation;
    }
    else
    {
        return TUNE_MOD_AUTO;
    }

    FUNCTION_FINISH(STB_TuneGetModulation);
}

/**
 * @brief   Sets the Physical Layer Pipe to be acquired
 * @param   path the tuner path to set up
 * @param   plp Physical Layer Pipe to be acquired
 */
void STB_TuneSetPLP(U8BIT path, U8BIT plp)
{
    FUNCTION_START(STB_TuneSetPLP);

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD) &&
            (tuner_status[path].sys_type == TUNE_SYSTEM_TYPE_DVBT2 || tuner_status[path].sys_type == TUNE_SYSTEM_TYPE_DVBT))
    {
        TUN_DBG("%u: PLP new:old [%u:%u]", path, plp, tuner_status[path].plp_id);

        if (tuner_status[path].plp_id != plp)
        {
            tuner_status[path].plp_id = plp;
            tuner_status[path].tuning_params_changed = TRUE;
        }
    }

    FUNCTION_FINISH(STB_TuneSetPLP);
}

/**
 * @brief   Returns the actual symbol rate when a tuner has locked
 * @param   path tuner path
 * @return  Symbol rate in symbols per second
 */
U32BIT STB_TuneGetActualSymbolRate(U8BIT path)
{
    U32BIT srate = 0;

    FUNCTION_START(STB_TuneGetActualSymbolRate);

    if (GetRealParamFromDriver(path) == TRUE)
    {
        srate = real_srate;
    }
    else
    {
        if (tuner_status[path].signal_type == TUNE_SIGNAL_QAM)
        {
            srate = tuner_status[path].u.cab.srate;
        }
        else if (tuner_status[path].signal_type == TUNE_SIGNAL_QPSK)
        {
            srate = tuner_status[path].u.sat.srate;
        }
    }
    TUN_DBG("%u: symbol rate = %lu", path, srate);

    FUNCTION_FINISH(STB_TuneGetActualSymbolRate);

    return(srate);
}

/**
 * @brief   Returns the cable mode when the tuner has locked
 * @param   path tuner path
 * @return  QAM mode
 */

E_STB_TUNE_CMODE STB_TuneGetActualCableMode(U8BIT path)
{
    E_STB_TUNE_CMODE mode;

    FUNCTION_START(STB_TuneGetActualCableMode);

    mode = TUNE_MODE_QAM_UNDEFINED;

    if ((path < num_paths) && (tuner_status[path].signal_type == TUNE_SIGNAL_QAM))
    {
        mode = tuner_status[path].u.cab.cmode;
    }

    FUNCTION_FINISH(STB_TuneGetActualCableMode);

    return(mode);
}

/**
 * @brief   Returns the system type supported by the path. This function
 *          differs from STB_TuneGetSystemType which only returns T2 or S2 if
 *          the tuner is currently performing T2 or S2 operations.
 * @param   path  the tuner path to query
 * @return  void
 */
void STB_TuneGetSupportedSystemType(U8BIT path, U8BIT *support_sys)
{
//get supported system type from /etc/tvconfig/dtvkit/config.xml
    if (support_sys == NULL)
    {
       TUN_ERR("%u: support_sys is null!", path);
       return;
    }

    if (aml_hw_cfg.tuner_num > path)
    {
       TUN_DBG("%u: signal_types=%lu", path, aml_hw_cfg.tuners[path].signal_types);
       if (aml_hw_cfg.tuners[path].signal_types & TUNE_SIGNAL_QAM)
       {
          support_sys[TUNE_SYSTEM_TYPE_DVBC] = TRUE;
       }

       if (aml_hw_cfg.tuners[path].signal_types & TUNE_SIGNAL_COFDM)
       {
          support_sys[TUNE_SYSTEM_TYPE_DVBT] = TRUE;
          if (aml_hw_cfg.tuners[path].support_dvbt2)
          {
             support_sys[TUNE_SYSTEM_TYPE_DVBT2] = TRUE;
          }
       }

       if (aml_hw_cfg.tuners[path].signal_types & TUNE_SIGNAL_QPSK)
       {
          support_sys[TUNE_SYSTEM_TYPE_DVBS] = TRUE;
          if (aml_hw_cfg.tuners[path].support_dvbs2)
          {
             support_sys[TUNE_SYSTEM_TYPE_DVBS2] = TRUE;
          }
       }
    }
}

BOOLEAN STB_TuneOpen(U8BIT path)
{
    BOOLEAN ret = FALSE, sem_ret = FALSE;
    FUNCTION_START(STB_TuneOpen);

    if (path < num_paths)
    {
        while (STB_TuneIsTvPlatform() && tuner_status[path].state == TUNER_EXITED && !STB_TuneIsSearchMode(path))
        {
            TUN_DBG("%u: tunertask_sem entry sem_wait:%p.",
                    tuner_status[path].path, tuner_status[path].tunertask_sem);
            sem_ret = STB_OSSemaphoreWaitTimeout(tuner_status[path].tunertask_sem, 1000);
            TUN_DBG("%u: tunertask_sem exit sem_timedwait:%p, sem_ret:%d, tstatus->state:%d.",
                    tuner_status[path].path, tuner_status[path].tunertask_sem, sem_ret, tuner_status[path].state);
        }

        pthread_mutex_lock(&tuner_status[path].lock);
        ret = OpenTuner(&tuner_status[path]);
        pthread_mutex_unlock(&tuner_status[path].lock);
    }

    FUNCTION_FINISH(STB_TuneOpen);
    return ret;
}

BOOLEAN STB_TuneIsOpened(U8BIT path)
{
    BOOLEAN ret = FALSE;

    if (path < num_paths)
    {
        ret = tuner_status[path].frontend_fd != INVALID_FD;
    }

    return ret;
}

void STB_TuneUpdateFeUsage(U8BIT path, BOOLEAN use)
{
    FUNCTION_START(STB_TuneUpdateFeUsage);

    if (path < num_paths)
    {
        pthread_mutex_lock(&tuner_status[path].lock);

        if (use)
            tuner_status[path].frontend_usage ++;
        else
            tuner_status[path].frontend_usage --;

        TUN_DBG("%u: fe_useage[%s]: %d", path, use?"Add":"Remove", tuner_status[path].frontend_usage);
        pthread_mutex_unlock(&tuner_status[path].lock);
    }

    FUNCTION_FINISH(STB_TuneUpdateFeUsage);
}

BOOLEAN STB_TuneIsTvPlatform()
{
    return isTvPlatform;
}

void STB_TuneSetSearchMode(U8BIT path, BOOLEAN mode)
{
    if (path < num_paths && STB_TuneIsTvPlatform())
    {
        pthread_mutex_lock(&tuner_status[path].lock);
        STB_OSMutexLock(tuner_status[path].mutex);
        TUN_DBG("tune path[%d] [state: %d].", path, tuner_status[path].state);

        if (tuner_status[path].search_mode != mode)
        {
            if (mode && tuner_status[path].state == TUNER_EXITED)
            {
                tuner_status[path].state = TUNER_IDLE;
            }
            else if (!mode && tuner_status[path].state == TUNER_IDLE)
            {
                /* DTVKit can only be set to the exit state when it exits. */
                /* tuner_status[path].state = TUNER_EXITED; */
            }

            tuner_status[path].search_mode = mode;
            TUN_DBG("tune path[%d] [search_mode: %d].", path, mode);
        }

        STB_OSMutexUnlock(tuner_status[path].mutex);
        pthread_mutex_unlock(&tuner_status[path].lock);
    }
}

BOOLEAN STB_TuneIsSearchMode(U8BIT path)
{
    BOOLEAN search_mode = FALSE;

    if (path < num_paths && STB_TuneIsTvPlatform())
    {
        STB_OSMutexLock(tuner_status[path].mutex);
        search_mode = tuner_status[path].search_mode;
        TUN_DBG("tune path[%d] [search_mode: %d].", path, search_mode);
        STB_OSMutexUnlock(tuner_status[path].mutex);
    }

    return search_mode;
}

void STB_TuneAllStart()
{
    U8BIT i;

    for (i = 0; i != num_paths; i++)
    {
        pthread_mutex_lock(&tuner_status[i].lock);

        //OpenTuner(&tuner_status[i]);
        if (STB_TuneIsTvPlatform() && tuner_status[i].state == TUNER_EXITED)
        {
            STB_OSMutexLock(tuner_status[i].mutex);
            tuner_status[i].state = TUNER_IDLE;
            tuner_status[i].search_mode = FALSE;
            STB_OSMutexUnlock(tuner_status[i].mutex);
            STB_OSSemaphoreSignal(tuner_status[i].tunertask_sem);
        }

        TUN_DBG("tune path[%d] state:%d", i, tuner_status[i].state);
        pthread_mutex_unlock(&tuner_status[i].lock);
    }
}

void STB_TuneAllStop()
{
    U8BIT i = 0;
    E_TUNER_STATE state;

    if (STB_TuneIsTvPlatform())
    {
        if (STB_DPIsAllPathReleased())
        {
            TUN_DBG("STB_DPIsAllPathReleased [TRUE].");
        }

        STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_RESOURCE_BUSY, &i, sizeof(U8BIT));
    }

    for (i = 0; i != num_paths; i++)
    {
        pthread_mutex_lock(&tuner_status[i].lock);

        if (tuner_status[i].frontend_fd != INVALID_FD)
        {
            STB_OSMutexLock(tuner_status[i].mutex);
            state = tuner_status[i].state;
            STB_OSMutexUnlock(tuner_status[i].mutex);

            if (state != TUNER_IDLE && state != TUNER_EXITED)
            {
                STB_TuneStopTuner(i);
            }
        }

        TUN_DBG("tune path[%d] close FE:%d, usage:%d", i, tuner_status[i].frontend_fd, tuner_status[i].frontend_usage);
        CloseTuner(&tuner_status[i]);

        if (STB_TuneIsTvPlatform())
        {
            STB_OSMutexLock(tuner_status[i].mutex);
            tuner_status[i].state = TUNER_EXITED;
            tuner_status[i].search_mode = FALSE;
            STB_OSMutexUnlock(tuner_status[i].mutex);
        }

        pthread_mutex_unlock(&tuner_status[i].lock);
    }
}

static BOOLEAN STB_TuneSetTone(U8BIT path, BOOLEAN use_22khz)
{
    BOOLEAN ret = FALSE;
    fe_sec_tone_mode_t tone;

    if (use_22khz)
    {
        tone = SEC_TONE_ON;
    }
    else
    {
        tone = SEC_TONE_OFF;
    }

    if (ioctl(tuner_status[path].frontend_fd, FE_SET_TONE, tone) >= 0)
        ret = TRUE;

    TUN_DBG("[%s] frontend_fd:%d, use_22khz:%d, ret:%d \n",
            __FUNCTION__, tuner_status[path].frontend_fd, use_22khz, ret);

    return ret;
}

BOOLEAN STB_Tune_BlindScan(U8BIT path, E_STB_TUNE_SYSTEM_TYPE sys_type, STB_Tnue_BlindCallback_t cb, void *user_data,
                                 unsigned int start_freq, unsigned int stop_freq, E_STB_TUNE_BlindUnicable_t unicable)
{
    if (start_freq == stop_freq)
    {
        TUN_DBG( "AM_FEND_BlindScan start_freq equal stop_freq\n");
        return FALSE;
    }

    /*this function set the parameters blind scan process needed.*/
    SetBlindScanFeProperty(tuner_status[path].frontend_fd, sys_type);

    memset(&(tuner_status[path].bs_setting), 0, sizeof(struct DVB_BlindScanAPI_Setting));

    if (sys_type == TUNE_SYSTEM_TYPE_DVBS)
    {
        tuner_status[path].bs_setting.bsPara.minfrequency = M_BS_START_FREQ * 1000;		    /*Default Set Blind scan start frequency*/
        tuner_status[path].bs_setting.bsPara.maxfrequency = M_BS_STOP_FREQ * 1000;		    /*Default Set Blind scan stop frequency*/
        tuner_status[path].bs_setting.bsPara.maxSymbolRate = M_BS_MAX_SYMB * 1000 * 1000;   /*Set MAX symbol rate*/
        tuner_status[path].bs_setting.bsPara.minSymbolRate = M_BS_MIN_SYMB * 1000 * 1000;   /*Set MIN symbol rate*/
        tuner_status[path].bs_setting.bsPara.timeout = FEND_WAIT_TIMEOUT;
        tuner_status[path].bs_setting.bsPara.minfrequency = start_freq/1000;		        /*Change default start frequency*/
        tuner_status[path].bs_setting.bsPara.maxfrequency = stop_freq/1000;			        /*Change default end frequency*/

        tuner_status[path].bs_setting.singlecablePara.version = unicable.unicable;
        tuner_status[path].bs_setting.singlecablePara.userband = unicable.channel;
        tuner_status[path].bs_setting.singlecablePara.frequency = unicable.frequency;
        tuner_status[path].bs_setting.singlecablePara.bank = unicable.bank;
        tuner_status[path].bs_setting.singlecablePara.uncommitted = unicable.uncommitted;
        tuner_status[path].bs_setting.singlecablePara.committed = unicable.committed;
    }
    else if (sys_type == TUNE_SYSTEM_TYPE_DVBC)
    {
        tuner_status[path].bs_setting.bsPara.minfrequency = start_freq;         /*Default Set Blind scan start frequency*/
        tuner_status[path].bs_setting.bsPara.maxfrequency = stop_freq;          /*Default Set Blind scan stop frequency*/
    }

    /*blindscan handle thread*/
    if (cb != tuner_status[path].blindscan_cb || user_data != tuner_status[path].blindscan_cb_user_data)
    {
        tuner_status[path].blindscan_cb = cb;
        tuner_status[path].blindscan_cb_user_data = user_data;
    }

    tuner_status[path].enable_blindscan_thread = TRUE;

    tuner_status[path].blindscan_thread = STB_OSCreateTask(fend_blindscan_thread,
                                                           (void *)(long)path, 0/*stack*/, 0/*priority*/,
                                                           (U8BIT *)"TunerBlindScanTask");
    if (tuner_status[path].blindscan_thread == NULL)
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN STB_Tune_BlindContinue(U8BIT path)
{
    return dvb_blindscan_continue(path);
}

BOOLEAN STB_Tune_BlindExit(U8BIT path)
{
    BOOLEAN ret = TRUE;

    /*Stop the thread*/
    tuner_status[path].enable_blindscan_thread = FALSE;
    STB_OSDestroyTask(tuner_status[path].blindscan_thread);

    return ret;
}

void STB_Tune_BlindGetTPCount(U8BIT path, U16BIT *count)
{
    pthread_mutex_lock(&tuner_status[path].lock);

    *count = 0;

    if(tuner_status[path].bs_setting.m_uiChannelCount)
    {
        *count = (unsigned int)(tuner_status[path].bs_setting.m_uiChannelCount);
    }

    pthread_mutex_unlock(&tuner_status[path].lock);

}

BOOLEAN STB_Tune_BlindGetTPInfo(U8BIT path, void *para, U16BIT *count)
{
    BOOLEAN ret = TRUE;
    para = (struct dvb_frontend_parameters *)para;

    if (!para)
    {
        *count = 0;
        return FALSE;
    }

    pthread_mutex_lock(&tuner_status[path].lock);

    if((*count) > tuner_status[path].bs_setting.m_uiChannelCount)
    {
        *count = (unsigned int)(tuner_status[path].bs_setting.m_uiChannelCount);
    }

    memcpy(para, tuner_status[path].bs_setting.channels, (*count) * sizeof(struct dvb_frontend_parameters));

    pthread_mutex_unlock(&tuner_status[path].lock);
    return ret;
}

/*---local function definitions----------------------------------------------*/

static BOOLEAN SetSysType(S_TUNER_STATUS *tstatus, E_STB_TUNE_SIGNAL_TYPE sig_type)
{
    BOOLEAN retval;
    BOOLEAN sig_sys_mismatch;
    BOOLEAN tuned_sys_mismatch;
    char fe_name[24];
    int mode;
    memset(fe_name, 0, sizeof(fe_name));
    retval = FALSE;
    sig_sys_mismatch = FALSE;
    tuned_sys_mismatch = (tstatus->tuned_sys_type != tstatus->sys_type);

    if (tstatus->frontend_fd != INVALID_FD)
    {
        switch (sig_type)
        {
            case TUNE_SIGNAL_QPSK:
                sig_sys_mismatch = (tstatus->tuned_sys_type != TUNE_SYSTEM_TYPE_DVBS && tstatus->tuned_sys_type != TUNE_SYSTEM_TYPE_DVBS2);
                if (sig_sys_mismatch || tuned_sys_mismatch)
                {
                    tstatus->tuned_sys_type = (tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS2) ? TUNE_SYSTEM_TYPE_DVBS2 : TUNE_SYSTEM_TYPE_DVBS;
                }

                break;

            case TUNE_SIGNAL_COFDM:
                sig_sys_mismatch = (tstatus->tuned_sys_type != TUNE_SYSTEM_TYPE_DVBT && tstatus->tuned_sys_type != TUNE_SYSTEM_TYPE_DVBT2);
                if (sig_sys_mismatch || tuned_sys_mismatch)
                {
                    tstatus->tuned_sys_type = (tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2) ? TUNE_SYSTEM_TYPE_DVBT2 : TUNE_SYSTEM_TYPE_DVBT;
                }

                break;

            case TUNE_SIGNAL_QAM:
                if (tstatus->tuned_sys_type != TUNE_SYSTEM_TYPE_DVBC)
                    tstatus->tuned_sys_type = TUNE_SYSTEM_TYPE_DVBC;

                break;

            case TUNE_SIGNAL_ISDBT:
                if (tstatus->tuned_sys_type != TUNE_SYSTEM_TYPE_ISDBT)
                    tstatus->tuned_sys_type = TUNE_SYSTEM_TYPE_ISDBT;

                break;

            default:
                TUN_ERR("not support sig_type:%d\n", sig_type);
                return retval;
        }

        if (SetFeProperty(tstatus->frontend_fd, tstatus->tuned_sys_type))
        {
            memset(&tstatus->fe_info, 0, sizeof(tstatus->fe_info));

            if (ioctl(tstatus->frontend_fd, FE_GET_INFO, &(tstatus->fe_info)) >= 0)
            {
                TUN_DBG("fe_info.type=%d", tstatus->fe_info.type);

                if (tstatus->fe_info.type == FE_OFDM)
                {
                    if (tstatus->tuned_sys_type == TUNE_SYSTEM_TYPE_ISDBT)
                    {
                        TUN_DBG("Tuner %s configured as ISDBT, min_freq=%lu, max_freq=%lu", fe_name,
                                tstatus->fe_info.frequency_min, tstatus->fe_info.frequency_max);
                        tstatus->signal_type = TUNE_SIGNAL_ISDBT;
                        tstatus->delivery_system = SYS_ISDBT;
                    }
                    else
                    {
                        TUN_DBG("Tuner %s configured as DVB-T/T2, min_freq=%lu, max_freq=%lu", fe_name,
                                tstatus->fe_info.frequency_min, tstatus->fe_info.frequency_max);
                        tstatus->signal_type = TUNE_SIGNAL_COFDM;
                        tstatus->delivery_system = SYS_DVBT2;
                    }
                }
                else if (tstatus->fe_info.type == FE_QAM)
                {
                    TUN_DBG("Tuner %s configured as DVBC, min_freq=%lu, max_freq=%lu", fe_name,
                            tstatus->fe_info.frequency_min, tstatus->fe_info.frequency_max);
                    tstatus->signal_type = TUNE_SIGNAL_QAM;
                    tstatus->delivery_system = SYS_DVBC_ANNEX_A;
                }
                else
                {
                    TUN_DBG("Tuner %s configured as DVB-S/S2, freq min/max=%lu/%lu, symbol rate min/max=%lu/%lu",
                            fe_name, tstatus->fe_info.frequency_min, tstatus->fe_info.frequency_max,
                            tstatus->fe_info.symbol_rate_min, tstatus->fe_info.symbol_rate_max);
                    tstatus->signal_type = TUNE_SIGNAL_QPSK;
                    tstatus->delivery_system = SYS_DVBS2;
                }

                retval = TRUE;
            }
            else
            {
                TUN_DBG("Failed to get FE_INFO for %s, errno %d", fe_name, errno);
            }
        }
        else
        {
            TUN_DBG("Failed to SetFeProperty for %s, errno %d", fe_name, errno);
        }
    }

    return(retval);
}

static BOOLEAN OpenTuner(S_TUNER_STATUS *tstatus)
{
    BOOLEAN retval, istv;
    char fe_name[24];
    int tuner_index;
    retval = TRUE;

    if (STB_TuneIsTvPlatform() && !resm_adc_requested && STB_Resman_Support())
    {
        if (!STB_Resman_Request(RESMAN_APP_DVBKIT, RESMAN_ID_ADC_PLL, 2000))
        {
            TUN_DBG("STB_Resman_Request RESMAN_ID_ADC_PLL failed!!!");

            return FALSE;
        }

        resm_adc_requested = TRUE;

        TUN_DBG("STB_Resman_Request RESMAN_ID_ADC_PLL OK.");
    }

    if (tstatus->frontend_fd != INVALID_FD)
    {
        TUN_DBG("FE is already open, frontend_fd:%d", tstatus->frontend_fd);
    }
    else
    {
        tuner_index = tstatus->path >= aml_hw_cfg.tuner_num ? aml_hw_cfg.tuner_num-1 : tstatus->path;
        snprintf(fe_name, sizeof(fe_name), "/dev/dvb0.frontend%u", aml_hw_cfg.tuners[tuner_index].frontend_idx);

        if ((tstatus->frontend_fd = open(fe_name, O_RDWR | O_NONBLOCK)) < 0)
        {
            TUN_ERR("Failed to open tune[%d] %s, errno %d", tuner_index, fe_name, errno);
            retval = FALSE;
        }
        else
        {
            TUN_DBG("Open tune[%d] %s frontend_fd:%d ", tuner_index, fe_name, tstatus->frontend_fd);
        }
    }


    return(retval);
}

static void CloseTuner(S_TUNER_STATUS *tstatus)
{
    if ((NULL != tstatus) && (tstatus->frontend_fd != INVALID_FD))
    {
        TUN_DBG("path %u: close tuner frontend_fd:%d", tstatus->path,tstatus->frontend_fd);

        if (tstatus->signal_type == TUNE_SIGNAL_QPSK)
        {
            TUN_DBG("path %u: lnb power and 22khz off frontend_fd:%d", tstatus->path,tstatus->frontend_fd);
            STB_TuneSetLNBVoltage(tstatus->path, LNB_VOLTAGE_OFF, FALSE);
            STB_TuneSet22kState(tstatus->path, FALSE, FALSE);
        }

        SetFeProperty(tstatus->frontend_fd, TUNE_SYSTEM_TYPE_ANALOG);
        tstatus->signal_type = TUNE_SIGNAL_NONE;
        close(tstatus->frontend_fd);
        tstatus->frontend_fd = INVALID_FD;
        tstatus->freq = 0;
        #ifdef EMUTUNNER_ENABLE
        EmuTunerStop(tstatus->path);
        #endif
    }

    if (STB_TuneIsTvPlatform() && resm_adc_requested && STB_Resman_Support())
    {
        STB_Resman_FreeRes(RESMAN_ID_ADC_PLL);

        resm_adc_requested = FALSE;

        TUN_DBG("STB_Resman_FreeRes RESMAN_ID_ADC_PLL OK.");
    }
}

/*static*/ BOOLEAN StartTune(S_TUNER_STATUS *tstatus)
{
    BOOLEAN retval;
    struct dvb_frontend_parameters fe_params;
    fe_sec_voltage_t voltage;
    fe_sec_tone_mode_t tone;

    retval = FALSE;

    switch (tstatus->signal_type)
    {
        case TUNE_SIGNAL_COFDM:
        {
            fe_params.frequency = tstatus->freq;

            switch (tstatus->u.terr.tbwidth)
            {
                case TUNE_TBWIDTH_5MHZ:
                    fe_params.u.ofdm.bandwidth = BANDWIDTH_5_MHZ;
                    break;

                case TUNE_TBWIDTH_6MHZ:
                    fe_params.u.ofdm.bandwidth = BANDWIDTH_6_MHZ;
                    break;

                case TUNE_TBWIDTH_7MHZ:
                    fe_params.u.ofdm.bandwidth = BANDWIDTH_7_MHZ;
                    break;

                case TUNE_TBWIDTH_8MHZ:
                    fe_params.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
                    break;

                case TUNE_TBWIDTH_10MHZ:
                    fe_params.u.ofdm.bandwidth = BANDWIDTH_10_MHZ;
                    break;

                default:
                    fe_params.u.ofdm.bandwidth = BANDWIDTH_AUTO;
                    break;
            }

            fe_params.u.ofdm.code_rate_HP = FEC_AUTO;
            fe_params.u.ofdm.code_rate_LP = FEC_AUTO;

            switch (tstatus->u.terr.tmode)
            {
                case TUNE_MODE_COFDM_1K:
                    fe_params.u.ofdm.transmission_mode = TRANSMISSION_MODE_1K;
                    break;

                case TUNE_MODE_COFDM_2K:
                    fe_params.u.ofdm.transmission_mode = TRANSMISSION_MODE_2K;
                    break;

                case TUNE_MODE_COFDM_4K:
                    fe_params.u.ofdm.transmission_mode = TRANSMISSION_MODE_4K;
                    break;

                case TUNE_MODE_COFDM_8K:
                    fe_params.u.ofdm.transmission_mode = TRANSMISSION_MODE_8K;
                    break;

                case TUNE_MODE_COFDM_16K:
                    fe_params.u.ofdm.transmission_mode = TRANSMISSION_MODE_16K;
                    break;

                case TUNE_MODE_COFDM_32K:
                    fe_params.u.ofdm.transmission_mode = TRANSMISSION_MODE_32K;
                    break;

                default:
                    fe_params.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
                    break;
            }

            fe_params.u.ofdm.constellation = QAM_AUTO;
            fe_params.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;

            if (ioctl(tstatus->frontend_fd, FE_SET_FRONTEND, &fe_params) >= 0)
            {
                TUN_DBG("%u: Tuning to %lu", tstatus->path, tstatus->freq);
                retval = TRUE;
            }
            else
            {
                TUN_ERR("%u: Unable to set tuning parameters, errno %d", tstatus->path, errno);
            }

            break;
        }

        case TUNE_SIGNAL_QAM:
        {
            fe_params.frequency = tstatus->freq;

            switch (tstatus->u.cab.cmode)
            {
                case TUNE_MODE_QAM_16:
                    fe_params.u.qam.modulation = QAM_16;
                    break;

                case TUNE_MODE_QAM_32:
                    fe_params.u.qam.modulation = QAM_32;
                    break;

                case TUNE_MODE_QAM_64:
                    fe_params.u.qam.modulation = QAM_64;
                    break;

                case TUNE_MODE_QAM_128:
                    fe_params.u.qam.modulation = QAM_128;
                    break;

                case TUNE_MODE_QAM_256:
                    fe_params.u.qam.modulation = QAM_256;
                    break;

                default:
                    fe_params.u.qam.modulation = QAM_AUTO;
                    break;
            }

            fe_params.u.qam.symbol_rate = tstatus->u.cab.srate;
            TUN_DBG("[%s] fe_params.u.qam.symbol_rate = %lu, fe_params.u.qam.modulation = %u\n", __FUNCTION__,
                    fe_params.u.qam.symbol_rate, fe_params.u.qam.modulation);

            if (ioctl(tstatus->frontend_fd, FE_SET_FRONTEND, &fe_params) >= 0)
            {
                TUN_DBG("%u: Tuning to %lu", tstatus->path, tstatus->freq);
                retval = TRUE;
            }
            else
            {
                TUN_ERR("%u: Unable to set tuning parameters, errno %d", tstatus->path, errno);
            }

            break;
        }

        case TUNE_SIGNAL_QPSK:
        {
            if (STB_TuneSetTone(tstatus->path, tstatus->u.sat.use_22khz))
            {
                fe_params.frequency = tstatus->freq;
                fe_params.inversion = INVERSION_AUTO;
                fe_params.u.qpsk.symbol_rate = tstatus->u.sat.srate;

                switch (tstatus->u.sat.fec)
                {
                    case TUNE_FEC_1_2:
                        fe_params.u.qpsk.fec_inner = FEC_1_2;
                        break;

                    case TUNE_FEC_2_3:
                        fe_params.u.qpsk.fec_inner = FEC_2_3;
                        break;

                    case TUNE_FEC_3_4:
                        fe_params.u.qpsk.fec_inner = FEC_3_4;
                        break;

                    case TUNE_FEC_5_6:
                        fe_params.u.qpsk.fec_inner = FEC_5_6;
                        break;

                    case TUNE_FEC_7_8:
                        fe_params.u.qpsk.fec_inner = FEC_7_8;
                        break;

                    case TUNE_FEC_2_5:
                        fe_params.u.qpsk.fec_inner = FEC_2_5;
                        break;

                    case TUNE_FEC_8_9:
                        fe_params.u.qpsk.fec_inner = FEC_8_9;
                        break;

                    case TUNE_FEC_9_10:
                        fe_params.u.qpsk.fec_inner = FEC_9_10;
                        break;

                    default:
                        fe_params.u.qpsk.fec_inner = FEC_AUTO;
                        break;
                }

                if (ioctl(tstatus->frontend_fd, FE_SET_FRONTEND, &fe_params) >= 0)
                {
                    TUN_DBG("%u: Tuning to %lu %lu %d", tstatus->path, tstatus->freq, tstatus->u.sat.srate, tstatus->u.sat.fec);
                    retval = TRUE;
                }
                else
                {
                    TUN_ERR("%u: Unable to set tuning parameters, errno %d", tstatus->path, errno);
                }
            }
            else
            {
                TUN_ERR("%u: Failed to set tone, errno %d", tstatus->path, errno);
            }

            break;
        }

        case TUNE_SIGNAL_ISDBT:
        {
            fe_params.frequency = tstatus->freq;

            switch (tstatus->u.isdbt.tbwidth)
            {
                case TUNE_TBWIDTH_6MHZ:
                    fe_params.u.ofdm.bandwidth = BANDWIDTH_6_MHZ;
                    break;

                case TUNE_TBWIDTH_7MHZ:
                    fe_params.u.ofdm.bandwidth = BANDWIDTH_7_MHZ;
                    break;

                case TUNE_TBWIDTH_8MHZ:
                    fe_params.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
                    break;

                default:
                    fe_params.u.ofdm.bandwidth = BANDWIDTH_AUTO;
                    break;
            }

            fe_params.u.ofdm.code_rate_HP = FEC_AUTO;
            fe_params.u.ofdm.code_rate_LP = FEC_AUTO;
            fe_params.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
            fe_params.u.ofdm.constellation = QAM_AUTO;
            fe_params.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;

            if (ioctl(tstatus->frontend_fd, FE_SET_FRONTEND, &fe_params) >= 0)
            {
                TUN_DBG("%u: Tuning to %lu", tstatus->path, tstatus->freq);
                retval = TRUE;
            }
            else
            {
                TUN_ERR("%u: Unable to set tuning parameters, errno %d (%s)", tstatus->path, errno, strerror(errno));
            }

            break;
        }

        default:
        {
            TUN_ERR("%u: Unsupported tuner type %u", tstatus->path, tstatus->signal_type);
            break;
        }
    }

    return(retval);
}

static BOOLEAN IsTunerLocked(S_TUNER_STATUS *tstatus)
{
    BOOLEAN locked;

    STB_OSMutexLock(tstatus->mutex);

    if (tstatus->state == TUNER_LOCKED)
    {
        locked = TRUE;
    }
    else
    {
        locked = FALSE;
    }

    STB_OSMutexUnlock(tstatus->mutex);

    return(locked);
}

static BOOLEAN IsTuningParameterMatched(S_TUNER_STATUS *tstatus, struct dvb_frontend_event event)
{
    BOOLEAN result;
    if (TUNE_SYSTEM_TYPE_DVBS == tstatus->sys_type || TUNE_SYSTEM_TYPE_DVBS2 == tstatus->sys_type)
    {
        TUN_INFO("status freq: %lu, event freq: %lu", tstatus->freq, event.parameters.frequency);
        if (event.parameters.frequency != tstatus->freq) {
            return FALSE;
        }
    }

    return TRUE;
}

static void* TunerTask(void *param)
{
    S_TUNER_STATUS *tstatus = param;
    E_TUNER_STATE state;
    BOOLEAN locked;
    BOOLEAN tuner_locked;
    U32BIT start_time;
    BOOLEAN stop;
    U8BIT delay_step = 0;
    U8BIT tune_idle_timer =0;
    U16BIT lost_signal_times=0;
    struct dvb_frontend_parameters fe_params;
    struct pollfd pfd;
    struct dvb_frontend_event fe_event;

    U32BIT wait_lock_timeout = WAIT_LOCK_TIMEOUT;

    while (TRUE)
    {
        STB_OSMutexLock(tstatus->mutex);
        state = tstatus->state;
        STB_OSMutexUnlock(tstatus->mutex);

        if (state == TUNER_IDLE)
        {
            /* Wait until tuning has been started */
            //TUN_DBG("%u: Waiting for tune request....", tstatus->path);
            BOOLEAN sem_ret = STB_OSSemaphoreWaitTimeout(tstatus->tune_sem, 1000);

            if (sem_ret)
            {
                tune_idle_timer = 0;
                if (FEND_FL_LOCK == (tstatus->lock_flags & FEND_FL_LOCK))
                {
                    STB_OSMutexLock(tstatus->mutex);
                    tstatus->state = TUNER_LOCKED;
                    state = tstatus->state;
                    STB_OSMutexUnlock(tstatus->mutex);
                    tstatus->lock_flags &= ~FEND_FL_LOCK;
                    TUN_INFO("##### %u: Already_Tuned fd:%d #####", tstatus->path, tstatus->frontend_fd);

                    STB_TimeConsumeDebug("Tune lock end");
                    goto Already_Tuned;
                }

                tstatus->state = TUNER_TUNING;
                stop = tstatus->stop;
                STB_OSMutexUnlock(tstatus->mutex);
                TUN_INFO("%u: Tuning started, checking LOCK status", tstatus->path);

                pfd.fd = tstatus->frontend_fd;
                pfd.events = POLLIN;
                pfd.revents = 0;


                if (TUNE_SIGNAL_QAM == tstatus->signal_type && 0 == tstatus->u.cab.srate)
                {   // 2021-05-12: for DVBC SRate Auto Mode, need more timeout
                    wait_lock_timeout = 30000;
                }
                else
                {
                    wait_lock_timeout = WAIT_LOCK_TIMEOUT;
                }

                TUN_INFO("wait_lock_timeout:%u", wait_lock_timeout);

                for (locked = FALSE, start_time = STB_OSGetClockMilliseconds();
                        !stop && !locked && (STB_OSGetClockDiff(start_time) < WAIT_LOCK_TIMEOUT); )
                {
                    if (poll(&pfd, 1, TUNER_POLLING_TIMEOUT) == 1)
                    {
                        if (ioctl(tstatus->frontend_fd, FE_GET_EVENT, &fe_event) >= 0)
                        {
                            TUN_INFO("status=0x%02x", fe_event.status);

                            if ((fe_event.status & FE_HAS_LOCK) != 0)
                            {
                                locked = TRUE;
                            }
                            else if ((fe_event.status & FE_TIMEDOUT) != 0)
                            {
                                /* Failed to lock */
                                break;
                            }
                        }
                    }

                    STB_OSMutexLock(tstatus->mutex);
                    stop = tstatus->stop;
                    STB_OSMutexUnlock(tstatus->mutex);
                }

                if (stop)
                {
                    TUN_INFO("%u: Tuning stopped", tstatus->path);
                    STB_OSMutexLock(tstatus->mutex);
                    tstatus->state = TUNER_IDLE;
                    STB_OSMutexUnlock(tstatus->mutex);
                }
                else
                {
                    if (locked)
                    {
                        /* The tuner locks when set to T or T2, so check whether the mode is correct for what was set */
                        struct dtv_property p = {.cmd = DTV_DELIVERY_SYSTEM, .u.data = 0};
                        struct dtv_properties props = {.num = 1, .props = &p};

                        if (ioctl(tstatus->frontend_fd, FE_GET_PROPERTY, &props) >= 0)
                        {
                            STB_OSMutexLock(tstatus->mutex);

                            if ((((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT) && (p.u.data != SYS_DVBT)) ||
                                    ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2) && (p.u.data != SYS_DVBT2))) &&
                                    (tstatus->signal_type != TUNE_SIGNAL_QAM))
                            {
                                locked = FALSE;
                                TUN_INFO("%u: Ignoring LOCKED status for %s, delivery system is %s", tstatus->path,
                                        GetSysTypeDebugString(tstatus->sys_type),
                                        ((p.u.data == SYS_DVBT) ? "DVB-T" : "DVB-T2"));
                            }
                            else if ((SYS_DVBS == p.u.data || SYS_DVBS2 == p.u.data) &&
                                     (TUNE_SYSTEM_TYPE_DVBS == tstatus->sys_type || TUNE_SYSTEM_TYPE_DVBS2 == tstatus->sys_type))
                            {
                                TUN_INFO("[%s:%d] data:%u, sys_type:%u", __FUNCTION__, __LINE__, p.u.data, tstatus->sys_type);

                                if (SYS_DVBS == p.u.data)
                                    tstatus->sys_type = TUNE_SYSTEM_TYPE_DVBS;
                                else
                                    tstatus->sys_type = TUNE_SYSTEM_TYPE_DVBS2;

                                // reserved[0] is used for modulation in demod
                                tstatus->u.sat.modulation = GetTuneModulation(p.reserved[0]);
                                TUN_INFO("[%s:%d] reserved[0]:%u, modulation:%u",
                                         __FUNCTION__, __LINE__, p.reserved[0], tstatus->u.sat.modulation);
                            }

                            STB_OSMutexUnlock(tstatus->mutex);
                        }
                    }

                    if (locked)
                    {
                        TUN_INFO("%u: LOCKED", tstatus->path);
                        STB_TimeConsumeDebug("Tune lock end");

                        STB_OSMutexLock(tstatus->mutex);
                        tstatus->state = TUNER_LOCKED;
                        STB_OSMutexUnlock(tstatus->mutex);

                        STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_LOCKED, &tstatus->path,
                                        sizeof(U8BIT));
                    }
                    else
                    {
                        TUN_INFO("%u: NOT LOCKED", tstatus->path);
                        STB_TimeConsumeDebug("Tune lock end");
                        //ClearTuner(tstatus);
                        STB_OSMutexLock(tstatus->mutex);
                        tstatus->state = TUNER_RELOCKING;
                        STB_OSMutexUnlock(tstatus->mutex);

                        STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path,
                                        sizeof(U8BIT));
                    }


                    TUN_INFO("path:%u: sem_signal:%p", tstatus->path, tstatus->tune_sem_lock);
                }
            }
            else
            {
                tune_idle_timer ++;
                TUN_INFO("path:%u:tune_idle_timer:%u,frontend_usage=%d", tstatus->path, tune_idle_timer,tstatus->frontend_usage);

                if (tune_idle_timer >= TUNER_USELESS_TIMEOUT && tstatus->frontend_usage == 0)
                {
                    pthread_mutex_lock(&tstatus->lock);
                    CloseTuner(tstatus);
                    tune_idle_timer = 0;
                    pthread_mutex_unlock(&tstatus->lock);
                }
            }
        }
        else if (state == TUNER_EXITED)
        {
            TUN_DBG("%u: [state = TUNER_EXITED] Waiting for tune start....", tstatus->path);
            tune_idle_timer = 0;
            usleep(1000 * 1000);
        }
        else
        {
        Already_Tuned:
            /* Monitor tuner lock status */
            tune_idle_timer = 0;

            if (state == TUNER_LOCKED)
            {
                locked = TRUE;
                tuner_locked = TRUE;
            }
            else if (state == TUNER_RELOCKING)
            {
                locked = FALSE;
                tuner_locked = FALSE;
            }

            pfd.fd = tstatus->frontend_fd;
            pfd.events = POLLIN;
            pfd.revents = 0;

            while ((state == TUNER_LOCKED) || (state == TUNER_RELOCKING))
            {
                if (poll(&pfd, 1, TUNER_POLLING_TIMEOUT) == 1)
                {
                    if (ioctl(tstatus->frontend_fd, FE_GET_EVENT, &fe_event) >= 0)
                    {
                        if (!IsTuningParameterMatched(tstatus, fe_event))
                        {
                            break;
                        }

                        if ((fe_event.status & FE_HAS_LOCK) != 0)
                        {
                            tuner_locked = TRUE;
                            TUN_ERR("FE_GET_EVENT LOCKED:%d state:%d", locked, state);

                            struct dtv_property p = {.cmd = DTV_DELIVERY_SYSTEM, .u.data = 0};
                            struct dtv_properties props = {.num = 1, .props = &p};

                            if (ioctl(tstatus->frontend_fd, FE_GET_PROPERTY, &props) >= 0)
                            {
                                if ((SYS_DVBS == p.u.data || SYS_DVBS2 == p.u.data) &&
                                      (TUNE_SYSTEM_TYPE_DVBS == tstatus->sys_type || TUNE_SYSTEM_TYPE_DVBS2 == tstatus->sys_type))
                                {
                                   TUN_INFO("[%s:%d] data:%u, sys_type:%u", __FUNCTION__, __LINE__, p.u.data, tstatus->sys_type);
                                   STB_OSMutexLock(tstatus->mutex);
                                   if (SYS_DVBS == p.u.data)
                                       tstatus->sys_type = TUNE_SYSTEM_TYPE_DVBS;
                                   else
                                       tstatus->sys_type = TUNE_SYSTEM_TYPE_DVBS2;

                                   // reserved[0] is used for modulation in demod
                                   tstatus->u.sat.modulation = GetTuneModulation(p.reserved[0]);
                                   STB_OSMutexUnlock(tstatus->mutex);
                                   TUN_INFO("[%s:%d] reserved[0]:%u, modulation:%u",
                                            __FUNCTION__, __LINE__, p.reserved[0], tstatus->u.sat.modulation);
                                }
                            }
                        }
                        else if ((fe_event.status & FE_TIMEDOUT) != 0)
                        {
                            tuner_locked = FALSE;
                            TUN_ERR("FE_GET_EVENT UNLOCKED:%d state:%d", locked, state);
                        }
                    }
                    else
                    {
                        TUN_ERR("%u: FE_GET_EVENT failed, errno %d", tstatus->path, errno);
                    }
                }

                STB_OSMutexLock(tstatus->mutex);
                stop = tstatus->stop;
                STB_OSMutexUnlock(tstatus->mutex);

                if (stop)
                {
                    STB_OSMutexLock(tstatus->mutex);
                    tstatus->stop = FALSE;
                    tstatus->state = TUNER_IDLE;
                    TUN_DBG("%u: Tuned stopped", tstatus->path);
                    STB_OSMutexUnlock(tstatus->mutex);
                    delay_step = 0;
                }
                else
                {
                    if (tuner_locked)
                    {
                        lost_signal_times = 0;
                        if (!locked)
                        {
                            /* Tuner has relocked */
                            TUN_DBG("%u: Tuner has relocked", tstatus->path);
                            locked = TRUE;

                            STB_OSMutexLock(tstatus->mutex);
                            tstatus->state = TUNER_LOCKED;
                            STB_OSMutexUnlock(tstatus->mutex);
                            STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_LOCKED, &tstatus->path, sizeof(U8BIT));
                        }

                        delay_step = 0;
                    }
                    else
                    {
                        if (locked)
                        {
                            if(TRUE == STB_TuneIsSearchMode(tstatus->path))//Filter unstable signals
                            {
                                if(lost_signal_times<TUNER_LOST_LOCK_TIMES)
                                {
                                    lost_signal_times++;
                                }
                                else
                                {
                                    TUN_DBG("%u: Lost LOCK, relock %u,try times(%d)", tstatus->path, tstatus->auto_relock,lost_signal_times);
                                    locked = FALSE;

                                    if (state == TUNER_LOCKED)
                                    {
                                        STB_OSMutexLock(tstatus->mutex);
                                        tstatus->state = TUNER_RELOCKING;
                                        STB_OSMutexUnlock(tstatus->mutex);
                                        STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path, sizeof(U8BIT));
                                    }
                                    lost_signal_times=0;
                                }
                            }
                            else
                            {
                                /* Lost lock */
                                TUN_DBG("%u: Lost LOCK, relock %u", tstatus->path, tstatus->auto_relock);

                                locked = FALSE;

                                if (state == TUNER_LOCKED)
                                {
                                    STB_OSMutexLock(tstatus->mutex);
                                    tstatus->state = TUNER_RELOCKING;
                                    STB_OSMutexUnlock(tstatus->mutex);
                                    STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path, sizeof(U8BIT));
                                }
                            }
                        }
                        else if (tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS || tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS2)
                        {
                            delay_step++;

                            if (delay_step >= 40)
                            {
                                delay_step = 0;
                                STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_SIGNAL_RECOVER, &tstatus->path, sizeof(U8BIT));
                            }
                        }
                    }
                }

                STB_OSMutexLock(tstatus->mutex);
                state = tstatus->state;
                STB_OSMutexUnlock(tstatus->mutex);
            }
        }
    }
    TUN_DBG("%u: o o  task exit....", tstatus->path);
    return NULL;
}

static void ClearTuner(S_TUNER_STATUS *tstatus)
{
    struct dtv_property cmd;
    struct dtv_properties props;

    memset(&cmd, 0, sizeof(struct dtv_property));
    cmd.cmd = DTV_CLEAR;
    props.num = 1;
    props.props = &cmd;

    if (ioctl(tstatus->frontend_fd, FE_SET_PROPERTY, &props) < 0)
    {
        TUN_ERR("%u: DTV_CLEAR failed, errno %d", tstatus->path, errno);
    }
}

/*static*/ U8BIT* GetSysTypeDebugString(E_STB_TUNE_SYSTEM_TYPE sys_type)
{
    U8BIT *string;

    if (sys_type == TUNE_SYSTEM_TYPE_DVBT)
    {
        string = (U8BIT *)"DVB-T";
    }
    else if (sys_type == TUNE_SYSTEM_TYPE_DVBT2)
    {
        string = (U8BIT *)"DVB-T2";
    }
    else if (sys_type == TUNE_SYSTEM_TYPE_DVBS)
    {
        string = (U8BIT *)"DVB-S";
    }
    else if (sys_type == TUNE_SYSTEM_TYPE_DVBS2)
    {
        string = (U8BIT *)"DVB-S2";
    }
    else if (sys_type == TUNE_SYSTEM_TYPE_DVBC)
    {
        string = (U8BIT *)"DVB-C";
    }
    else if (sys_type == TUNE_SYSTEM_TYPE_UNKNOWN)
    {
        string = (U8BIT *)"UNKNOWN";
    }
    else if (sys_type == TUNE_SYSTEM_TYPE_ISDBT)
    {
        string = (U8BIT *)"DVB-ISDBT";
    }
    else if (sys_type == TUNE_SYSTEM_TYPE_ANALOG)
    {
        string = (U8BIT *)"DVB-ANALOG";
    }
    else
    {
        TUN_DBG("ERROR: sys_type = %d, is invalid.", sys_type);
        string = (U8BIT *)"UNKNOWN";
    }

    return(string);
}

static BOOLEAN IsDiffSysType(S_TUNER_STATUS * tstatus)
{
    BOOLEAN is_diff = TRUE;

    struct dtv_property p = {.cmd = DTV_DELIVERY_SYSTEM, .u.data = 0};
    struct dtv_properties props = {.num = 1, .props = &p};

    if (ioctl(tstatus->frontend_fd, FE_GET_PROPERTY, &props) >= 0)
    {
        if (((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT) && (p.u.data == SYS_DVBT)) ||
                    ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2) && (p.u.data == SYS_DVBT2)) ||
                ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS) && (p.u.data == SYS_DVBS)) ||
                    ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS2) && (p.u.data == SYS_DVBS2)) ||
                ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBC)  &&
                     ((p.u.data == SYS_DVBC_ANNEX_A) || (p.u.data == SYS_DVBC_ANNEX_B) || (p.u.data == SYS_DVBC_ANNEX_C))))
        {
            TUN_DBG(" same sys_type %s, delivery system is %d", GetSysTypeDebugString(tstatus->sys_type), p.u.data);
            is_diff = FALSE;
        }
        else
        {
            TUN_DBG(" different sys_type %s, delivery system is %d", GetSysTypeDebugString(tstatus->sys_type), p.u.data);
            is_diff = TRUE;
        }
    }
    else
    {
        TUN_DBG("ERROR: FE_GET_PROPERTY is failed!");
        is_diff = TRUE;
    }

    return is_diff;
}

/*static*/ E_TUNER_EVENT GetTunerLockStatus(U32BIT frontend_fd)
{
    struct dvb_frontend_event fe_event;
    E_TUNER_EVENT tune_event = TUNER_STATE_UNKNOW;

    if (ioctl(frontend_fd, FE_READ_STATUS, &fe_event.status) >= 0)
    {
        TUN_DBG("status=0x%02x", fe_event.status);

        if ((fe_event.status & FE_HAS_LOCK) != 0)
        {
            tune_event = TUNER_STATE_LOCKED;
        }
        else if ((fe_event.status & FE_TIMEDOUT) != 0)
        {
            tune_event = TUNER_STATE_TIMEOUT;
        }
    }
    else
    {
        TUN_DBG("frontend_fd:%d FE_READ_STATUS errno:%d",frontend_fd, errno);
    }

    return tune_event;
}

static void SetTunerT2PLP(U32BIT frontend_fd, U8BIT plp_id)
{
    struct dtv_property cmd;
    struct dtv_properties props;

    TUN_DBG("Set PLP %u", plp_id);
    memset(&cmd, 0, sizeof(struct dtv_property));
    cmd.cmd = DTV_DVBT2_PLP_ID;
    cmd.u.data = plp_id;
    props.num = 1;
    props.props = &cmd;

    if (ioctl(frontend_fd, FE_SET_PROPERTY, &props) < 0)
    {
        TUN_ERR(" Failed to set number of PLPs, errno %d", errno);
    }
}

static BOOLEAN dvb_set_prop (U32BIT fd, const struct dtv_properties *prop)
{
    if (ioctl(fd, FE_SET_PROPERTY, prop) < 0)
    {
        TUN_ERR("ioctl FE_SET_PROPERTY failed, error:%s", strerror(errno));
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN dvb_wait_event (U32BIT fd, struct dvb_frontend_event *evt, int timeout)
{
    BOOLEAN ret;
    struct pollfd pfd;
    struct dvb_frontend_event event;

    pfd.fd = fd;
    pfd.events = POLLIN;

    ret = poll(&pfd, 1, timeout);

    if (ret != 1)
    {
        return FALSE;
    }

    if (ioctl(fd, FE_GET_EVENT, &event) < 0)
    {
        TUN_ERR("ioctl FE_GET_EVENT failed, error:%s", strerror(errno));
        return FALSE;
    }

    evt->status = event.status;
    evt->parameters.frequency = event.parameters.frequency;
    evt->parameters.inversion = event.parameters.inversion;
    evt->parameters.u.qpsk = event.parameters.u.qpsk;
    evt->parameters.u.qam = event.parameters.u.qam;
    evt->parameters.u.ofdm = event.parameters.u.ofdm;
    evt->parameters.u.vsb = event.parameters.u.vsb;

    return TRUE;
}

static BOOLEAN dvbsx_blindscan_setsinglecable(U8BIT fd, struct dvbsx_singlecable_parameters *psinglecablePara)
{
    int ret = TRUE;

    /*set propty*/
    struct dtv_properties prop;
    struct dtv_property *property = NULL;
    int num = 6;

    property = STB_MEMGetSysRAM(num * sizeof(struct dtv_property));
    if (NULL == property)
        return FALSE;

    prop.num = num;
    prop.props = property;

    /*set singlecable*/
    (property+0)->cmd = DTV_SINGLE_CABLE_VER;
    (property+0)->u.data = psinglecablePara->version;
    (property+1)->cmd = DTV_SINGLE_CABLE_USER_BAND;
    (property+1)->u.data = psinglecablePara->userband;
    (property+2)->cmd = DTV_SINGLE_CABLE_BAND_FRE;
    (property+2)->u.data = psinglecablePara->frequency;
    (property+3)->cmd = DTV_SINGLE_CABLE_BANK;
    (property+3)->u.data = psinglecablePara->bank;
    (property+4)->cmd = DTV_SINGLE_CABLE_UNCOMMITTED;
    (property+4)->u.data = psinglecablePara->uncommitted;
    (property+5)->cmd = DTV_SINGLE_CABLE_COMMITTED;
    (property+5)->u.data = psinglecablePara->committed;

    for (num = 0; num < 6; num++)
    {
        TUN_DBG( "set singlecable num[%d] cmd[%d]data[%d]\r\n", num, (property+num)->cmd, (property+num)->u.data);
    }

    ret = dvb_set_prop(fd, &prop);

    if (!ret)
    {
        TUN_DBG( "set singlecable cmd error\n");
    }

    if (property != NULL)
    {
        STB_MEMFreeSysRAM(property);
        property = NULL;
    }

    return ret;
}

static BOOLEAN dvb_blindscan_scan(U8BIT fd, struct dvbsx_blindscanpara *pbspara)
{
    int ret = TRUE;
#if 0

    if (ioctl(tuner_status[path].frontend_fd, FE_SET_BLINDSCAN, pbspara) < 0)
    {
        TUN_DBG( "ioctl dvb_blindscan_scan failed, error:%s", strerror(errno));
        return FALSE;
    }

#else
    /*set propty*/
    struct dtv_properties prop;
    struct dtv_property *property = NULL;
    int num = 8;

    property = STB_MEMGetSysRAM(num * sizeof(struct dtv_property));
    if(NULL == property)
        return FALSE;

    memset(property, 0, num * sizeof(struct dtv_property));

    prop.num = num;
    prop.props = property;
    /*set min fre*/
    (property+0)->cmd = DTV_BLIND_SCAN_MIN_FRE;
    (property+0)->u.data = pbspara->minfrequency;
    /*set max fre*/
    (property+1)->cmd = DTV_BLIND_SCAN_MAX_FRE;
    (property+1)->u.data = pbspara->maxfrequency;

    /*set min rate*/
    (property+2)->cmd = DTV_BLIND_SCAN_MIN_SRATE;
    (property+2)->u.data = pbspara->minSymbolRate;

    /*set max rate*/
    (property+3)->cmd = DTV_BLIND_SCAN_MAX_SRATE;
    (property+3)->u.data = pbspara->maxSymbolRate;
    /*set fre range*/
    (property+4)->cmd = DTV_BLIND_SCAN_FRE_RANGE;
    (property+4)->u.data = pbspara->frequencyRange;
    /*set fre step*/
    (property+5)->cmd = DTV_BLIND_SCAN_FRE_STEP;
    (property+5)->u.data = pbspara->frequencyStep;
    /*set time out*/
    (property+6)->cmd = DTV_BLIND_SCAN_TIMEOUT;
    (property+6)->u.data = pbspara->timeout;
    /*set start blind scan*/
    (property+7)->cmd = DTV_START_BLIND_SCAN;
    (property+7)->u.data = 0;

    for (num = 0; num < 8; num++)
    {
        TUN_DBG( "set start blind num[%d] cmd[%d]data[%d]\r\n", num, (property+num)->cmd, (property+num)->u.data);
    }

    ret = dvb_set_prop(fd, &prop);

    if (!ret)
    {
        TUN_DBG( "set start blind cmd error\n");
    }

    if (property != NULL)
    {
        STB_MEMFreeSysRAM(property);
        property = NULL;
    }

#endif
    return ret;
}

static BOOLEAN dvb_blindscan_getscanevent(int frontend_fd, struct dvbsx_blindscanevent *pbsevent)
{
    BOOLEAN ret = TRUE;
#if 0

    if (ioctl(tuner_status[path].frontend_fd, FE_GET_BLINDSCANEVENT, pbsevent) < 0)
    {
        TUN_DBG( "ioctl FE_GET_BLINDSCANEVENT failed, error:%s", strerror(errno));
        return FALSE;
    }

#else

    struct dvb_frontend_event event;
    ret = dvb_wait_event(frontend_fd, &event, 200);
    if (TRUE == ret)
    {
        if (event.status&BLINDSCAN_UPDATESTARTFREQ)
        {
            pbsevent->status = BLINDSCAN_UPDATESTARTFREQ;
            pbsevent->u.m_uistartfreq_khz = event.parameters.frequency;
        }
        else if (event.status&BLINDSCAN_UPDATEPROCESS)
        {
            pbsevent->status = BLINDSCAN_UPDATEPROCESS;
            pbsevent->u.m_uiprogress = event.parameters.frequency;
        }
        else if (event.status&BLINDSCAN_UPDATERESULTFREQ)
        {
            pbsevent->status = BLINDSCAN_UPDATERESULTFREQ;
            memcpy(&(pbsevent->u.parameters),
                   &(event.parameters), sizeof(struct dvb_frontend_parameters));
        }
        else
        {
            STB_SPDebugWrite("[%s]: %d  event.status = 0x%x, frequency = %d\n", __FUNCTION__, __LINE__, event.status, event.parameters.frequency);
            pbsevent->status = BLINDSCAN_UPDATERESULT_OTHERS;
        }
    }

#endif
    return ret;
}

static BOOLEAN dvb_blindscan_cancel(U8BIT path)
{
    BOOLEAN ret = TRUE;
#if 0

    if (ioctl(tuner_status[path].frontend_fd, FE_SET_BLINDSCANCANCEl) < 0)
    {
        TUN_DBG( "ioctl FE_SET_BLINDSCANCANCEl failed, error:%s", strerror(errno));
        return FALSE;
    }

#else

    struct dtv_properties prop;
    struct dtv_property property;

    prop.num = 1;
    prop.props = &property;
    /*set min fre*/
    memset(&property, 0, sizeof(property));
    property.cmd = DTV_CANCEL_BLIND_SCAN;
    property.u.data = 0;

    ret = dvb_set_prop(tuner_status[path].frontend_fd, &prop);

    if (!ret)
    {
        TUN_DBG( "set cancel blind scan error\n");
    }

#endif
    return ret;
}

static BOOLEAN dvb_blindscan_continue(U8BIT path)
{
    BOOLEAN ret = TRUE;

    struct dtv_properties prop;
    struct dtv_property property;

    prop.num = 1;
    prop.props = &property;
    /*set min fre*/
    memset(&property, 0, sizeof(property));
    property.cmd = DTV_BLIND_SCAN_STEP_NEXT;
    property.u.data = 0;

    ret = dvb_set_prop(tuner_status[path].frontend_fd, &prop);

    if (!ret)
    {
        TUN_DBG( "set continue blind scan error\n");
    }

    return ret;
}


/**\brief Performs a blind scan operation.*/
static BOOLEAN  AM_FEND_IBlindScanAPI_Start(U8BIT path)
{
    BOOLEAN ret = FALSE;

    pthread_mutex_lock(&tuner_status[path].lock);
    struct dvbsx_singlecable_parameters * psinglecablePara = &(tuner_status[path].bs_setting.singlecablePara);
    if (psinglecablePara && psinglecablePara->version != 0)
    {
        ret = dvbsx_blindscan_setsinglecable(tuner_status[path].frontend_fd, psinglecablePara);
        if (!ret)
        {
            pthread_mutex_unlock(&tuner_status[path].lock);
            return ret;
        }
    }

    struct dvbsx_blindscanpara * pbsPara = &(tuner_status[path].bs_setting.bsPara);
    /*driver need to set in blindscan mode*/
    ret = dvb_blindscan_scan(tuner_status[path].frontend_fd, pbsPara);

    pthread_mutex_unlock(&tuner_status[path].lock);
    return ret;
}

/**\brief Queries the blind scan event.*/
static BOOLEAN  AM_FEND_IBlindScanAPI_GetScanEvent(U8BIT path, struct dvbsx_blindscanevent *pbsevent)
{
    BOOLEAN ret = FALSE;

    pthread_mutex_lock(&tuner_status[path].lock);
    struct dvbsx_blindscanevent * pbsEvent = &(tuner_status[path].bs_setting.bsEvent);

    /*Query the internal blind scan procedure information.*/
    ret = dvb_blindscan_getscanevent(tuner_status[path].frontend_fd, pbsEvent);

    if(!ret)
    {
        ret = FALSE;
        pthread_mutex_unlock(&tuner_status[path].lock);
        return ret;
    }

    memcpy(pbsevent, pbsEvent, sizeof(struct dvbsx_blindscanevent));

    /*update tp info*/
    if(pbsEvent->status == BLINDSCAN_UPDATERESULTFREQ)
    {
        /*now driver return 1 tp*/
        for (U16BIT i = 0; i < tuner_status[path].bs_setting.m_uiChannelCount; i++)
        {
            /* skip it if already existed */
            if (0 == memcmp(&(tuner_status[path].bs_setting.channels[i]),
                            &(pbsEvent->u.parameters),
                            sizeof(struct dvb_frontend_parameters)))
            {
                TUN_INFO("channel freq(%lu) is duplicated the index [%d]\n",
                         pbsEvent->u.parameters.frequency, i);
                pthread_mutex_unlock(&tuner_status[path].lock);
                return ret;
            }
        }

        if (tuner_status[path].bs_setting.m_uiChannelCount == FEND_BS_MAX_CHANNEL) {
            TUN_ERR("channel count(%d) reaches the limit(%d)\n",
                    tuner_status[path].bs_setting.m_uiChannelCount, FEND_BS_MAX_CHANNEL);
            pthread_mutex_unlock(&tuner_status[path].lock);
            return ret;
        }

        memcpy(&(tuner_status[path].bs_setting.channels[tuner_status[path].bs_setting.m_uiChannelCount]),
               &(pbsEvent->u.parameters), sizeof(struct dvb_frontend_parameters));
        tuner_status[path].bs_setting.m_uiChannelCount++;
    }

    pthread_mutex_unlock(&tuner_status[path].lock);
    return ret;
}

/**\brief Stops blind scan process.*/
static BOOLEAN AM_FEND_IBlindScanAPI_Exit(U8BIT path)
{
    BOOLEAN ret = TRUE;

    pthread_mutex_lock(&tuner_status[path].lock);
    /*driver need to set in demod mode*/
    ret = dvb_blindscan_cancel(path);

    usleep(10 * 1000);

    pthread_mutex_unlock(&tuner_status[path].lock);
    return ret;
}

static BOOLEAN AM_FEND_BlindDump(U8BIT path)
{
    BOOLEAN ret = TRUE;
    int i = 0;

    TUN_DBG( "AM_FEND_BlindDump start %d--------------------\n", tuner_status[path].bs_setting.m_uiChannelCount);
    pthread_mutex_lock(&tuner_status[path].lock);

    for(i = 0; i < (tuner_status[path].bs_setting.m_uiChannelCount); i++)
    {
        TUN_DBG( "num:%d freq:%d symb:%d\n", i, tuner_status[path].bs_setting.channels[i].frequency, tuner_status[path].bs_setting.channels[i].u.qpsk.symbol_rate);
    }

    pthread_mutex_unlock(&tuner_status[path].lock);
    TUN_DBG( "AM_FEND_BlindDump end--------------------\n");

    return ret;
}

static void* fend_blindscan_thread(void *arg)
{
    U8BIT path = (long)arg;
    struct dvbsx_blindscanevent cur_bsevent;
    E_STB_TUNE_BlindEvent_t evt;
    BOOLEAN ret = FALSE;
    unsigned short index = 0;
    enum DVB_BlindScanAPI_Status BS_Status = DVB_BS_Status_Init;
    U8BIT wait_reports = 0;
    memset(&evt, 0, sizeof(E_STB_TUNE_BlindEvent_t));
    memset(&cur_bsevent, 0, sizeof(cur_bsevent));
    while (BS_Status != DVB_BS_Status_Exit)
    {
        if (!tuner_status[path].enable_blindscan_thread)
        {
            BS_Status = DVB_BS_Status_Cancel;
        }

        switch(BS_Status)
        {
            case DVB_BS_Status_Init:
            {
                BS_Status = DVB_BS_Status_Start;
                TUN_DBG( "fend_blindscan_thread %d", DVB_BS_Status_Init);
                break;
            }

            case DVB_BS_Status_Start:
            {
                ret = AM_FEND_IBlindScanAPI_Start(path);
                TUN_DBG( "fend_blindscan_thread AM_FEND_IBlindScanAPI_Start %d", ret);

                if(!ret)
                {
                    BS_Status = DVB_BS_Status_Exit;
                    if(tuner_status[path].blindscan_cb)
                    {
                        evt.status = AM_FEND_BLIND_START_FAILED;
                        tuner_status[path].blindscan_cb(path, &evt, tuner_status[path].blindscan_cb_user_data);
                    }
                }
                else
                {
                    BS_Status = DVB_BS_Status_Wait;
                }

                break;
            }

            case DVB_BS_Status_Wait:
            {
                ret = AM_FEND_IBlindScanAPI_GetScanEvent(path, &cur_bsevent);
                TUN_DBG( "fend_blindscan_thread AM_FEND_IBlindScanAPI_GetScanEvent %d", ret);

                if(ret)
                {
                    BS_Status = DVB_BS_Status_User_Process;
                }
                else
                {
                    BS_Status = DVB_BS_Status_Wait;

                    wait_reports++;
                    // to avoid wait event reporting frequently
                    if(wait_reports == 5 && tuner_status[path].blindscan_cb)
                    {
                        evt.status = AM_FEND_BLIND_WAIT;
                        tuner_status[path].blindscan_cb(path, &evt, tuner_status[path].blindscan_cb_user_data);
                        wait_reports = 0;
                    }
                }

                break;
            }

            case DVB_BS_Status_User_Process:
            {
                /*
                ------------Custom code start-------------------
                customer can add the callback function here such as adding TP information to TP list or lock the TP for parsing PSI
                Add custom code here; Following code is an example
                */
                TUN_DBG( "fend_blindscan_thread custom cb");

                if(tuner_status[path].blindscan_cb)
                {
                    if(cur_bsevent.status == BLINDSCAN_UPDATESTARTFREQ)
                    {
                        TUN_DBG( "adp start freq %d\n", cur_bsevent.u.m_uistartfreq_khz);
                        evt.freq = cur_bsevent.u.m_uistartfreq_khz;

                        evt.status = AM_FEND_BLIND_START;
                        tuner_status[path].blindscan_cb(path, &evt, tuner_status[path].blindscan_cb_user_data);
                    }
                    else if(cur_bsevent.status == BLINDSCAN_UPDATEPROCESS)
                    {
                        TUN_DBG( "adp process %d\n", cur_bsevent.u.m_uiprogress);
                        evt.process = cur_bsevent.u.m_uiprogress;

                        evt.status = AM_FEND_BLIND_UPDATEPROCESS;
                        tuner_status[path].blindscan_cb(path, &evt, tuner_status[path].blindscan_cb_user_data);
                    }
                    else if(cur_bsevent.status == BLINDSCAN_UPDATERESULTFREQ)
                    {
                        TUN_DBG( "adp result freq %d symb %d\n", cur_bsevent.u.parameters.frequency, cur_bsevent.u.parameters.u.qpsk.symbol_rate);

                        evt.status = AM_FEND_BLIND_UPDATETP;
                        evt.freq = cur_bsevent.u.parameters.frequency;
                        evt.srate = cur_bsevent.u.parameters.u.qpsk.symbol_rate;
                        tuner_status[path].blindscan_cb(path, &evt, tuner_status[path].blindscan_cb_user_data);
                    }
                }

                /*------------Custom code end -------------------*/
                if(cur_bsevent.status == BLINDSCAN_UPDATESTARTFREQ)
                {
                    BS_Status = DVB_BS_Status_Wait;
                }
                else if(cur_bsevent.status == BLINDSCAN_UPDATEPROCESS)
                {
                    if (evt.process < 100)
                        BS_Status = DVB_BS_Status_Wait;
                    else
                        BS_Status = DVB_BS_Status_WaitExit;
                }
                else if(cur_bsevent.status == BLINDSCAN_UPDATERESULTFREQ)
                {
                    BS_Status = DVB_BS_Status_Wait;
                }
                else if(cur_bsevent.status == BLINDSCAN_UPDATERESULT_OTHERS)
                {
                    TUN_DBG( "adp result event ERROR\n");
                    BS_Status = DVB_BS_Status_Wait;
                }

                break;
            }

            case DVB_BS_Status_WaitExit:
            {
                usleep(50*1000);
                break;
            }

            case DVB_BS_Status_Cancel:
            {
                AM_FEND_BlindDump(path);

                ret = AM_FEND_IBlindScanAPI_Exit(path);
                if(FALSE == ret)
                {
                    TUN_DBG( "AM_FEND_IBlindScanAPI_Exit error");
                }
                BS_Status = DVB_BS_Status_Exit;

                TUN_DBG( "AM_FEND_IBlindScanAPI_Exit");
                break;
            }

            default:
            {
                BS_Status = DVB_BS_Status_Cancel;
                break;
            }
        }
    }

    return NULL;
}

static BOOLEAN GetRealParamFromDriver(U8BIT path)
{
    U32BIT srate = 0;
    E_STB_TUNE_CMODE cmode = TUNE_MODE_QAM_UNDEFINED;
    struct dtv_property cmd;
    struct dtv_properties props;
    U_STB_DEMO_CAPABILITY uCap;

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD)
                           && IsTunerLocked(&tuner_status[path]))
    {
        if ((TRUE == STB_GetDemoCapabilityByType(TUNE_SIGNAL_QAM, &uCap)) && (1 ==uCap.dvbc.symbol_rate_auto))
        {
            memset(&cmd, 0, sizeof(struct dtv_property));

            cmd.cmd = DTV_DELIVERY_SYSTEM;
            props.num = 1;
            props.props = &cmd;

            if (ioctl(tuner_status[path].frontend_fd, FE_GET_PROPERTY, &props) >= 0)
            {
                if (SYMBOL_RATE_AUTO == cmd.reserved[1])
                {
                    return FALSE;
                }
                else
                {
                    switch (cmd.reserved[0])
                    {
                        case QAM_16:
                            cmode = TUNE_MODE_QAM_16;
                            break;
                        case QAM_32:
                            cmode = TUNE_MODE_QAM_32;
                            break;
                        case QAM_64:
                            cmode = TUNE_MODE_QAM_64;
                            break;
                        case QAM_128:
                            cmode = TUNE_MODE_QAM_128;
                            break;
                        case QAM_256:
                            cmode = TUNE_MODE_QAM_256;
                            break;
                        default:
                            cmode = TUNE_MODE_QAM_UNDEFINED;
                            break;
                    }
                    srate = cmd.reserved[1];
                    TUN_DBG("%u: from driver:", path);
                    TUN_DBG("%u: symbol rate = %lu", path, srate);
                    if (tuner_status[path].signal_type == TUNE_SIGNAL_QAM)
                    {
                        TUN_DBG("%u: cable mode = %lu", path, cmd.reserved[0]);
                    }
                }
                real_srate = srate;
                real_cmode = cmode;
                return TRUE;
            }
            else
            {
                TUN_ERR("%u: Failed to get real param from driver, errno %d", path, errno);
                return FALSE;
            }
        }
    }
    return FALSE;
}

E_TUNER_EVENT STB_TuneGetLockStatus(U8BIT path)
{
    E_TUNER_EVENT tuner_event = TUNER_STATE_UNKNOW;

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        tuner_event = GetTunerLockStatus(tuner_status[path].frontend_fd);
    }

    //TUN_INFO("tuner_event:%u", tuner_event);

    return tuner_event;
}

int STB_TuneGetEwbsFlag(U8BIT path)
{
    int retval = 0;
    struct dtv_property cmd;
    struct dtv_properties props;
    memset(&cmd, 0, sizeof(struct dtv_property));
    cmd.cmd = DTV_ISDBT_PARTIAL_RECEPTION;
    props.num = 1;
    props.props = &cmd;
    if (ioctl(tuner_status[path].frontend_fd, FE_GET_PROPERTY, &props) >= 0)
    {
        retval = (int)cmd.u.buffer.reserved1[0];
        TUN_DBG("path,sysid=%d", path, retval);
        retval = (int)cmd.u.buffer.reserved1[1];
        TUN_DBG("path=%d,ewbsflag=%d)", path, retval);
    }
    return(retval);
}
