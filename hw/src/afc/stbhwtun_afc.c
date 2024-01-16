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
#define TUNER_WRAPPER

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
#include "stbhwc.h"
#include "stbhwini.h"
#include "stbhwutils.h"


#include "emu_internal.h"
#include "wrapper_frontend.h"
/*---Macro Definitions for this file-----------------------------------------*/
#ifdef TUNER_DEBUG
#define TUN_DBG(x,...)          STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define TUN_DBG(x,...)
#endif

#define TUN_ERR(x,...)          STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#define TUN_INFO(x,...)         STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

/*---local (static) variable declarations for this file----------------------*/
static void Tuner_EventCallback(BOOLEAN repeat, U16BIT event_class, U16BIT event_type, void *data, U32BIT data_size)
{
    TUN_DBG("Tuner_EventCallback");
    STB_OSSendEvent(repeat, event_class, event_type, data, data_size);
}

static BOOLEAN IsPercentConversionRequired(U8BIT path)
{
    BOOLEAN retval = TRUE;

    // TODO

    return retval;
}

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialises the tuner component
 * @param   paths number of tuning paths to initialise
 */
void STB_TuneInitialise(U8BIT paths)
{
    FUNCTION_START(STB_TuneInitialise);

    Wrapper_TuneInitialise(paths);

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

    Wrapper_TuneAutoRelock(path, state);

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
    FUNCTION_START(STB_TuneSetActualTsInputIdx);

    Wrapper_TuneSetActualTsInputIdx(path, frontend_fd);

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

    Wrapper_TuneSetActualSupportedSystemType(path, frontend_fd);

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

    sig_type = (E_STB_TUNE_SIGNAL_TYPE)Wrapper_TuneGetSignalType(path);
    if (sig_type == TUNE_SIGNAL_NONE)
    {
        if (path < aml_hw_cfg.tuner_num)
        {
            sig_type = aml_hw_cfg.tuners[path].signal_types;
        }
        else
        {
            sig_type = TUNE_SIGNAL_NONE;
        }
    }

    FUNCTION_FINISH(STB_TuneGetSignalType);

    return sig_type;
}

U16BIT STB_TuneGetActualSignalType(U8BIT path)
{
    U16BIT sig_type;

    FUNCTION_START(STB_TuneGetSignalType);

    sig_type = (U16BIT)Wrapper_TuneGetActualSignalType(path);

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

    Wrapper_TuneSetSignalType(path, (EW_STB_TUNE_SIGNAL_TYPE)type);

    FUNCTION_FINISH(STB_TuneSetSignalType);
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
    FUNCTION_START(STB_TuneStartTuner);

    USE_UNWANTED_PARAM(freq_off);
    USE_UNWANTED_PARAM(anlg_vtype);

    TUN_DBG("STB_TuneStartTuner enter %d",freq);

    Wrapper_SendEvent callback = Tuner_EventCallback;
    Wrapper_RegisterCallback(callback);
    Wrapper_TuneStartTuner(path, freq, srate,
                           (EW_STB_TUNE_FEC)fec, (EW_STB_TUNE_TMODE)tmode,
                           (EW_STB_TUNE_TBWIDTH)tbwidth, (EW_STB_TUNE_CMODE)cmode);

    FUNCTION_FINISH(STB_TuneStartTuner);
}

/**
 * @brief   Stops any locking attempt, or unlocks if locked
 * @param   path the tuner path to stop
 */
void STB_TuneStopTuner(U8BIT path)
{
    FUNCTION_START(STB_TuneStopTuner);

    Wrapper_TuneStopTuner(path);

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

    symbol_rate = Wrapper_TuneGetMinTunerSymbolRate(path);

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

    symbol_rate = Wrapper_TuneGetMaxTunerSymbolRate(path);

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

    min_freq = Wrapper_TuneGetMinTunerFreqKHz(path);

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

    max_freq = Wrapper_TuneGetMaxTunerFreqKHz(path);

    FUNCTION_FINISH(STB_TuneGetMaxTunerFreqKHz);

    return(max_freq);
}

/**
 * @brief   Returns the current signal dBuV
 * @param   path the tuner path to query
 * @return  the signal dBuV as percentage of maximum (0-100)
 */
U8BIT STB_TuneGetSignaldBuV(U8BIT path)
{
    U8BIT retval = 0;

    FUNCTION_START(STB_TuneGetSignaldBuV);


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

    FUNCTION_START(STB_TuneGetSignalStrength);

    if (WRAPPER_TUNER_STATE_LOCKED == Wrapper_TuneGetLockStatus(path)) {
        retval = STB_TuneReadSignalStrength(path);
    }
    else {
        TUN_DBG("%u: Unlock", path);
    }

    FUNCTION_FINISH(STB_TuneGetSignalStrength);

    return retval;
}

