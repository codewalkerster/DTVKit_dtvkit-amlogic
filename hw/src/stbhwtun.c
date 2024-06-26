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
 * If you or your organization is not a member of DTVKit then you have access
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
#include <ctype.h>
#include <sys/types.h>
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


#include "stbhwdef.h"
#include "stbhwtun.h"
#include "stbhwtun_inner.h"
#include "stbhwtun_ex.h"
#include "stbhwmem.h"
#include "stbhwos.h"
#include "stbhwresm.h"
#include "stbhwc.h"
#include "stbhwini.h"
#include "stbhwutils.h"
#include "stb_utils.h"

#include "emu_internal.h"

#include "aml_frontend_api.h"

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
static E_STB_TUNE_BlindEvent_t cur_evt;


/*---local function prototypes for this file---------------------------------*/
static BOOLEAN OpenTuner(S_TUNER_STATUS *tstatus);
static void CloseTuner(S_TUNER_STATUS *tstatus);
/*static*/ BOOLEAN StartTune(S_TUNER_STATUS *tstatus);
static BOOLEAN IsTunerLocked(S_TUNER_STATUS *tstatus);
static BOOLEAN IsTuningParameterMatched(S_TUNER_STATUS *tstatus, struct dvb_frontend_event event);
static void* TunerTask(void *param);
static void ClearTuner(S_TUNER_STATUS *tstatus);
static void ConvertToLowercase(char *str);
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
static fe_delivery_system_t SysTypeToFeMode(E_STB_TUNE_SYSTEM_TYPE sys_type);
/*static*/ BOOLEAN SetFeProperty(int fe_fd, E_STB_TUNE_SYSTEM_TYPE tuned_sys_type);
static E_STB_TUNE_MODULATION GetTuneModulation(enum fe_modulation modulation);
static E_STB_TUNE_TCODERATE TuneGetActualTerrCodeRate(U8BIT path);
static BOOLEAN STB_TuneSetTone(U8BIT path, BOOLEAN use_22khz);
static void STB_TuneSetVoltageInterface(U8BIT path, E_STB_TUNE_LNB_VOLTAGE vol);
static BOOLEAN GetRealParamFromDriver(U8BIT path);
static void TuneStopTuner(S_TUNER_STATUS *tstatus);

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialises the tuner component
 * @param   paths number of tuning paths to initialise
 */
void STB_TuneInitialise(U8BIT paths)
{
    int fe_fd = INVALID_FD;
    BOOLEAN adapter_found;
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
    //CERT_Log_StartingUp("Current isTvPlatform [%s].", isTvPlatform ? "Yes": "No");

    /* Find out how many tuners are available */
    do
    {
        for (num_paths = 0, adapter_found = TRUE; adapter_found && (num_paths < AML_MAX_TUNER_NUM); )
        {
            TUN_DBG("[Path %u] Before get from driver, ts_input_idx=%u, signal_types=%u, support_dvbt2=%u, support_dvbt2=%u",
                    num_paths,
                    aml_hw_cfg.tuners[num_paths].ts_input_idx,
                    aml_hw_cfg.tuners[num_paths].signal_types,
                    aml_hw_cfg.tuners[num_paths].support_dvbt2,
                    aml_hw_cfg.tuners[num_paths].support_dvbs2);

            fe_fd = aml_frontend_open_tuner(num_paths);
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

                aml_frontend_close_tuner(fe_fd);
                fe_fd = INVALID_FD;

                num_paths++;
            }
            else
            {
                //CERT_Log_StartingUp("not found %s", fe_name);
                adapter_found = FALSE;
            }
        }

        if ((!adapter_found) && (init_try_count < 10) && (num_paths == 0))
        {
            init_try_count++;
            sleep(1);

            TUN_DBG("retry: %d", init_try_count);
            //CERT_Log_StartingUp("retry: %d", init_try_count);
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
            for (U8BIT i = 0; i != num_paths; i++)
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
                tuner_status[i].lock = STB_OSCreateMutex();

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
                        //CERT_Log_StartingUp("Failed to create task for tuner %u", i);
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
        //CERT_Log_StartingUp("No tuners found!");
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
        U32BIT index = 0;
        if (aml_frontend_get_tsinput(frontend_fd, &index))
        {
            aml_hw_cfg.tuners[path].ori_tsinput_idx = index;
            aml_hw_cfg.tuners[path].ts_input_idx = index;
            TUN_DBG("%u: ts_input_idx=%u", path, index);
        }
        else
        {
            TUN_ERR("%u: Failed to read ts_input from driver", path);
        }
    }
    else
    {
        TUN_DBG("path=%u, num_paths=%u, aml_hw_cfg.tuner_num=%u, frontend_fd=%d",
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
    FUNCTION_START(STB_TuneSetActualSupportedSystemType);

    if (frontend_fd != INVALID_FD)
    {
        U8BIT data[32];
        U32BIT len = 0;
        memset(data, 0, sizeof(data));

        if (aml_frontend_get_support_delivery_system_list(frontend_fd, data, &len))
        {
            aml_hw_cfg.tuners[path].signal_types  = 0;
            aml_hw_cfg.tuners[path].support_dvbt2 = 0;
            aml_hw_cfg.tuners[path].support_dvbs2 = 0;
            for (U32BIT i = 0; i < len; i++)
            {
                fe_delivery_system_t delsys = data[i];
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
                    case SYS_ATSC:
                        aml_hw_cfg.tuners[path].signal_types |= TUNE_SIGNAL_VSB;
                        break;
                    case SYS_DVBC_ANNEX_B:
                        aml_hw_cfg.tuners[path].signal_types |= TUNE_SIGNAL_QAMB;
                        break;
                    default:
                        break;
                }
                TUN_DBG("%u: supported delsys[%u]=%u", path, i, data[i]);
            }
        }
        else
        {
            TUN_ERR("%u: Failed to read ts_input from driver", path);
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
        STB_OSMutexLock(tstatus->lock);
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
                    TuneStopTuner(tstatus);

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

        STB_OSMutexUnlock(tstatus->lock);
    }

    FUNCTION_FINISH(STB_TuneSetSignalType);
}

static fe_delivery_system_t SysTypeToFeMode(E_STB_TUNE_SYSTEM_TYPE sys_type)
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

        case TUNE_SYSTEM_TYPE_ATSC_T:
            fe_mode = SYS_ATSC;
            break;

        case TUNE_SYSTEM_TYPE_ATSC_C:
            fe_mode = SYS_DVBC_ANNEX_B;
            break;
        default:
            TUN_ERR("not support type:%d", sys_type);
            break;
    }

    return fe_mode;
}

/*static*/ BOOLEAN SetFeProperty(int fe_fd, E_STB_TUNE_SYSTEM_TYPE tuned_sys_type)
{
    if (fe_fd == INVALID_FD)
        return FALSE;

    fe_delivery_system_t fe_mode = SysTypeToFeMode(tuned_sys_type);
    if (fe_mode == SYS_UNDEFINED)
        return FALSE;

    return aml_frontend_set_fe_property(fe_fd, fe_mode);
}

