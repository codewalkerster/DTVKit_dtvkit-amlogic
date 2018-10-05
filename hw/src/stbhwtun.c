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

/*#define TUNER_DEBUG*/

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */

/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwtun.h"

/* third party header files */

/*---Macro Definitions for this file-----------------------------------------*/
#ifdef TUNER_DEBUG
   #define TUN_DBG(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define TUN_DBG(x,...)
#endif

/*---constant definitions for this file--------------------------------------*/


/*---local typedef structs for this file-------------------------------------*/


/*---local (static) variable declarations for this file----------------------*/


/*---local function prototypes for this file---------------------------------*/


/*---local function definitions----------------------------------------------*/


/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialises the tuner component
 * @param   paths number of tuning paths to initialise
 */
void STB_TuneInitialise(U8BIT paths)
{
   FUNCTION_START(STB_TuneInitialise);
   USE_UNWANTED_PARAM(paths);
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
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(state);
   FUNCTION_FINISH(STB_TuneAutoRelock);
}

/**
 * @brief   Gets the signal type of the tuner path
 * @param   path the tuner path to configure
 * @return  the tuner signal type
 */
E_STB_TUNE_SIGNAL_TYPE STB_TuneGetSignalType(U8BIT path)
{
   FUNCTION_START(STB_TuneGetSignalType);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetSignalType);

   return TUNE_SIGNAL_NONE;
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
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(freq);
   USE_UNWANTED_PARAM(srate);
   USE_UNWANTED_PARAM(fec);
   USE_UNWANTED_PARAM(freq_off);
   USE_UNWANTED_PARAM(tmode);
   USE_UNWANTED_PARAM(tbwidth);
   USE_UNWANTED_PARAM(cmode);
   USE_UNWANTED_PARAM(anlg_vtype);
   FUNCTION_FINISH(STB_TuneStartTuner);
}

/**
 * @brief   Restarts tuner and attempts to lock to signal in StartTuner call
 * @param   path the tuner path to restart
 */
void STB_TuneRestartTuner(U8BIT path)
{
   FUNCTION_START(STB_TuneRestartTuner);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneRestartTuner);
}

/**
 * @brief   Stops any locking attempt, or unlocks if locked
 * @param   path the tuner path to stop
 */
void STB_TuneStopTuner(U8BIT path)
{
   FUNCTION_START(STB_TuneStopTuner);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneStopTuner);
}

/**
 * @brief   Returns the minimum tuner frequency in KHz
 * @param   path the tuner path to query
 * @return  minimum frequency in Khz
 */
U32BIT STB_TuneGetMinTunerFreqKHz(U8BIT path)
{
   FUNCTION_START(STB_TuneGetMinTunerFreqKHz);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetMinTunerFreqKHz);

   return(0);
}

/**
 * @brief   Returns the maximum tuner frequency in KHz
 * @param   path the tuner path to query
 * @return  maximum frequency in Khz
 */
U32BIT STB_TuneGetMaxTunerFreqKHz(U8BIT path)
{
   FUNCTION_START(STB_TuneGetMaxTunerFreqKHz);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetMaxTunerFreqKHz);

   return(0);
}

/**
 * @brief   Returns the current signal strength
 * @param   path the tuner path to query
 * @return  the signal strength as percentage of maximum (0-100)
 */
U8BIT STB_TuneGetSignalStrength(U8BIT path)
{
   FUNCTION_START(STB_TuneGetSignalStrength);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetSignalStrength);

   return 0;
}

/**
 * @brief   Returns the current data integrity
 * @param   path the tuner path to query
 * @return  the data integrity as percentage of maximum possible (0-100)
 * @todo     Confirm DVB API BER units
 */
U8BIT STB_TuneGetDataIntegrity(U8BIT path)
{
   FUNCTION_START(STB_TuneGetDataIntegrity);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetDataIntegrity);

   return 0;
}

/**
 * @brief   Returns the actual frequency of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the frequency in Hz
 */
U32BIT STB_TuneGetActualTerrFrequency(U8BIT path)
{
   FUNCTION_START(STB_TuneGetActualTerrFrequency);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetActualTerrFrequency);
   return(0);
}

/**
 * @brief   Returns the actual freq offset of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the frequency offset in Hz
 */
S8BIT STB_TuneGetActualTerrFreqOffset(U8BIT path)
{
   FUNCTION_START(STB_TuneGetActualTerrFreqOffset);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetActualTerrFreqOffset);
   return(0);
}

/**
 * @brief   Returns the actual mode of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the tuning mode
 */
E_STB_TUNE_TMODE STB_TuneGetActualTerrMode(U8BIT path)
{
   FUNCTION_START(STB_TuneGetActualTerrMode);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetActualTerrMode);
   return(TUNE_MODE_COFDM_UNDEFINED);
}

/**
 * @brief   Returns the actual bandwidth of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the signal bandwidth
 */
E_STB_TUNE_TBWIDTH STB_TuneGetActualTerrBwidth(U8BIT path)
{
   FUNCTION_START(STB_TuneGetActualTerrBwidth);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetActualTerrBwidth);
   return 0;
}

/**
 * @brief   Returns the constellation of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the constellation
 */
E_STB_TUNE_TCONST STB_TuneGetActualTerrConstellation(U8BIT path)
{
   FUNCTION_START(STB_TuneGetActualTerrConstellation);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetActualTerrConstellation);
   return 0;
}

/**
 * @brief   Returns the heirarchy of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the heirarchy
 */