U8BIT STB_TuneReadSignalStrength(U8BIT path)
{
    U8BIT retval = 0;
    S16BIT strength = 0;

    FUNCTION_START(STB_TuneReadSignalStrength);

    strength = (S16BIT)Wrapper_TuneGetSignalStrength(path);
    if (IsPercentConversionRequired(path))
    {
        retval = STB_Utils_StrengthToSSI(path, strength);
        TUN_DBG("%u: Percent=%u%%(strength:%d)", path, retval, strength);
    }
    else
    {
        retval = (U8BIT)strength;
        TUN_DBG("%u: Strength:%d", path, retval);
    }

    FUNCTION_FINISH(STB_TuneReadSignalStrength);

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
    U32BIT retval = 0;

    FUNCTION_START(STB_TuneGetDataIntegrity);

    if (WRAPPER_TUNER_STATE_LOCKED == Wrapper_TuneGetLockStatus(path)) {
        retval = Wrapper_TuneGetDataIntegrity(path);
    }
    else {
        TUN_DBG("%u: Unlock", path);
    }

    FUNCTION_FINISH(STB_TuneGetDataIntegrity);

    return retval;
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

    FUNCTION_START(STB_TuneGetSignalQuality);

    if (WRAPPER_TUNER_STATE_LOCKED == Wrapper_TuneGetLockStatus(path)) {
        retval = STB_TuneReadSignalQuality(path);
    }
    else {
        TUN_DBG("%u: Unlock", path);
    }

    FUNCTION_FINISH(STB_TuneGetSignalQuality);

    return retval;
}