static E_STB_TUNE_MODULATION GetTuneModulation(fe_modulation_t modulation)
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

        TUN_DBG("%u: Stopping tuning...", tstatus->path);

        if (HW_ISDB_SYSTEM == STB_HWGetDtvSystem())
        {
            stb_tune_stop_tuner(tstatus);
        }

        if (state != TUNER_IDLE && state != TUNER_EXITED)
        {
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

        if ((tstatus->signal_type == TUNE_SIGNAL_COFDM &&
               (tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT ||
               (tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2 && tstatus->delivery_system == SYS_DVBT2))) ||
             (tstatus->signal_type == TUNE_SIGNAL_QPSK &&
                (tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS ||
                (tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS2 && tstatus->delivery_system == SYS_DVBS2))) ||
             (tstatus->signal_type == TUNE_SIGNAL_QAM && tstatus->delivery_system == SYS_DVBC_ANNEX_A) ||
             (tstatus->signal_type == TUNE_SIGNAL_ISDBT && tstatus->delivery_system == SYS_ISDBT) ||
             (tstatus->signal_type == TUNE_SIGNAL_VSB && tstatus->delivery_system == SYS_ATSC) ||
             (tstatus->signal_type == TUNE_SIGNAL_QAMB && tstatus->delivery_system == SYS_DVBC_ANNEX_B))
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
                case TUNE_SIGNAL_QAMB:
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
                case TUNE_SIGNAL_VSB:
                    if (tstatus->u.terr.tmode != tmode)
                    {
                        start_tuning = TRUE;
                        tstatus->u.terr.tmode = tmode;
                    }
                    break;

                default:
                    break;
            }

            STB_OSMutexLock(tstatus->mutex);
            state = tstatus->state;
            STB_OSMutexUnlock(tstatus->mutex);

            STB_OSMutexLock(tstatus->lock); //Keep serial operation that tune on the same path.

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
                #ifdef EMUTUNNER_ENABLE
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

            STB_OSMutexUnlock(tstatus->lock);
        }
        else
        {
            TUN_ERR("%u: system type %u not supported", tstatus->path, tstatus->sys_type);

            STB_OSMutexLock(tstatus->mutex);
            state = tstatus->state;
            STB_OSMutexUnlock(tstatus->mutex);

            if (state != TUNER_IDLE && state != TUNER_EXITED)
            {
                STB_OSMutexLock(tstatus->lock);
                TuneStopTuner(tstatus);
                STB_OSMutexUnlock(tstatus->lock);
            }

            STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path, sizeof(U8BIT));
        }
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
        tstatus = &tuner_status[path];

        #ifdef EMUTUNNER_ENABLE
        STB_OSMutexLock(tstatus->mutex);
        EmuTunerStop(path);
        STB_OSMutexUnlock(tstatus->mutex);
        #endif

        STB_OSMutexLock(tstatus->lock);
        TuneStopTuner(tstatus);
        STB_OSMutexUnlock(tstatus->lock);
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

static BOOLEAN IsExternalDemod(U8BIT path)
{
    BOOLEAN retval = FALSE;
    if (NULL != strstr(tuner_status[path].fe_info.name, "cxd2856"))
    {
       retval = TRUE;  //For cxd2856, not need to convert to percentage
    }

    return retval;
}

BOOLEAN STB_TuneGetSignalInfo(U8BIT path, S_STB_TUNE_SIGNAL_INFO* signal_info)
{
    if (signal_info == NULL)
    {
        TUN_ERR("Invalid signal_info");
        return FALSE;
    }

#ifdef EMUTUNNER_ENABLE
    signal_info->ssi  = EmuTunerGetSignalStrength(path);
    signal_info->sqi  = EmuTunerGetSignalQuality(path);
    if (signal_info->ssi > 0 || signal_info->sqi > 0)
    {
        return TRUE;
    }
#endif

    if (path >= num_paths)
    {
        TUN_ERR("%u: Invalid path", path);
        return FALSE;
    }

    S32BIT frontend_fd = (S32BIT)tuner_status[path].frontend_fd;
    if (frontend_fd == INVALID_FD)
    {
        TUN_ERR("%u: Invalid frontend_fd %d", path, frontend_fd);
        return FALSE;
    }

    S16BIT strength = -100; //dBm
    if (!aml_frontend_get_signal_strength(frontend_fd, (U16BIT *)&strength))
    {
        TUN_ERR("%u: Failed to get signal strength", path);
    }

    S16BIT dBuV = 0;
    S16BIT dBmV = 0;
    S16BIT ssi = 0;
    if (IsExternalDemod(path))
    {
        // dBmV(x1000) for external demod
        if (!aml_frontend_get_signal_strength_property(frontend_fd, (U16BIT *)&ssi, (U16BIT *)&dBmV))
        {
            TUN_ERR("%u: Failed to get signal dBm for external demd", path);
        }
        dBuV = 0; // not required for external demod currently
    }
    else
    {
        dBuV = 109 + strength; // plus 108.75(dBm to dBuV for 75 ohms)
        dBmV = 49 + strength;  // plus 48.75(dBm to dBmV for 75 ohms)
        ssi = (U16BIT)STB_Utils_StrengthToSSI(path, strength);
    }

    U32BIT ber = 0;
    if (!aml_frontend_get_signal_ber(frontend_fd, &ber))
    {
        TUN_ERR("%u: Failed to get signal ber", path);
    }

    S16BIT snr = 0;
    if (!aml_frontend_get_signal_snr(frontend_fd, (U16BIT *)&snr))
    {
        TUN_ERR("%u: Failed to get signal snr", path);
    }

    S16BIT sqi = 0;
    if (IsExternalDemod(path))
    {
        sqi = snr; // SQI is equal to SNR for external demod
    }
    else
    {
        sqi = (S16BIT)STB_Utils_SNR10ToSQI(path, snr);
    }

    if (!IsTunerLocked(&tuner_status[path]))
    {
        // SSI&SQI is set to 0 if unlock
        ssi = 0;
        sqi = 0;
    }

    signal_info->strength = strength;
    signal_info->dBuV = dBuV;
    signal_info->dBmV = dBmV;
    signal_info->snr  = snr;
    signal_info->ber  = ber;
    signal_info->ssi  = ssi;
    signal_info->sqi  = sqi;

    TUN_INFO("%u: Signal Strength=%d(dBm) dBuV=%d dBmV=%d SNR=%d.%d BER=%d(e-10) SSI=%d%% SQI=%d%%",
             path,
             signal_info->strength,
             signal_info->dBuV,
             signal_info->dBmV,
             signal_info->snr/10, signal_info->snr%10,
             signal_info->ber,
             signal_info->ssi,
             signal_info->sqi);

    return TRUE;
}

/**
 * @brief   Returns the actual frequency of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the frequency in Hz
 */