E_STB_TUNE_THIERARCHY STB_TuneGetActualTerrHierarchy(U8BIT path)
{
   FUNCTION_START(STB_TuneGetActualTerrHierarchy);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetActualTerrHierarchy);
   return TUNE_THIERARCHY_UNDEFINED;
}

/**
 * @brief   Returns the LP code rate of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  The LP code rate
 */
E_STB_TUNE_TCODERATE STB_TuneGetActualTerrLpCodeRate(U8BIT path)
{
   FUNCTION_START(STB_TuneGetActualTerrLpCodeRate);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetActualTerrLpCodeRate);
   return(TUNE_TCODERATE_UNDEFINED);
}

/**
 * @brief   Returns the HP code rate of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  The HP code rate
 */
E_STB_TUNE_TCODERATE STB_TuneGetActualTerrHpCodeRate(U8BIT path)
{
   FUNCTION_START(STB_TuneGetActualTerrHpCodeRate);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetActualTerrHpCodeRate);
   return(TUNE_TCODERATE_UNDEFINED);
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
   FUNCTION_START(STB_TuneGetActualTerrCellId);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetActualTerrCellId);
   return(0);
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

/**
 * @brief   Sets the LNB voltage for the given tuner
 * @param   path tuner path
 * @param   voltage voltage setting
 */
void STB_TuneSetLNBVoltage(U8BIT path, E_STB_TUNE_LNB_VOLTAGE voltage)
{
   FUNCTION_START(STB_TuneSetLNBVoltage);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(voltage);
   FUNCTION_FINISH(STB_TuneSetLNBVoltage);
}

/**
 * @brief   Sets the type of modulation for the specified tuner
 * @param   path tuner path
 * @param   modulation type of modulation
 */
void STB_TuneSetModulation(U8BIT path, E_STB_TUNE_MODULATION modulation)
{
   FUNCTION_START(STB_TuneSetModulation);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(modulation);
   FUNCTION_FINISH(STB_TuneSetModulation);
}

/**
 * @brief   Turns the 22 kHz tone on or off
 * @param   path tuner path
 * @param   state TRUE to turn the tone on, FALSE to turn it off
 */
void STB_TuneSet22kState(U8BIT path, BOOLEAN state)
{
   FUNCTION_START(STB_TuneSet22kState);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(state);
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
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(data);
   USE_UNWANTED_PARAM(size);
   FUNCTION_FINISH(STB_TuneSendDISEQCMessage);
}

/**
 * @brief   Sets the pulse limit for the east
 * @param   path tuner path
 * @param   count east limit count
 */
void STB_TuneSetPulseLimitEast(U8BIT path, U16BIT count)
{
   FUNCTION_START(STB_TuneSetPulseLimitEast);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(count);
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
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(count);
   FUNCTION_FINISH(STB_TuneSetPulseLimitWest);
}

void STB_TuneChangePulsePosition(U8BIT path, U16BIT count)
{
   FUNCTION_START(STB_TuneChangePulsePosition);
   USE_UNWANTED_PARAM(path);
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
void STB_TuneSetLOFrequency(U8BIT tuner, U16BIT lo_freq)
{
   FUNCTION_START(STB_TuneSetLOFrequency);
   USE_UNWANTED_PARAM(tuner);
   USE_UNWANTED_PARAM(lo_freq);
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
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(type);
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
   FUNCTION_START(STB_TuneGetSystemType);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetSystemType);
   return(TUNE_SYSTEM_TYPE_UNKNOWN);
}

/**
 * @brief   Sets the Physical Layer Pipe to be acquired
 * @param   path the tuner path to set up
 * @param   plp Physical Layer Pipe to be acquired
 */
void STB_TuneSetPLP(U8BIT path, U8BIT plp)
{
   FUNCTION_START(STB_TuneSetPLP);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(plp);
   FUNCTION_FINISH(STB_TuneSetPLP);
}

/**
 * @brief   Returns the actual symbol rate when a tuner has locked
 * @param   path tuner path
 * @return  Symbol rate in symbols per second
 */
U32BIT STB_TuneGetActualSymbolRate(U8BIT path)
{
   FUNCTION_START(STB_TuneGetActualSymbolRate);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetActualSymbolRate);
   return(0);
}

/**
 * @brief   Returns the cable mode when the tuner has locked
 * @param   path tuner path
 * @return  QAM mode
 */
E_STB_TUNE_CMODE STB_TuneGetActualCableMode(U8BIT path)
{
   FUNCTION_START(STB_TuneGetActualCableMode);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetActualCableMode);
   return(TUNE_MODE_QAM_UNDEFINED);
}

/**
 * @brief   Returns the system type supported by the path. This function
 *          differs from STB_TuneGetSystemType which only returns T2 or S2 if
 *          the tuner is currently performing T2 or S2 operations.
 * @param   path  the tuner path to query
 * @return  (E_STB_TUNE_SYSTEM_TYPE) the system type supported by this path.
 *          TUNE_SYSTEM_TYPE_DVBT2 means both DVBT and DVBT2 are supported,
 *          TUNE_SYSTEM_TYPE_DVBS2 means both DVBS and DVBS2 are supported
 */
E_STB_TUNE_SYSTEM_TYPE STB_TuneGetSupportedSystemType(U8BIT path)
{
   FUNCTION_START(STB_TuneGetSupportedSystemType);
   USE_UNWANTED_PARAM(path);
   FUNCTION_FINISH(STB_TuneGetSupportedSystemType);

   return TUNE_SYSTEM_TYPE_UNKNOWN;
}