U8BIT STB_TuneReadSignalQuality(U8BIT path)
{
    U8BIT retval = 0;
    S16BIT quality = 0;

    FUNCTION_START(STB_TuneReadSignalQuality);

    quality = (S16BIT)Wrapper_TuneGetSignalQuality(path);
    if (IsPercentConversionRequired(path))
    {
        retval = STB_Utils_SNR10ToSQI(path, quality);
        TUN_DBG("%u: Percent=%u%%(snr=%d.%d)", path, retval, quality / 10, quality % 10);
    }
    else
    {
        retval = (U8BIT)quality;
        TUN_DBG("%u: Snr=%d.%d", path, retval / 10, retval % 10);
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

    FUNCTION_START(STB_TuneGetSignalSNR);


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

    FUNCTION_START(STB_TuneGetActualTerrFrequency);

    freq = Wrapper_TuneGetActualTerrFrequency(path);

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

    FUNCTION_START(STB_TuneGetActualTerrFreqOffset);

    offset = Wrapper_TuneGetActualTerrFreqOffset(path);

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

    FUNCTION_START(STB_TuneGetActualTerrMode);

    mode = (E_STB_TUNE_TMODE)Wrapper_TuneGetActualTerrMode(path);

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
    E_STB_TUNE_TBWIDTH bwidth;

    bwidth = (E_STB_TUNE_TBWIDTH)Wrapper_TuneGetActualTerrBwidth(path);

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

    t_modu = (E_STB_TUNE_TCONST)Wrapper_TuneGetActualTerrConstellation(path);

    FUNCTION_FINISH(STB_TuneGetActualTerrConstellation);

    return t_modu;
}

/**
 * @brief   Returns the heirarchy of the current terrestrial signal
 * @param   tuner_id, the tuner index to query
 * @return  the heirarchy
 */
E_STB_TUNE_HIERARCHY STB_TuneGetActualTerrHierarchy(U8BIT tuner_id)
{
    U8BIT retval = TUNE_HIERARCHY_NONE;

    FUNCTION_START(STB_TuneGetActualTerrHierarchy);

    retval = (E_STB_TUNE_HIERARCHY)Wrapper_TuneGetActualTerrHierarchy(tuner_id);

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

    FUNCTION_START(STB_TuneGetMPLPIDList);

    retval = Wrapper_TuneGetMPLPIDList(tuner_id, plp_list, listlen);

    FUNCTION_FINISH(STB_TuneGetMPLPIDList);

    return retval;
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

    t_rc = (E_STB_TUNE_TCODERATE)Wrapper_TuneGetActualTerrLpCodeRate(path);

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

    t_rc = (E_STB_TUNE_TCODERATE)Wrapper_TuneGetActualTerrHpCodeRate(path);

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

    cell_id = Wrapper_TuneGetActualTerrCellId(path);

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
    E_STB_TUNE_TBWIDTH bwidth = TUNE_TBWIDTH_8MHZ;

    //bwidth = Wrapper_TuneGetActualIsdbtBwidth(path);

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

    voltage = (E_STB_TUNE_LNB_VOLTAGE)Wrapper_TuneGetLNBVoltage(path);

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

    Wrapper_TuneSetLNBVoltage(path, (EW_STB_TUNE_LNB_VOLTAGE)voltage, retune);

    FUNCTION_FINISH(STB_TuneSetLNBVoltage);
}

void STB_TuneSetFrontendFd(U8BIT path, U32BIT fe_fd)
{
    USE_UNWANTED_PARAM(path);
    USE_UNWANTED_PARAM(fe_fd);
}

E_STB_TUNE_SYSTEM_TYPE STB_TuneGetActualSysType(U8BIT path)
{
    E_STB_TUNE_SYSTEM_TYPE sys_type = TUNE_SYSTEM_TYPE_UNKNOWN;

    return sys_type;
}

void STB_TuneSetVoltageInterface(U8BIT path, E_STB_TUNE_LNB_VOLTAGE vol)
{
    USE_UNWANTED_PARAM(path);
    USE_UNWANTED_PARAM(vol);
}

/**
 * @brief   Sets the type of modulation for the specified tuner
 * @param   path tuner path
 * @param   modulation type of modulation
 */
void STB_TuneSetModulation(U8BIT path, E_STB_TUNE_MODULATION modulation)
{
    FUNCTION_START(STB_TuneSetModulation);

    Wrapper_TuneSetModulation(path, (EW_STB_TUNE_MODULATION)modulation);

    FUNCTION_FINISH(STB_TuneSetModulation);
}

BOOLEAN STB_TuneGet22kState(U8BIT path)
{
    BOOLEAN state = FALSE;

    FUNCTION_START(STB_TuneGet22kState);

    state = Wrapper_TuneGet22kState(path);

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

    Wrapper_TuneSet22kState(path, state, retune);

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

    Wrapper_TuneSendDISEQCMessage(path, data, size);

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

    Wrapper_TuneReceiveDISEQCReply(path, data, size, timeout);

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

    Wrapper_TuneSendBurstMessage(path, data);

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

    Wrapper_TuneSetLOFrequency(path, lo_freq);

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

    Wrapper_TuneSetSystemType(path, (EW_STB_TUNE_SYSTEM_TYPE)type);

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

    type = (E_STB_TUNE_SYSTEM_TYPE)Wrapper_TuneGetSystemType(path);

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
    FUNCTION_FINISH(STB_TuneGetModulation);

    return (E_STB_TUNE_MODULATION)Wrapper_TuneGetModulation(path);
}

/**
 * @brief   Sets the Physical Layer Pipe to be acquired
 * @param   path the tuner path to set up
 * @param   plp Physical Layer Pipe to be acquired
 */
void STB_TuneSetPLP(U8BIT path, U8BIT plp)
{
    FUNCTION_START(STB_TuneSetPLP);

    Wrapper_TuneSetPLP(path, plp);

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

    srate = Wrapper_TuneGetActualSymbolRate(path);

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

    EW_STB_TUNE_CMODE cmode = Wrapper_TuneGetActualCableMode(path);
    mode = (E_STB_TUNE_CMODE)cmode;

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
    FUNCTION_START(STB_TuneGetSupportedSystemType);

    Wrapper_TuneGetSupportedSystemType(path, support_sys);

    FUNCTION_FINISH(STB_TuneGetSupportedSystemType);
}
BOOLEAN STB_TuneOpen(U8BIT path)
{
    BOOLEAN ret = FALSE;
    FUNCTION_START(STB_TuneOpen);

    ret = Wrapper_TuneOpen(path);

    FUNCTION_FINISH(STB_TuneOpen);
    return ret;
}


BOOLEAN STB_TuneIsOpened(U8BIT path)
{
    FUNCTION_START(STB_TuneIsOpened);

    BOOLEAN ret = FALSE;
    ret = Wrapper_TuneIsOpened(path);

    FUNCTION_FINISH(STB_TuneIsOpened);
    return ret;
}

void STB_TuneUpdateFeUsage(U8BIT path, BOOLEAN use)
{
    FUNCTION_START(STB_TuneUpdateFeUsage);

    Wrapper_TuneUpdateFeUsage(path, use);

    FUNCTION_FINISH(STB_TuneUpdateFeUsage);
}

BOOLEAN STB_TuneIsTvPlatform()
{
    FUNCTION_START(STB_TuneIsTvPlatform);

    BOOLEAN isTvPlatform = FALSE;
    isTvPlatform = Wrapper_TuneIsTvPlatform();

    FUNCTION_FINISH(STB_TuneIsTvPlatform);

    return isTvPlatform;
}

void STB_TuneSetSearchMode(U8BIT path, BOOLEAN mode)
{
    FUNCTION_START(STB_TuneSetSearchMode);

    Wrapper_TuneSetSearchMode(path, mode);

    FUNCTION_FINISH(STB_TuneSetSearchMode);
}

BOOLEAN STB_TuneIsSearchMode(U8BIT path)
{
    FUNCTION_START(STB_TuneIsSearchMode);

    BOOLEAN search_mode = FALSE;
    search_mode = Wrapper_TuneIsSearchMode(path);

    FUNCTION_FINISH(STB_TuneIsSearchMode);
    return search_mode;
}

void STB_TuneAllStart()
{
    FUNCTION_START(STB_TuneAllStart);

    Wrapper_TuneAllStart();

    FUNCTION_FINISH(STB_TuneAllStart);
}

void STB_TuneAllStop()
{
    FUNCTION_START(STB_TuneAllStop);

    if (STB_TuneIsTvPlatform())
    {
        STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_RESOURCE_BUSY, NULL, 0);
    }

    Wrapper_TuneAllStop();

    FUNCTION_FINISH(STB_TuneAllStop);
}