U32BIT STB_TuneGetActualTerrFrequency(U8BIT path)
{
    U32BIT freq = 0;

    FUNCTION_START(STB_TuneGetActualTerrFrequency);

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        if (IsTunerLocked(&tuner_status[path]))
        {
            if (aml_frontend_get_frequency(tuner_status[path].frontend_fd, &freq))
            {
                TUN_DBG("%u: freq=%lu", path, freq);
            }
            else
            {
                freq = 0;
                TUN_ERR("%u: failed to read frequency", path);
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
    S8BIT offset = 0;

    FUNCTION_START(STB_TuneGetActualTerrFreqOffset);

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        if (IsTunerLocked(&tuner_status[path]))
        {
            U32BIT freq = 0;
            if (aml_frontend_get_frequency(tuner_status[path].frontend_fd, &freq))
            {
                offset = freq - tuner_status[path].freq;
                TUN_DBG("%u: freq=%u, actual=%u, offset=%d", path,
                        tuner_status[path].freq, freq, offset);
            }
            else
            {
                TUN_ERR("%u: failed to read frequency", path);
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
    E_STB_TUNE_TMODE mode = TUNE_MODE_COFDM_UNDEFINED;

    FUNCTION_START(STB_TuneGetActualTerrMode);

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        if (IsTunerLocked(&tuner_status[path]))
        {
            fe_transmit_mode_t tmode = TRANSMISSION_MODE_AUTO;
            if (aml_frontend_get_transmission_mode(tuner_status[path].frontend_fd, (U32BIT *)&tmode))
            {
                TUN_DBG("%u: mode=%u", path, tmode);

                switch (tmode)
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
                TUN_ERR("%u: Failed to read transmission mode", path);
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
    E_STB_TUNE_TCONST t_modu = TUNE_TCONST_UNDEFINED;

    FUNCTION_START(STB_TuneGetActualTerrConstellation);

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        fe_delivery_system_t fe_sys = SYS_UNDEFINED;
        fe_modulation_t fe_modu = QPSK;
        if (aml_frontend_get_terr_constellation(tuner_status[path].frontend_fd,
                                                (U32BIT *)&fe_sys,
                                                (U32BIT *)&fe_modu))
        {
            TUN_DBG("%u: fe_sys=%u fe_modu=%u", path, fe_sys, fe_modu);
            if (fe_sys == SYS_DVBT2 || fe_sys == SYS_DVBT)
            {
                switch (fe_modu)
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
            TUN_ERR("%u: Failed to read terrestrial constellation", path);
        }
    }

    FUNCTION_FINISH(STB_TuneGetActualTerrConstellation);

    return t_modu;
}

/**
 * @brief   Returns the heirarchy of the current terrestrial signal
 * @param   tuner_id, the tuner index to query
 * @return  the hierarchy
 */
E_STB_TUNE_HIERARCHY STB_TuneGetActualTerrHierarchy(U8BIT tuner_id)
{
    U16BIT retval = TUNE_HIERARCHY_NONE;
    U8BIT plp_ids[MAX_PLP_NUMBER];

    FUNCTION_START(STB_TuneGetActualTerrHierarchy);

    if (tuner_status[tuner_id].sys_type == TUNE_SYSTEM_TYPE_DVBT2)
    {
        retval = STB_TuneGetMPLPIDList(tuner_id, plp_ids, MAX_PLP_NUMBER);
        if (retval != 0)
        {
            retval--;
        }
    }
    else // TUNE_SYSTEM_TYPE_DVBT
    {
        if (tuner_id < num_paths)
        {
            if (tuner_status[tuner_id].frontend_fd != INVALID_FD &&
                GetTunerLockStatus(tuner_status[tuner_id].frontend_fd) == TUNER_STATE_LOCKED)
            {
                fe_hierarchy_t hierarchy = HIERARCHY_NONE;
                if (aml_frontend_get_terr_hierarchy(tuner_status[tuner_id].frontend_fd, (U32BIT *)&hierarchy))
                {
                    TUN_DBG("%u: hierarchy=%lu", tuner_id, hierarchy);

                    switch (hierarchy)
                    {
                        case HIERARCHY_NONE:
                            retval = TUNE_HIERARCHY_NONE;
                            break;

                        case HIERARCHY_1:
                            retval = TUNE_HIERARCHY_1;
                            break;

                        case HIERARCHY_2:
                            retval = TUNE_HIERARCHY_2;
                            break;

                        case HIERARCHY_4:
                            retval = TUNE_HIERARCHY_4;
                            break;

                        default:
                            retval = TUNE_HIERARCHY_NONE;
                            break;
                    }
                }
                else
                {
                    TUN_ERR("%u: Failed to get hierarchy", tuner_id);
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
U16BIT STB_TuneGetMPLPIDList(U8BIT tuner_id, U8BIT *plp_list, U16BIT listlen)
{
    U16BIT retval = 0;
    U8BIT plp_ids[MAX_PLP_NUMBER];
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
                    U32BIT plp_list_num = 0;
                    if (aml_frontend_get_dvbt2_plp_id_list(tuner_status[tuner_id].frontend_fd,
                                                           MAX_PLP_NUMBER, plp_ids, &plp_list_num))
                    {
                        retval = (U16BIT)plp_list_num;
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
                        TUN_ERR("%u: Failed to get number of PLPs", tuner_id);
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
        TUN_ERR("%u: Failed to get mplp list", tuner_id);
    }

    FUNCTION_FINISH(STB_TuneGetMPLPIDList);

    return retval;
}

static E_STB_TUNE_TCODERATE TuneGetActualTerrCodeRate(U8BIT path)
{
    E_STB_TUNE_TCODERATE t_rc = TUNE_TCODERATE_UNDEFINED;

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        fe_delivery_system_t fe_sys = SYS_UNDEFINED;
        fe_code_rate_t fe_cr = FEC_NONE;
        if (aml_frontend_get_terr_coderate(tuner_status[path].frontend_fd,
                                           (U32BIT *)&fe_sys,
                                           (U32BIT *)&fe_cr))
        {
            if (fe_sys == SYS_DVBT2 || fe_sys == SYS_DVBT)
            {
                switch (fe_cr)
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
            TUN_ERR("%u: Failed to read terrestrial coderate", path);
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
    U16BIT cell_id = 0xFFFF;

    FUNCTION_START(STB_TuneGetActualTerrCellId);

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        fe_delivery_system_t fe_sys = SYS_UNDEFINED;
        U16BIT id = 0xFFFF;
        if (aml_frontend_get_terr_cellid(tuner_status[path].frontend_fd,
                                         (U32BIT *)&fe_sys,
                                         (U32BIT *)&id))
        {
            if (fe_sys == SYS_DVBT2 || fe_sys == SYS_DVBT)
            {
                cell_id = id;
                TUN_DBG("%u: cell_id=0x%04X", path, cell_id);
            }
        }
        else
        {
            TUN_ERR("%u: Failed to read terrestrial cell id", path);
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
void STB_TuneSetLNBVoltage(U8BIT path, E_STB_TUNE_LNB_VOLTAGE voltage, BOOLEAN retune, E_TUNER_SETTING_MODE mode)
{
    FUNCTION_START(STB_TuneSetLNBVoltage);

    if (path < num_paths)
    {
        if (tuner_status[path].u.sat.lnb_voltage != voltage)
        {
            tuner_status[path].u.sat.lnb_voltage = voltage;
            if (retune)
            {
                tuner_status[path].tuning_params_changed = TRUE;
            }
        }

        if (mode == MODE_IMMEDIATE)
        {
            STB_TuneSetVoltageInterface(path, voltage);
        }
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
    fe_delivery_system_t fe_sys = SYS_UNDEFINED;
    fe_modulation_t modulation = QPSK;
    U32BIT srate = 0;
    if (aml_frontend_get_delivery_system(tuner_status[path].frontend_fd,
                                         (U32BIT *)&fe_sys, (U32BIT *)&modulation, &srate))
    {
        if (fe_sys == SYS_DVBT)
        {
            sys_type = TUNE_SYSTEM_TYPE_DVBT;
        }
        else if (fe_sys == SYS_DVBT2)
        {
            sys_type = TUNE_SYSTEM_TYPE_DVBT2;
        }
        else if (fe_sys == SYS_DVBS)
        {
            sys_type = TUNE_SYSTEM_TYPE_DVBS;
        }
        else if (fe_sys == SYS_DVBS2)
        {
            sys_type = TUNE_SYSTEM_TYPE_DVBS2;
        }
        else if (fe_sys == SYS_ATSC)
        {
            sys_type = TUNE_SYSTEM_TYPE_ATSC_T;
        }
        else if (fe_sys == SYS_DVBC_ANNEX_B)
        {
            sys_type = TUNE_SYSTEM_TYPE_ATSC_C;
        }
        // loop more
    }
    else
    {
        TUN_ERR("%u: Failed to read actual fe system", path);
    }

    TUN_DBG("path:%u fd:%d fe_sys:%u sys_type:%u", path, tuner_status[path].frontend_fd, fe_sys, sys_type);

    return sys_type;
}


static void STB_TuneSetVoltageInterface(U8BIT path, E_STB_TUNE_LNB_VOLTAGE vol)
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
    if (!aml_frontend_set_voltage(tuner_status[path].frontend_fd, voltage))
    {
        TUN_ERR("%u: Failed to set voltage", path);
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
void STB_TuneSet22kState(U8BIT path, BOOLEAN state, BOOLEAN retune, E_TUNER_SETTING_MODE mode)
{
    FUNCTION_START(STB_TuneSet22kState);

    if (path < num_paths)
    {
        if (tuner_status[path].u.sat.use_22khz != state)
        {
            tuner_status[path].u.sat.use_22khz = state;
            if (retune)
            {
                tuner_status[path].tuning_params_changed = TRUE;
            }
        }

        if (mode == MODE_IMMEDIATE)
        {
            STB_TuneSetTone(path, state);
        }
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

    if (!aml_frontend_master_send_diseqc_cmd(tuner_status[path].frontend_fd, data, size))
    {
        TUN_ERR("%u: Failed to send diseqc cmd", path);
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
void STB_TuneReceiveDISEQCReply(U8BIT path, U8BIT *data, U8BIT data_size, U32BIT timeout)
{
    FUNCTION_START(STB_TuneReceiveDISEQCReply);

    if (!aml_frontend_slave_receive_diseqc_reply(tuner_status[path].frontend_fd, data, data_size, timeout))
    {
        TUN_ERR("%u: Failed to receive diseqc reply", path);
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
    if (!aml_frontend_send_diseqc_burst(tuner_status[path].frontend_fd, data))
    {
        TUN_ERR("%u: Failed to send busrt msg", path);
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
void STB_TuneSetLOFrequency(U8BIT path, S32BIT lo_freq)
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

    FUNCTION_FINISH(STB_TuneSetSystemType);
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
        if (tuner_status[path].signal_type == TUNE_SIGNAL_QAM ||
            tuner_status[path].signal_type == TUNE_SIGNAL_QAMB)
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

    if ((path < num_paths) &&
        (tuner_status[path].signal_type == TUNE_SIGNAL_QAM))
    {
        if (GetRealParamFromDriver(path) == TRUE)
        {
            mode = real_cmode;
        }
        else
        {
            mode = tuner_status[path].u.cab.cmode;
        }
    }
    else if ((path < num_paths) &&
         (tuner_status[path].signal_type == TUNE_SIGNAL_QAMB))
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
       if (aml_hw_cfg.tuners[path].signal_types & TUNE_SIGNAL_QAMB)
       {
          support_sys[TUNE_SYSTEM_TYPE_ATSC_C] = TRUE;
       }
       if (aml_hw_cfg.tuners[path].signal_types & TUNE_SIGNAL_VSB)
       {
          support_sys[TUNE_SYSTEM_TYPE_ATSC_T] = TRUE;
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

        STB_OSMutexLock(tuner_status[path].lock);
        ret = OpenTuner(&tuner_status[path]);
        STB_OSMutexUnlock(tuner_status[path].lock);
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
        STB_OSMutexLock(tuner_status[path].lock);

        if (use)
            tuner_status[path].frontend_usage ++;
        else
            tuner_status[path].frontend_usage --;

        TUN_DBG("%u: fe_useage[%s]: %d", path, use?"Add":"Remove", tuner_status[path].frontend_usage);
        STB_OSMutexUnlock(tuner_status[path].lock);
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
        STB_OSMutexLock(tuner_status[path].lock);
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
        STB_OSMutexUnlock(tuner_status[path].lock);
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

    STB_File_Echo("/sys/module/amlogic_dvb_demux/parameters/cache_clear_time","5000");
    for (i = 0; i != num_paths; i++)
    {
        STB_OSMutexLock(tuner_status[i].lock);

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
        STB_OSMutexUnlock(tuner_status[i].lock);
    }
}

void STB_TuneAllStop()
{
    U8BIT i = 0;
    E_TUNER_STATE state;

    if (STB_TuneIsTvPlatform())
    {
        STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_RESOURCE_BUSY, NULL, 0);
    }

    for (i = 0; i != num_paths; i++)
    {
        STB_OSMutexLock(tuner_status[i].lock);

        if (tuner_status[i].frontend_fd != INVALID_FD)
        {
            STB_OSMutexLock(tuner_status[i].mutex);
            state = tuner_status[i].state;
            STB_OSMutexUnlock(tuner_status[i].mutex);

            if (state != TUNER_IDLE && state != TUNER_EXITED)
            {
                TuneStopTuner(&tuner_status[i]);
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

        STB_OSMutexUnlock(tuner_status[i].lock);
    }
    STB_File_Echo("/sys/module/amlogic_dvb_demux/parameters/cache_clear_time","1000");
}

static BOOLEAN STB_TuneSetTone(U8BIT path, BOOLEAN use_22khz)
{
    if (!aml_frontend_set_tone(tuner_status[path].frontend_fd, use_22khz))
    {
        TUN_ERR("%u: Failed to set tone", path);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN STB_Tune_BlindScan(U8BIT path, E_STB_TUNE_SYSTEM_TYPE sys_type, STB_Tnue_BlindCallback_t cb, void *user_data,
                                 unsigned int start_freq, unsigned int stop_freq, E_STB_TUNE_BlindUnicable_t unicable)
{
    if (start_freq >= stop_freq)
    {
        TUN_DBG( "AM_FEND_BlindScan start_freq equal stop_freq\n");
        return FALSE;
    }

    /*this function set the parameters blind scan process needed.*/
    SetFeProperty(tuner_status[path].frontend_fd, sys_type);

    memset(&(tuner_status[path].bs_setting), 0, sizeof(struct DVB_BlindScanAPI_Setting));

    if (sys_type == TUNE_SYSTEM_TYPE_DVBS)
    {
        tuner_status[path].bs_setting.bsPara.minfrequency = M_BS_START_FREQ * 1000;         /*Default Set Blind scan start frequency*/
        tuner_status[path].bs_setting.bsPara.maxfrequency = M_BS_STOP_FREQ * 1000;          /*Default Set Blind scan stop frequency*/
        tuner_status[path].bs_setting.bsPara.maxSymbolRate = M_BS_MAX_SYMB * 1000 * 1000;   /*Set MAX symbol rate*/
        tuner_status[path].bs_setting.bsPara.minSymbolRate = M_BS_MIN_SYMB * 1000 * 1000;   /*Set MIN symbol rate*/
        tuner_status[path].bs_setting.bsPara.timeout = FEND_WAIT_TIMEOUT;
        if (start_freq > M_BS_START_FREQ)
        {
            tuner_status[path].bs_setting.bsPara.minfrequency = start_freq * 1000;          /*Change default start frequency*/
        }
        if (stop_freq < M_BS_STOP_FREQ)
        {
            tuner_status[path].bs_setting.bsPara.maxfrequency = stop_freq * 1000;           /*Change default end frequency*/
        }
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
    STB_OSMutexLock(tuner_status[path].lock);

    *count = 0;

    if (tuner_status[path].bs_setting.m_uiChannelCount)
    {
        *count = (unsigned int)(tuner_status[path].bs_setting.m_uiChannelCount);
    }

    STB_OSMutexUnlock(tuner_status[path].lock);

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

    STB_OSMutexLock(tuner_status[path].lock);

    if ((*count) > tuner_status[path].bs_setting.m_uiChannelCount)
    {
        *count = (unsigned int)(tuner_status[path].bs_setting.m_uiChannelCount);
    }

    memcpy(para, tuner_status[path].bs_setting.channels, (*count) * sizeof(struct dvb_frontend_parameters));

    STB_OSMutexUnlock(tuner_status[path].lock);
    return ret;
}

void STB_TuneGetCurrentTPInfo(E_STB_TUNE_BlindEvent_t* evt)
{
    if (evt != NULL)
    {
        evt->freq = cur_evt.freq;
        evt->srate = cur_evt.srate;
        evt->status = cur_evt.status;
    }
}

/*---local function definitions----------------------------------------------*/
static void ConvertToLowercase(char *str) {
    int length = strlen(str);
    for (int i = 0; i < length; ++i)
    {
        str[i] = tolower(str[i]);
    }
}

static BOOLEAN SetSysType(S_TUNER_STATUS *tstatus, E_STB_TUNE_SIGNAL_TYPE sig_type)
{
    BOOLEAN retval;
    BOOLEAN sig_sys_mismatch;
    BOOLEAN tuned_sys_mismatch;
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

            case TUNE_SIGNAL_QAMB:
                if (tstatus->tuned_sys_type != TUNE_SYSTEM_TYPE_ATSC_C)
                    tstatus->tuned_sys_type = TUNE_SYSTEM_TYPE_ATSC_C;

                break;
            case TUNE_SIGNAL_VSB:
                if (tstatus->tuned_sys_type != TUNE_SYSTEM_TYPE_ATSC_T)
                    tstatus->tuned_sys_type = TUNE_SYSTEM_TYPE_ATSC_T;

                break;

            default:
                TUN_ERR("not support sig_type:%d\n", sig_type);
                return retval;
        }

        if (SetFeProperty(tstatus->frontend_fd, tstatus->tuned_sys_type))
        {
            memset(&tstatus->fe_info, 0, sizeof(tstatus->fe_info));

            if (aml_frontend_get_frontend_info(tstatus->frontend_fd, &(tstatus->fe_info)))
            {
                // fe name to lower case
                ConvertToLowercase(tstatus->fe_info.name);

                TUN_DBG("fe_info.type=%d, name=%s", tstatus->fe_info.type, tstatus->fe_info.name);

                if (tstatus->fe_info.type == FE_OFDM)
                {
                    if (tstatus->tuned_sys_type == TUNE_SYSTEM_TYPE_ISDBT)
                    {
                        TUN_DBG("Tuner configured as ISDBT, min_freq=%lu, max_freq=%lu",
                                tstatus->fe_info.frequency_min, tstatus->fe_info.frequency_max);
                        tstatus->signal_type = TUNE_SIGNAL_ISDBT;
                        tstatus->delivery_system = SYS_ISDBT;
                    }
                    else
                    {
                        TUN_DBG("Tuner configured as DVB-T/T2, min_freq=%lu, max_freq=%lu",
                                tstatus->fe_info.frequency_min, tstatus->fe_info.frequency_max);
                        tstatus->signal_type = TUNE_SIGNAL_COFDM;
                        tstatus->delivery_system = SYS_DVBT2;
                    }
                }
                else if (tstatus->fe_info.type == FE_QAM)
                {
                    TUN_DBG("Tuner configured as DVBC, min_freq=%lu, max_freq=%lu",
                            tstatus->fe_info.frequency_min, tstatus->fe_info.frequency_max);
                    tstatus->signal_type = TUNE_SIGNAL_QAM;
                    tstatus->delivery_system = SYS_DVBC_ANNEX_A;
                }
                else if (tstatus->fe_info.type == FE_QPSK)
                {
                    TUN_DBG("Tuner configured as DVB-S/S2, freq min/max=%lu/%lu, symbol rate min/max=%lu/%lu",
                            tstatus->fe_info.frequency_min, tstatus->fe_info.frequency_max,
                            tstatus->fe_info.symbol_rate_min, tstatus->fe_info.symbol_rate_max);
                    tstatus->signal_type = TUNE_SIGNAL_QPSK;
                    tstatus->delivery_system = SYS_DVBS2;
                }
                else if (tstatus->fe_info.type == FE_ATSC)
                {
                    TUN_DBG("Tuner configured as ATSC, min_freq=%lu, max_freq=%lu sig_type=%u",
                            tstatus->fe_info.frequency_min, tstatus->fe_info.frequency_max, sig_type);
                    if (sig_type == TUNE_SIGNAL_QAMB)
                    {
                        tstatus->delivery_system = SYS_DVBC_ANNEX_B;
                    }
                    else
                    {
                        tstatus->delivery_system = SYS_ATSC;
                    }
                }
                else
                {
                    TUN_DBG("Tuner configured as unknown type");
                }

                retval = TRUE;
            }
            else
            {
                TUN_DBG("Failed to get FE_INFO");
            }
        }
        else
        {
            TUN_DBG("Failed to SetFeProperty");
        }
    }

    return(retval);
}

static BOOLEAN OpenTuner(S_TUNER_STATUS *tstatus)
{
    BOOLEAN retval, istv;
    U8BIT tuner_index = 0;
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
        tstatus->frontend_fd = aml_frontend_open_tuner(aml_hw_cfg.tuners[tuner_index].frontend_idx);
        if (tstatus->frontend_fd < 0)
        {
            TUN_ERR("Failed to open tune[%d]", tuner_index);
            retval = FALSE;
        }
        else
        {
            TUN_DBG("Open tune[%d] frontend_fd:%d ", tuner_index, tstatus->frontend_fd);
        }
    }


    return(retval);
}

static void CloseTuner(S_TUNER_STATUS *tstatus)
{
    if ((NULL != tstatus) && (tstatus->frontend_fd != INVALID_FD))
    {
        TUN_DBG("path %u: close tuner frontend_fd:%d", tstatus->path,tstatus->frontend_fd);

        // !!! Not check QPSK since signal type may be set to none before closed
        // if (tstatus->signal_type == TUNE_SIGNAL_QPSK)
        {
            TUN_DBG("path %u: lnb power and 22khz off frontend_fd:%d", tstatus->path,tstatus->frontend_fd);

            // It may cause system issue when in not DVBS mode
            // kernel set power and 22khz off while closing fronend
            STB_TuneSetLNBVoltage(tstatus->path, LNB_VOLTAGE_OFF, FALSE, MODE_DELAYED);
            STB_TuneSet22kState(tstatus->path, FALSE, FALSE, MODE_DELAYED);
        }

        SetFeProperty(tstatus->frontend_fd, TUNE_SYSTEM_TYPE_ANALOG);
        tstatus->signal_type = TUNE_SIGNAL_NONE;
        aml_frontend_close_tuner(tstatus->frontend_fd);
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

    memset(&fe_params, 0, sizeof(struct dvb_frontend_parameters));

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

            if (aml_frontend_set_frontend(tstatus->frontend_fd, &fe_params))
            {
                TUN_DBG("%u: Tuning to %u", tstatus->path, tstatus->freq);
                retval = TRUE;
            }
            else
            {
                TUN_ERR("%u: Unable to set tuning parameters", tstatus->path);
            }

            break;
        }

        case TUNE_SIGNAL_QAMB:
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

            //For ATSC-C, modulation is set to both qam and vsb
            if (tstatus->signal_type == TUNE_SIGNAL_QAMB)
            {
                fe_params.u.vsb.modulation = QAM_AUTO;
            }

            if (aml_frontend_set_frontend(tstatus->frontend_fd, &fe_params))
            {
                TUN_DBG("%u: Tuning to %u", tstatus->path, tstatus->freq);
                retval = TRUE;
            }
            else
            {
                TUN_ERR("%u: Unable to set tuning parameters", tstatus->path);
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

                if (aml_frontend_set_frontend(tstatus->frontend_fd, &fe_params))
                {
                    TUN_DBG("%u: Tuning to %u %u %d", tstatus->path, tstatus->freq, tstatus->u.sat.srate, tstatus->u.sat.fec);
                    retval = TRUE;
                }
                else
                {
                    TUN_ERR("%u: Unable to set tuning parameters", tstatus->path);
                }
            }
            else
            {
                TUN_ERR("%u: Failed to set tone", tstatus->path);
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

            if (aml_frontend_set_frontend(tstatus->frontend_fd, &fe_params))
            {
                TUN_DBG("%u: Tuning to %u", tstatus->path, tstatus->freq);
                retval = TRUE;
            }
            else
            {
                TUN_ERR("%u: Unable to set tuning parameters", tstatus->path);
            }

            break;
        }

        case TUNE_SIGNAL_VSB:
        {
            fe_params.frequency = tstatus->freq;
            fe_params.u.vsb.modulation = tstatus->u.terr.tmode == TUNE_MODE_VSB_16 ? VSB_16 : VSB_8;

            if (aml_frontend_set_frontend(tstatus->frontend_fd, &fe_params))
            {
                TUN_DBG("%u: Tuning to %lu", tstatus->path, tstatus->freq);
                retval = TRUE;
            }
            else
            {
                TUN_ERR("%u: Unable to set tuning parameters", tstatus->path);
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
                STB_OSMutexLock(tstatus->mutex);
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
                        if (aml_frontend_get_event(tstatus->frontend_fd, &fe_event))
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
                        fe_delivery_system_t fe_sys = SYS_UNDEFINED;
                        fe_modulation_t modulation = QPSK;
                        U32BIT srate = 0;
                        if (aml_frontend_get_delivery_system(tstatus->frontend_fd,
                                                             (U32BIT *)&fe_sys, (U32BIT *)&modulation, &srate))
                        {
                            STB_OSMutexLock(tstatus->mutex);

                            if (tstatus->signal_type == TUNE_SIGNAL_QAM ||
                                tstatus->signal_type == TUNE_SIGNAL_QAMB ||
                                tstatus->signal_type == TUNE_SIGNAL_VSB)
                            {
                                TUN_INFO("%u: Keep LOCKED status for %s", tstatus->path,
                                         (tstatus->signal_type == TUNE_SIGNAL_QAM ? "QAM" :
                                          (tstatus->signal_type == TUNE_SIGNAL_QAMB ? "QAMB": "VSB")));
                            }
                            else if ((((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT) && (fe_sys != SYS_DVBT)) ||
                                    ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2) && (fe_sys != SYS_DVBT2))) &&
                                    (tstatus->signal_type != TUNE_SIGNAL_QAM))
                            {
                                locked = FALSE;
                                TUN_INFO("%u: Ignoring LOCKED status for %s, delivery system is %s", tstatus->path,
                                        GetSysTypeDebugString(tstatus->sys_type),
                                        ((fe_sys == SYS_DVBT) ? "DVB-T" : "DVB-T2"));
                            }
                            else if ((SYS_DVBS == fe_sys || SYS_DVBS2 == fe_sys) &&
                                     (TUNE_SYSTEM_TYPE_DVBS == tstatus->sys_type || TUNE_SYSTEM_TYPE_DVBS2 == tstatus->sys_type))
                            {
                                TUN_INFO("[%s:%d] fe_sys:%u, sys_type:%u", __FUNCTION__, __LINE__, fe_sys, tstatus->sys_type);

                                if (SYS_DVBS == fe_sys)
                                    tstatus->sys_type = TUNE_SYSTEM_TYPE_DVBS;
                                else
                                    tstatus->sys_type = TUNE_SYSTEM_TYPE_DVBS2;

                                // reserved[0] is used for modulation in demod
                                tstatus->u.sat.modulation = GetTuneModulation(modulation);
                                TUN_INFO("[%s:%d] modulation:%u, sat.modulation:%u",
                                         __FUNCTION__, __LINE__, modulation, tstatus->u.sat.modulation);
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
                    STB_OSMutexLock(tstatus->lock);
                    CloseTuner(tstatus);
                    tune_idle_timer = 0;
                    STB_OSMutexUnlock(tstatus->lock);
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
                    if (aml_frontend_get_event(tstatus->frontend_fd, &fe_event))
                    {
                        if (!IsTuningParameterMatched(tstatus, fe_event))
                        {
                            break;
                        }

                        if ((fe_event.status & FE_HAS_LOCK) != 0)
                        {
                            tuner_locked = TRUE;
                            TUN_ERR("FE_GET_EVENT LOCKED:%d state:%d", locked, state);

                            fe_delivery_system_t fe_sys = SYS_UNDEFINED;
                            fe_modulation_t modulation = QPSK;
                            U32BIT srate = 0;
                            if (aml_frontend_get_delivery_system(tstatus->frontend_fd,
                                                                 (U32BIT *)&fe_sys, (U32BIT *)&modulation, &srate))
                            {
                                if ((SYS_DVBS == fe_sys || SYS_DVBS2 == fe_sys) &&
                                      (TUNE_SYSTEM_TYPE_DVBS == tstatus->sys_type || TUNE_SYSTEM_TYPE_DVBS2 == tstatus->sys_type))
                                {
                                   TUN_INFO("[%s:%d] data:%u, sys_type:%u", __FUNCTION__, __LINE__, fe_sys, tstatus->sys_type);
                                   STB_OSMutexLock(tstatus->mutex);
                                   if (SYS_DVBS == fe_sys)
                                       tstatus->sys_type = TUNE_SYSTEM_TYPE_DVBS;
                                   else
                                       tstatus->sys_type = TUNE_SYSTEM_TYPE_DVBS2;

                                   tstatus->u.sat.modulation = GetTuneModulation(modulation);
                                   STB_OSMutexUnlock(tstatus->mutex);
                                   TUN_INFO("[%s:%d] modulation:%u, sat.modulation:%u",
                                            __FUNCTION__, __LINE__, modulation, tstatus->u.sat.modulation);
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
                        TUN_ERR("%u: FE_GET_EVENT failed, errno %d", tstatus->path);
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
                            if (TRUE == STB_TuneIsSearchMode(tstatus->path))//Filter unstable signals
                            {
                                if (lost_signal_times < TUNER_LOST_LOCK_TIMES)
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
    if (!aml_frontend_clear_tuner(tstatus->frontend_fd))
    {
        TUN_ERR("%u: Failed to clear tuner", tstatus->path);
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
    else if (sys_type == TUNE_SYSTEM_TYPE_ATSC_T)
    {
        string = (U8BIT *)"ATSC-T";
    }
    else if (sys_type == TUNE_SYSTEM_TYPE_ATSC_C)
    {
        string = (U8BIT *)"ATSC-C";
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
    fe_delivery_system_t fe_sys = SYS_UNDEFINED;
    fe_modulation_t modulation = QPSK;
    U32BIT srate = 0;
    if (aml_frontend_get_delivery_system(tstatus->frontend_fd,
                                         (U32BIT *)&fe_sys, (U32BIT *)&modulation, &srate))
    {
        if (((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT) && (fe_sys == SYS_DVBT)) ||
                    ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2) && (fe_sys == SYS_DVBT2)) ||
                ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS) && (fe_sys == SYS_DVBS)) ||
                    ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS2) && (fe_sys== SYS_DVBS2)) ||
                ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBC)  &&
                 (fe_sys == SYS_DVBC_ANNEX_A || fe_sys == SYS_DVBC_ANNEX_C)) ||
             (tstatus->sys_type == TUNE_SYSTEM_TYPE_ATSC_T && fe_sys == SYS_ATSC) ||
             (tstatus->sys_type == TUNE_SYSTEM_TYPE_ATSC_C && fe_sys == SYS_DVBC_ANNEX_B))
        {
            TUN_DBG(" same sys_type %s, delivery system is %d", GetSysTypeDebugString(tstatus->sys_type), fe_sys);
            is_diff = FALSE;
        }
        else
        {
            TUN_DBG(" different sys_type %s, delivery system is %d", GetSysTypeDebugString(tstatus->sys_type), fe_sys);
            is_diff = TRUE;
        }
    }
    else
    {
        TUN_DBG("Fail to get delivery system");
        is_diff = TRUE;
    }

    return is_diff;
}

/*static*/ E_TUNER_EVENT GetTunerLockStatus(U32BIT frontend_fd)
{
    fe_status_t fe_status;
    E_TUNER_EVENT tune_event = TUNER_STATE_UNKNOWN;

    if (aml_frontend_get_tuner_status(frontend_fd, (U32BIT *)&fe_status))
    {
        TUN_DBG("status=0x%02x", fe_status);

        if ((fe_status & FE_HAS_LOCK) != 0)
        {
            tune_event = TUNER_STATE_LOCKED;
        }
        else if ((fe_status & FE_TIMEDOUT) != 0)
        {
            tune_event = TUNER_STATE_TIMEOUT;
        }
    }
    else
    {
        TUN_DBG("frontend_fd:%d Fail to get tuner lock status", frontend_fd);
    }

    return tune_event;
}

static void SetTunerT2PLP(U32BIT frontend_fd, U8BIT plp_id)
{
    if (!aml_frontend_set_dvbt2_plp_id(frontend_fd, plp_id))
    {
        TUN_ERR("Failed to set T2 PLP");
    }
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

    if (!aml_frontend_get_event(fd, &event))
    {
        TUN_ERR("Fail to get frontend event");
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
    return aml_frontend_blindscan_set_singlecable(fd, psinglecablePara);
}

static BOOLEAN dvb_blindscan_scan(U8BIT fd, struct dvbsx_blindscanpara *pbspara)
{
    return aml_frontend_blindscan_start(fd, pbspara);
}

static BOOLEAN dvb_blindscan_getscanevent(int frontend_fd, struct dvbsx_blindscanevent *pbsevent)
{
    BOOLEAN ret = TRUE;
    struct dvb_frontend_event event;
    ret = dvb_wait_event(frontend_fd, &event, 200);
    if (TRUE == ret)
    {
        TUN_ERR("Event status:%x",event.status);
        if (event.status&BLINDSCAN_UPDATESTARTFREQ)
        {
            pbsevent->status = event.status;
            pbsevent->u.m_uistartfreq_khz = event.parameters.frequency;
            if (event.status&FE_HAS_LOCK)
            {
                cur_evt.freq = event.parameters.frequency/1000;
                cur_evt.srate = event.parameters.u.qpsk.symbol_rate;
                TUN_ERR("update current freq:%dMhz",cur_evt.freq);
            }
        }
        else if (event.status&BLINDSCAN_UPDATEPROCESS)
        {
            pbsevent->status = BLINDSCAN_UPDATEPROCESS;
            pbsevent->u.m_uiprogress = event.parameters.frequency;
        }
        else if (event.status&BLINDSCAN_UPDATERESULTFREQ)
        {
            cur_evt.freq = event.parameters.frequency/1000;
            cur_evt.srate = event.parameters.u.qpsk.symbol_rate;
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

    return ret;
}

static BOOLEAN dvb_blindscan_cancel(U8BIT path)
{
    return aml_frontend_blindscan_cancel(tuner_status[path].frontend_fd);
}

static BOOLEAN dvb_blindscan_continue(U8BIT path)
{
    return aml_frontend_blindscan_next(tuner_status[path].frontend_fd);
}


/**\brief Performs a blind scan operation.*/
static BOOLEAN  AM_FEND_IBlindScanAPI_Start(U8BIT path)
{
    BOOLEAN ret = FALSE;

    STB_OSMutexLock(tuner_status[path].lock);
    struct dvbsx_singlecable_parameters * psinglecablePara = &(tuner_status[path].bs_setting.singlecablePara);
    if (psinglecablePara && psinglecablePara->version != 0)
    {
        ret = dvbsx_blindscan_setsinglecable(tuner_status[path].frontend_fd, psinglecablePara);
        if (!ret)
        {
            STB_OSMutexUnlock(tuner_status[path].lock);
            return ret;
        }
    }

    struct dvbsx_blindscanpara * pbsPara = &(tuner_status[path].bs_setting.bsPara);
    /*driver need to set in blindscan mode*/
    ret = dvb_blindscan_scan(tuner_status[path].frontend_fd, pbsPara);

    STB_OSMutexUnlock(tuner_status[path].lock);
    return ret;
}

/**\brief Queries the blind scan event.*/
static BOOLEAN  AM_FEND_IBlindScanAPI_GetScanEvent(U8BIT path, struct dvbsx_blindscanevent *pbsevent)
{
    BOOLEAN ret = FALSE;

    STB_OSMutexLock(tuner_status[path].lock);
    struct dvbsx_blindscanevent * pbsEvent = &(tuner_status[path].bs_setting.bsEvent);

    /*Query the internal blind scan procedure information.*/
    ret = dvb_blindscan_getscanevent(tuner_status[path].frontend_fd, pbsEvent);

    if (!ret)
    {
        ret = FALSE;
        STB_OSMutexUnlock(tuner_status[path].lock);
        return ret;
    }

    memcpy(pbsevent, pbsEvent, sizeof(struct dvbsx_blindscanevent));

    /*update tp info*/
    if (pbsEvent->status == BLINDSCAN_UPDATERESULTFREQ)
    {
        /*now driver return 1 tp locked*/
        for (U16BIT i = 0; i < tuner_status[path].bs_setting.m_uiChannelCount; i++)
        {
            /* skip it if already existed */
            if (0 == memcmp(&(tuner_status[path].bs_setting.channels[i]),
                            &(pbsEvent->u.parameters),
                            sizeof(struct dvb_frontend_parameters)))
            {
                TUN_INFO("channel freq(%lu) is duplicated the index [%d]\n",
                         pbsEvent->u.parameters.frequency, i);
                STB_OSMutexUnlock(tuner_status[path].lock);
                return ret;
            }
        }

        if (tuner_status[path].bs_setting.m_uiChannelCount == FEND_BS_MAX_CHANNEL) {
            TUN_ERR("channel count(%d) reaches the limit(%d)\n",
                    tuner_status[path].bs_setting.m_uiChannelCount, FEND_BS_MAX_CHANNEL);
            STB_OSMutexUnlock(tuner_status[path].lock);
            return ret;
        }

        memcpy(&(tuner_status[path].bs_setting.channels[tuner_status[path].bs_setting.m_uiChannelCount]),
               &(pbsEvent->u.parameters), sizeof(struct dvb_frontend_parameters));
        tuner_status[path].bs_setting.m_uiChannelCount++;
    }

    STB_OSMutexUnlock(tuner_status[path].lock);
    return ret;
}

/**\brief Stops blind scan process.*/
static BOOLEAN AM_FEND_IBlindScanAPI_Exit(U8BIT path)
{
    BOOLEAN ret = TRUE;

    STB_OSMutexLock(tuner_status[path].lock);
    /*driver need to set in demod mode*/
    ret = dvb_blindscan_cancel(path);

    STB_OSMutexUnlock(tuner_status[path].lock);

    usleep(10 * 1000);

    return ret;
}

static BOOLEAN AM_FEND_BlindDump(U8BIT path)
{
    BOOLEAN ret = TRUE;
    int i = 0;

    TUN_DBG( "AM_FEND_BlindDump start %d--------------------\n", tuner_status[path].bs_setting.m_uiChannelCount);
    STB_OSMutexLock(tuner_status[path].lock);

    for (i = 0; i < (tuner_status[path].bs_setting.m_uiChannelCount); i++)
    {
        TUN_DBG( "num:%d freq:%d symb:%d\n", i, tuner_status[path].bs_setting.channels[i].frequency, tuner_status[path].bs_setting.channels[i].u.qpsk.symbol_rate);
    }

    STB_OSMutexUnlock(tuner_status[path].lock);
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

        switch (BS_Status)
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

                if (!ret)
                {
                    BS_Status = DVB_BS_Status_Exit;
                    if (tuner_status[path].blindscan_cb)
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

                if (ret)
                {
                    BS_Status = DVB_BS_Status_User_Process;
                }
                else
                {
                    BS_Status = DVB_BS_Status_Wait;

                    wait_reports++;
                    // to avoid wait event reporting frequently
                    if (wait_reports == 5 && tuner_status[path].blindscan_cb)
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

                if (tuner_status[path].blindscan_cb)
                {
                    if (cur_bsevent.status & BLINDSCAN_UPDATESTARTFREQ)
                    {
                        TUN_DBG( "adp start freq %d\n", cur_bsevent.u.m_uistartfreq_khz);
                        evt.freq = cur_bsevent.u.m_uistartfreq_khz;

                        evt.status = AM_FEND_BLIND_START;
                        if (cur_bsevent.status & FE_HAS_LOCK)
                        {
                            evt.status = AM_FEND_BLIND_UPDATETP;
                        }
                        tuner_status[path].blindscan_cb(path, &evt, tuner_status[path].blindscan_cb_user_data);
                    }
                    else if (cur_bsevent.status == BLINDSCAN_UPDATEPROCESS)
                    {
                        TUN_DBG( "adp process %d\n", cur_bsevent.u.m_uiprogress);
                        evt.process = cur_bsevent.u.m_uiprogress;

                        evt.status = AM_FEND_BLIND_UPDATEPROCESS;
                        tuner_status[path].blindscan_cb(path, &evt, tuner_status[path].blindscan_cb_user_data);
                    }
                    else if (cur_bsevent.status & BLINDSCAN_UPDATERESULTFREQ)
                    {
                        TUN_DBG( "adp result freq %d symb %d\n", cur_bsevent.u.parameters.frequency, cur_bsevent.u.parameters.u.qpsk.symbol_rate);

                        evt.status = AM_FEND_BLIND_UPDATETP;
                        evt.freq = cur_bsevent.u.parameters.frequency;
                        evt.srate = cur_bsevent.u.parameters.u.qpsk.symbol_rate;
                        tuner_status[path].blindscan_cb(path, &evt, tuner_status[path].blindscan_cb_user_data);
                    }
                }

                /*------------Custom code end -------------------*/
                if (cur_bsevent.status & BLINDSCAN_UPDATESTARTFREQ)
                {
                    BS_Status = DVB_BS_Status_Wait;
                }
                else if (cur_bsevent.status == BLINDSCAN_UPDATEPROCESS)
                {
                    if (evt.process < 100)
                        BS_Status = DVB_BS_Status_Wait;
                    else
                        BS_Status = DVB_BS_Status_WaitExit;
                }
                else if (cur_bsevent.status == BLINDSCAN_UPDATERESULTFREQ)
                {
                    BS_Status = DVB_BS_Status_Wait;
                }
                else if (cur_bsevent.status == BLINDSCAN_UPDATERESULT_OTHERS)
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
                if (FALSE == ret)
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
    E_STB_TUNE_CMODE cmode = TUNE_MODE_QAM_UNDEFINED;
    U_STB_DEMO_CAPABILITY uCap;
    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD)
                           && IsTunerLocked(&tuner_status[path]))
    {
        if ((TRUE == STB_GetDemoCapabilityByType(TUNE_SIGNAL_QAM, &uCap)) && (1 == uCap.dvbc.symbol_rate_auto))
        {
            fe_delivery_system_t fe_sys = SYS_UNDEFINED;
            fe_modulation_t modulation = QPSK;
            U32BIT srate = 0;
            if (aml_frontend_get_delivery_system(tuner_status[path].frontend_fd,
                                                 (U32BIT *)&fe_sys, (U32BIT *)&modulation, &srate))

            {
                if (SYMBOL_RATE_AUTO == srate)
                {
                    return FALSE;
                }
                else
                {
                    switch (modulation)
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
                    if (tuner_status[path].signal_type == TUNE_SIGNAL_QAM)
                    {
                        TUN_DBG("%u: symbol rate = %lu, cable mode = %lu", path, srate, modulation);
                    }
                    else
                    {
                        TUN_DBG("%u: from driver symbol rate = %lu", path, srate);
                    }
                }
                real_srate = srate;
                real_cmode = cmode;
                return TRUE;
            }
            else
            {
                TUN_ERR("%u: Failed to get real param from driver", path);
                return FALSE;
            }
        }
    }
    return FALSE;
}

E_TUNER_EVENT STB_TuneGetLockStatus(U8BIT path)
{
    E_TUNER_EVENT tuner_event = TUNER_STATE_UNKNOWN;

    if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
    {
        tuner_event = GetTunerLockStatus(tuner_status[path].frontend_fd);
    }

    return tuner_event;
}

int STB_TuneGetEwbsFlag(U8BIT path)
{
    int retval = 0;
    U32BIT sys_id = 0;
    U32BIT ewbs_flag = 0;

    if (aml_frontend_get_isdbt_partial_reception(tuner_status[path].frontend_fd, &sys_id, &ewbs_flag))
    {
        TUN_DBG("%u: sysid=%u ewbsflag=%u", path, sys_id, ewbs_flag);
        retval = (int)ewbs_flag;
    }
    else
    {
        TUN_ERR("%u: Failed to get ewbs flag", path);
    }

    return(retval);
}