BOOLEAN STB_Tune_BlindExit(U8BIT path)
{
    BOOLEAN ret = TRUE;

    FUNCTION_START(STB_Tune_BlindExit);

    ret = Wrapper_Tune_BlindExit(path);

    FUNCTION_FINISH(STB_Tune_BlindExit);

    return ret;
}

void STB_Tune_BlindGetTPCount(U8BIT path, U16BIT *count)
{
    FUNCTION_START(STB_Tune_BlindGetTPCount);

    Wrapper_Tune_BlindGetTPCount(path, count);

    FUNCTION_FINISH(STB_Tune_BlindGetTPCount);
}

BOOLEAN STB_Tune_BlindGetTPInfo(U8BIT path, void *para, U16BIT *count)
{
    BOOLEAN ret = TRUE;
    U32BIT* freq = NULL;
    U32BIT* srate = NULL;

    FUNCTION_START(STB_Tune_BlindGetTPInfo);

    ret = Wrapper_Tune_BlindGetTPInfo(path, &freq, &srate, count);
    if (FALSE == ret)
    {
        return FALSE;
    }

    struct dvb_frontend_parameters* fe_para = (struct dvb_frontend_parameters*)para;
    if (fe_para == NULL || freq == NULL || srate == NULL)
    {
        return FALSE;
    }

    for (U16BIT i = 0; i < *count; i++)
    {
        fe_para[i].frequency = freq[i];
        fe_para[i].u.qpsk.symbol_rate = srate[i];
    }

    FUNCTION_FINISH(STB_Tune_BlindGetTPInfo);

    return ret;
}

BOOLEAN STB_Tune_BlindScan(U8BIT path, E_STB_TUNE_SYSTEM_TYPE sys_type, STB_Tnue_BlindCallback_t cb, void *user_data,
                                 unsigned int start_freq, unsigned int stop_freq, E_STB_TUNE_BlindUnicable_t unicable)
{
    BOOLEAN ret = TRUE;
    E_TTYPE terr_type = E_TERR_TYPE_UNKNOWN;
    EW_STB_TUNE_BlindUnicable_t w_unicable;

    FUNCTION_START(STB_Tune_BlindScan);

    if (sys_type == TUNE_SYSTEM_TYPE_DVBS || sys_type == TUNE_SYSTEM_TYPE_DVBS2)
    {
        terr_type = E_TERR_TYPE_DVBS;
    }
    else if (sys_type == TUNE_SYSTEM_TYPE_DVBC)
    {
        terr_type = E_TERR_TYPE_DVBC;
    }

    w_unicable.frequency = unicable.frequency;
    w_unicable.unicable = unicable.unicable;
    w_unicable.channel = unicable.channel;
    w_unicable.bank = unicable.bank;
    w_unicable.position_b = unicable.position_b;
    w_unicable.uncommitted = unicable.uncommitted;
    w_unicable.committed = unicable.committed;

    if (!Wrapper_Tune_BlindScan(path, terr_type, (Wrapper_Tune_BlindCallback_t)cb, user_data, start_freq, stop_freq, w_unicable))
    {
        E_STB_TUNE_BlindEvent_t evt;
        evt.status = AM_FEND_BLIND_START_FAILED;
        cb(path, &evt, user_data);
    }

    FUNCTION_FINISH(STB_Tune_BlindScan);

    return ret;
}

BOOLEAN STB_Tune_BlindContinue(U8BIT path)
{
    BOOLEAN ret = TRUE;

    return ret;
}


/*---local function definitions----------------------------------------------*/

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

E_TUNER_EVENT STB_TuneGetLockStatus(U8BIT path)
{
    E_TUNER_EVENT tuner_event = TUNER_STATE_UNKNOWN;
    FUNCTION_START(STB_TuneGetLockStatus);

    tuner_event = (E_TUNER_EVENT)Wrapper_TuneGetLockStatus(path);

    FUNCTION_FINISH(STB_TuneGetLockStatus);
    return tuner_event;
}

