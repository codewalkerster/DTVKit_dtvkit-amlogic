/*******************************************************************************
 * Copyright © 2014 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * Copyright © 2004 Ocean Blue Software Ltd
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
 * @brief   Header file - Function prototypes for tuner control
 * @file    stbhwtun.h
 * @date    06/02/2001
 */

// pre-processor mechanism so multiple inclusions don't cause compilation error

#ifndef _STBHWTUN_H

#define _STBHWTUN_H

//---#includes for this file---------------------------------------------------
#include "techtype.h"
#include "stbhwc.h"

/* third party header files */

//---Constant and macro definitions for public use-----------------------------
#define SYMBOL_RATE_AUTO         0

#define MAX_PLP_NUMBER      256

//---Enumerations for public use-----------------------------------------------
typedef enum e_stb_tune_system_type
{
    TUNE_SYSTEM_TYPE_UNKNOWN = 0,
    TUNE_SYSTEM_TYPE_DVBT = 1,
    TUNE_SYSTEM_TYPE_DVBT2 = 2,
    TUNE_SYSTEM_TYPE_DVBS = 3,
    TUNE_SYSTEM_TYPE_DVBS2 = 4,
    TUNE_SYSTEM_TYPE_DVBC = 5,
    TUNE_SYSTEM_TYPE_ISDBT = 6,
    TUNE_SYSTEM_TYPE_ANALOG = 7,
    TUNE_SYSTEM_TYPE_ATSC_C = 8,
    TUNE_SYSTEM_TYPE_ATSC_T = 9,
    TUNE_SYSTEM_TYPE_END = 10
} E_STB_TUNE_SYSTEM_TYPE;


typedef enum e_stb_tune_signal_type
{
    TUNE_SIGNAL_NONE = 0,
    TUNE_SIGNAL_QPSK = 1,
    TUNE_SIGNAL_COFDM = 2,
    TUNE_SIGNAL_QAM = 4,
    TUNE_SIGNAL_ISDBT = 5,
    TUNE_SIGNAL_ANALOG = 8,
    TUNE_SIGNAL_VSB  = 16,
    TUNE_SIGNAL_QAMB = 64
} E_STB_TUNE_SIGNAL_TYPE;

/* Terrestrial */
typedef enum e_stb_tune_tmode
{
    TUNE_MODE_COFDM_1K = 0,
    TUNE_MODE_COFDM_2K = 1,
    TUNE_MODE_COFDM_4K = 2,
    TUNE_MODE_COFDM_8K = 3,
    TUNE_MODE_COFDM_16K = 4,
    TUNE_MODE_COFDM_32K = 5,
    TUNE_MODE_VSB_8 = 6,
    TUNE_MODE_VSB_16 = 7,
    TUNE_MODE_COFDM_UNDEFINED = 255
} E_STB_TUNE_TMODE;

typedef enum e_stb_tune_tbwidth
{
    TUNE_TBWIDTH_8MHZ = 0,
    TUNE_TBWIDTH_7MHZ = 1,
    TUNE_TBWIDTH_6MHZ = 2,
    TUNE_TBWIDTH_5MHZ = 3,
    TUNE_TBWIDTH_10MHZ = 4,
} E_STB_TUNE_TBWIDTH;

typedef enum e_stb_tune_tconst
{
    TUNE_TCONST_QPSK = 0,
    TUNE_TCONST_QAM16 = 1,
    TUNE_TCONST_QAM64 = 2,
    TUNE_TCONST_QAM128 = 3,
    TUNE_TCONST_QAM256 = 4,
    TUNE_TCONST_UNDEFINED = 255
} E_STB_TUNE_TCONST;

typedef enum e_stb_tune_hierarchy
{
    TUNE_HIERARCHY_NONE = 0,
    TUNE_HIERARCHY_1 = 1,
    TUNE_HIERARCHY_2 = 2,
    TUNE_HIERARCHY_4 = 4,
    TUNE_HIERARCHY_8 = 8,
    TUNE_HIERARCHY_16 = 16,
    TUNE_HIERARCHY_32 = 32,
    TUNE_HIERARCHY_64 = 64,
    TUNE_HIERARCHY_128 = 128,
    TUNE_HIERARCHY_UNDEFINED = 255
} E_STB_TUNE_HIERARCHY;

typedef enum e_stb_tune_tcoderate
{
    TUNE_TCODERATE_1_2 = 0,
    TUNE_TCODERATE_2_3 = 1,
    TUNE_TCODERATE_3_4 = 2,
    TUNE_TCODERATE_5_6 = 3,
    TUNE_TCODERATE_7_8 = 4,
    TUNE_TCODERATE_3_5 = 5,
    TUNE_TCODERATE_4_5 = 6,
    TUNE_TCODERATE_UNDEFINED = 255
} E_STB_TUNE_TCODERATE;

typedef enum e_stb_tune_tguardint
{
    TUNE_TGUARDINT_1_32 = 0,
    TUNE_TGUARDINT_1_16 = 1,
    TUNE_TGUARDINT_1_8 = 2,
    TUNE_TGUARDINT_1_4 = 3,
    TUNE_TGUARDINT_1_128 = 4,
    TUNE_TGUARDINT_19_128 = 5,
    TUNE_TGUARDINT_19_256 = 6,
    TUNE_TGUARDINT_UNDEFINED = 255
} E_STB_TUNE_TGUARDINT;

/* Cable */
typedef enum  e_stb_tune_cmode
{
    TUNE_MODE_QAM_4 = 0,
    TUNE_MODE_QAM_8 = 1,
    TUNE_MODE_QAM_16 = 2,
    TUNE_MODE_QAM_32 = 3,
    TUNE_MODE_QAM_64 = 4,
    TUNE_MODE_QAM_128 = 5,
    TUNE_MODE_QAM_256 = 6,
    TUNE_MODE_QAM_UNDEFINED = 255
} E_STB_TUNE_CMODE;


/* Satellite */
typedef enum
{
    TUNE_MOD_AUTO,
    TUNE_MOD_QPSK,   /* quartenary phase shift key */
    TUNE_MOD_8PSK,   /* octenary phase shift key */
    TUNE_MOD_16QAM,  /* Not valid for DVB-S2 */
    TUNE_MOD_16APSK,
    TUNE_MOD_32APSK
} E_STB_TUNE_MODULATION;

typedef enum e_stb_tune_lnb_voltage
{
    LNB_VOLTAGE_OFF = 0,
    LNB_VOLTAGE_14V = 1,
    LNB_VOLTAGE_18V = 2
} E_STB_TUNE_LNB_VOLTAGE;

typedef enum e_stb_tune_fec
{
    TUNE_FEC_AUTOMATIC = 0,
    TUNE_FEC_1_2 = 1,
    TUNE_FEC_2_3 = 2,
    TUNE_FEC_3_4 = 3,
    TUNE_FEC_5_6 = 4,
    TUNE_FEC_7_8 = 5,
    TUNE_FEC_1_4 = 6,
    TUNE_FEC_1_3 = 7,
    TUNE_FEC_2_5 = 8,
    TUNE_FEC_8_9 = 9,
    TUNE_FEC_9_10 = 10,
    TUNE_FEC_3_5 = 11,
    TUNE_FEC_4_5 = 12
} E_STB_TUNE_FEC;

/**\brief Frontend blindscan status*/
typedef enum
{
    AM_FEND_BLIND_START,            /**< Blindscan start*/
    AM_FEND_BLIND_UPDATEPROCESS,    /**< Blindscan update process*/
    AM_FEND_BLIND_UPDATETP,         /**< Blindscan update transport program*/
    AM_FEND_BLIND_START_FAILED,     /**< Blindscan start unsuccessfully*/
    AM_FEND_BLIND_WAIT              /**< Blindscan wait*/
} E_STB_TUNE_BlindStatus_t;

typedef enum
{
    TUNER_STATE_LOCKED,
    TUNER_STATE_TIMEOUT,
    TUNER_STATE_UNKNOWN
} E_TUNER_EVENT;

typedef enum
{
    MODE_IMMEDIATE,
    MODE_DELAYED, // controlled by kernel self
} E_TUNER_SETTING_MODE;

/**\brief Blindscan event*/
typedef struct
{
    E_STB_TUNE_BlindStatus_t    status; /**< Blindscan status*/
    unsigned int freq;
    unsigned int srate;
    unsigned int process;
} E_STB_TUNE_BlindEvent_t;


/**\brief Blindscan unicable config*/
typedef struct
{
    U32BIT frequency; /* Unicable IF in khz*/
    U8BIT  unicable; /* 0-off, 1-ver_1, 2-ver_2 */
    U8BIT  channel; /* unicable Userband */
    U8BIT  bank;
    BOOLEAN position_b;
    U16BIT uncommitted;
    U16BIT committed;
} E_STB_TUNE_BlindUnicable_t;

/* Analog */
typedef enum e_stb_tune_analog_video_type
{
    TUNE_ANLG_VIDEO_PAL_I = 0,
    TUNE_ANLG_VIDEO_PAL_B = 1,
    TUNE_ANLG_VIDEO_PAL_G = 2,
    TUNE_ANLG_VIDEO_PAL_D = 3,
    TUNE_ANLG_VIDEO_PAL_K = 4,
    TUNE_ANLG_VIDEO_PAL_L = 5,
    TUNE_ANLG_VIDEO_PAL_LDASH = 6
} E_STB_TUNE_ANALOG_VIDEO_TYPE;

typedef struct s_stb_tune_signal_info
{
    S16BIT strength;  // dBm
    S16BIT dBuV;
    S16BIT dBmV;      // dBmV(x1000) for external demod
    S16BIT snr;       // Signal Noise Ratio
    U32BIT ber;       // Bit Error Rate
    S16BIT ssi;       // Signal Strength Indicator as percentage (0-100)
    S16BIT sqi;       // Signal Quality Indicator as percentage (0-100)
    S16BIT reserved[4];
} S_STB_TUNE_SIGNAL_INFO;

//---Global type defs for public use-------------------------------------------
/**\brief Blindscan callback function*/
typedef void (*STB_Tnue_BlindCallback_t) (int dev_no, E_STB_TUNE_BlindEvent_t *evt, void *user_data);

//---Global Function prototypes for public use---------------------------------

/**
 * @brief   Initialises the tuner component
 * @param   paths number of tuning paths to initialise
 */
void STB_TuneInitialise(U8BIT paths);

/**
 * @brief Assign actual ts_input_idx (obtained from driver) to aml_hw_cfg
 * @param path The tuner path to set up
 * @param frontend_fd The FD identifying the fe_name, which is required to
 *                    perform I/O control operation
 */
void STB_TuneSetActualTsInputIdx(U8BIT path, S32BIT frontend_fd);

/**
 * @brief Assign actual supported delivery system type (obtained from driver)
 *        to aml_hw_cfg
 * @param path The tuner path to set up
 * @param frontend_fd The FD identifying the fe_name, which is required to
 *                    perform I/O control operation
 */
void STB_TuneSetActualSupportedSystemType(U8BIT path, S32BIT frontend_fd);

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
void STB_TuneSetSystemType(U8BIT path, E_STB_TUNE_SYSTEM_TYPE type);

/**
 * @brief   Returns the signal type as set by STB_TuneSetTerrType or as
 *          re-written by the driver.
 * @param   path the tuner path to query
 * @return  Signal type.
 */
E_STB_TUNE_SYSTEM_TYPE STB_TuneGetSystemType(U8BIT path);

/**
 * @brief   Enables or disabled auto tuner relocking
 * @param   path the tuner path to configure
 * @param   state TRUE enables relocking, FALSE disables it
 */
void STB_TuneAutoRelock(U8BIT path, BOOLEAN state);

/**
 * @brief   Gets the signal types of the given tuner path.
 *          This will be a bitmask of supported types defined by E_STB_TUNE_SIGNAL_TYPE
 * @param   path tuner path
 * @return  the signal types supported by the given tuner
 */
U16BIT STB_TuneGetSignalType(U8BIT path);
U16BIT STB_TuneGetActualSignalType(U8BIT path);

/**
 * @brief   This function is only relevant for tuners that support more than one signal type;
 *          for tuners that don't support more than one signal type it can be a blank function.
 *          It will be called to inform the platform which of the supported signal types is being
 *          used.
 * @param   path tuner path
 * @param   type signal type that is being used for this tuner
 */
void STB_TuneSetSignalType(U8BIT path, E_STB_TUNE_SIGNAL_TYPE type);

/**
 * @brief   Returns the minimum tuner frequency in KHz
 * @param   path the tuner path to query
 * @return  minimum frequency in Khz
 */
U32BIT STB_TuneGetMinTunerFreqKHz(U8BIT path);

/**
 * @brief   Returns the maximum tuner frequency in KHz
 * @param   path the tuner path to query
 * @return  maximum frequency in Khz
 */
U32BIT STB_TuneGetMaxTunerFreqKHz(U8BIT path);

/**
 * @brief   Starts the tuner, it will then attempt to lock specified signal.
 *          Unrequired parameters can be passed as 0 (zero)
 * @param   path the tuner path to start
 * @param   freq the frequency to tune to
 * @param   srate the symbol rate to lock
 * @param   fec The forward error correction rate
 * @param   freq_off The frequency offset to use
 * @param   tmode The COFDM mode
 * @param   tbwidth The signal bandwidth
 * @param   cmode The QAM mode
 * @param   anlg_vtype The type of video for analogue tuner
 */
void STB_TuneStartTuner(U8BIT path, U32BIT freq, U32BIT srate, E_STB_TUNE_FEC fec,
                        S8BIT freq_off, E_STB_TUNE_TMODE tmode, E_STB_TUNE_TBWIDTH tbwidth,
                        E_STB_TUNE_CMODE cmode, E_STB_TUNE_ANALOG_VIDEO_TYPE anlg_vtype);

/**
 * @brief   Restarts tuner and attempts to lock to signal in StartTuner call
 * @param   path the tuner path to restart
 */
void STB_TuneRestartTuner(U8BIT path);

/**
 * @brief   Stops any locking attempt, or unlocks if locked
 * @param   path the tuner path to stop
 */
void STB_TuneStopTuner(U8BIT path);


BOOLEAN STB_TuneGetSignalInfo(U8BIT path, S_STB_TUNE_SIGNAL_INFO *singal_info);

/**
 * @brief   Returns the actual frequency of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the frequency in Hz
 */
U32BIT STB_TuneGetActualTerrFrequency(U8BIT path);

/**
 * @brief   Returns the actual freq offset of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the frequency offset in Hz
 */
S8BIT STB_TuneGetActualTerrFreqOffset(U8BIT path);

/**
 * @brief   Returns the actual mode of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the tuning mode
 */
E_STB_TUNE_TMODE STB_TuneGetActualTerrMode(U8BIT path);

/**
 * @brief   Returns the actual bandwidth of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the signal bandwidth
 */
E_STB_TUNE_TBWIDTH STB_TuneGetActualTerrBwidth(U8BIT path);

/**
 * @brief   Returns the constellation of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the constellation
 */
E_STB_TUNE_TCONST STB_TuneGetActualTerrConstellation(U8BIT path);

/**
 * @brief   Returns the hierarchy of the current terrestrial signal.
 * @param   path the tuner path to query
 * @return  the hierarchy, i.e. the maximum PLP id possibly present at the current frequency.
 */
E_STB_TUNE_HIERARCHY STB_TuneGetActualTerrHierarchy(U8BIT path);

/**
 * @brief   Returns the hierarchy of the current terrestrial signal.
 * @param   path the tuner path to query
 * @param   plp_list, out param, to store all the pip id in the current freq
 * @param   listlen,  in  param, the max numbers of pipid that can be stored in the list
 * @return  the pip number of the current frequency.
 */
U16BIT STB_TuneGetMPLPIDList(U8BIT path, U8BIT *plp_list, U16BIT listlen);

/**
 * @brief   Returns the LP code rate of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  The LP code rate
 */
E_STB_TUNE_TCODERATE STB_TuneGetActualTerrLpCodeRate(U8BIT path);

/**
 * @brief   Returns the HP code rate of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  The HP code rate
 */
E_STB_TUNE_TCODERATE STB_TuneGetActualTerrHpCodeRate(U8BIT path);

/**
 * @brief   Returns the guard interval of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the guard interval
 */
E_STB_TUNE_TGUARDINT STB_TuneGetActualTerrGuardInt(U8BIT path);

/**
 * @brief   Returns the cell id the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the cell id
 */
U16BIT STB_TuneGetActualTerrCellId(U8BIT path);

/**
 * @brief   Returns the actual bandwidth of the current isdbt signal
 * @param   path the tuner path to query
 * @return  the signal bandwidth
 */
E_STB_TUNE_TBWIDTH STB_TuneGetActualIsdbtBwidth(U8BIT path);

/**
 * @brief   Returns the type of modulation for the specified tuner
 * @param   path tuner path
 * @return  the type of modulation
 */
E_STB_TUNE_MODULATION STB_TuneGetModulation(U8BIT path);

/**
 * @brief   Sets the Physical Layer Pipe to be acquired
 * @param   path the tuner path to set up
 * @param   plp Physical Layer Pipe to be acquired
 */
void STB_TuneSetPLP(U8BIT path, U8BIT plp);

/**
 * @brief   Gets the Physical Layer Pipe to be acquired
 * @param   path  the tuner path to query
 * @return  Physical Layer Pipe to be acquired
 */
U8BIT STB_TuneGetPLP(U8BIT path);

/**
 * @brief   Enables/disables aerial power for DVB-T
 * @param   path tuner path
 * @param   enabled TRUE to enable
 */
void STB_TuneActiveAerialPower(U8BIT path, BOOLEAN enabled);

/**
 * @brief   Sets the local oscillator frequency used by the LNB
 * @param   path the tuner path to query
 */
void STB_TuneSetLOFrequency(U8BIT path, S32BIT lo_freq);

/**
 * @brief   Sets the type of modulation for the specified tuner
 * @param   path tuner path
 * @param   modulation type of modulation
 */
void STB_TuneSetModulation(U8BIT path, E_STB_TUNE_MODULATION modulation);

/**
 * @brief   Gets the LNB voltage for the given tuner
 * @param   path tuner path
 */
E_STB_TUNE_LNB_VOLTAGE STB_TuneGetLNBVoltage(U8BIT path);

/**
 * @brief   Sets the LNB voltage for the given tuner
 * @param   path tuner path
 * @param   voltage voltage setting
 */
void STB_TuneSetLNBVoltage(U8BIT path, E_STB_TUNE_LNB_VOLTAGE voltage, BOOLEAN retune, E_TUNER_SETTING_MODE mode);

/**
 * @brief   Gets the 22 kHz tone on or off
 * @param   path tuner path
 */
BOOLEAN STB_TuneGet22kState(U8BIT path);

/**
 * @brief   Turns the 22 kHz tone on or off
 * @param   path tuner path
 * @param   state TRUE to turn the tone on, FALSE to turn it off
 */
void STB_TuneSet22kState(U8BIT path, BOOLEAN state, BOOLEAN retune, E_TUNER_SETTING_MODE mode);

/**
 * @brief   Sets the 12V switch for the given tuner
 * @param   path tuner path
 * @param   state TRUE for on
 */
void STB_TuneSet12VSwitch(U8BIT path, BOOLEAN state);

/**
 * @brief   Receives a DisEqc reply
 * @param   path tuner path
 * @param   data pointer to the received data
 * @param   timeout maximum number of milliseconds to wait for a reply
 * @return  The number of bytes received
 */
U8BIT STB_TuneGetDISEQCReply(U8BIT path, U8BIT *data, U32BIT timeout);

/**
 * @brief   Sends the DisEqc message
 * @param   path - tuner path
 * @param   data - message data
 * @param   size - number of bytes in message data
 */
void STB_TuneSendDISEQCMessage(U8BIT path, U8BIT *data, U8BIT size);

/**
 * @brief   Sends the Burst message
 * @param   path - tuner path
 * @param   data - message data
 */
void STB_TuneSendBurstMessage(U8BIT path, U8BIT data);

/**
 * @brief   Receives the DisEqc reply
 * @param   path - tuner path
 * @param   data - message data
 * @param   size - number of bytes in message data
 * @param   timneout - ioctl timeout
 */
void STB_TuneReceiveDISEQCReply(U8BIT path, U8BIT *data, U8BIT size, U32BIT timeout);

/**
 * @brief   Sets the pulse limit for the east
 * @param   path tuner path
 * @param   count east limit count
 */
void STB_TuneSetPulseLimitEast(U8BIT path, U16BIT count);

/**
 * @brief   Sets the pulse limit for the west
 * @param   path tuner path
 * @param   count west limit count
 */
void STB_TuneSetPulseLimitWest(U8BIT path, U16BIT count);

void STB_TuneChangePulsePosition(U8BIT path, U16BIT count);

/**
 * @brief   Returns the current pulse position
 * @param   path tuner path
 * @return  Current puls position
 */
U16BIT STB_TuneGetPulsePosition(U8BIT path);

void STB_TuneAtPulsePosition(U8BIT path, U16BIT position);

/**
 * @brief Changes the value of skew position count
 * @param path tuner path
 * @param count skew position count
 */
void STB_TuneChangeSkewPosition(U8BIT path, U16BIT count);

/**
 * @brief   Returns the carrier signal strength as a percentage
 * @param   path tuner path
 * @param   freq carrier frequency
 * @return  Strength as a percentage
 */
U8BIT STB_TuneSatGetCarrierStrength(U8BIT path, U32BIT freq);

/**
 * @brief   Returns the actual symbol rate when a tuner has locked
 * @param   path tuner path
 * @return  Symbol rate in symbols per second
*/
U32BIT STB_TuneGetActualSymbolRate(U8BIT path);

/**
 * @brief   Returns the cable mode when the tuner has locked
 * @param   path tuner path
 * @return  QAM mode
 */
E_STB_TUNE_CMODE STB_TuneGetActualCableMode(U8BIT path);

/**
 * @brief   Returns the system type supported by the path. This function
 *          differs from STB_TuneGetSystemType which only returns T2 or S2 if
 *          the tuner is currently performing T2 or S2 operations.
 * @param   path  the tuner path to query
 */
void STB_TuneGetSupportedSystemType(U8BIT path, U8BIT *support_sys);

/**
 * @brief   Returns the maxmum tuner symbol rate
 * @param   path the tuner path to query
 * @return  maxmum tuner symbol rate
 */
U32BIT STB_TuneGetMaxTunerSymbolRate(U8BIT path);

/**
 * @brief   Returns the minimum tuner symbol rate
 * @param   path the tuner path to query
 * @return  minimum tuner symbol rate
 */
U32BIT STB_TuneGetMinTunerSymbolRate(U8BIT path);

BOOLEAN STB_TuneOpen(U8BIT path);

BOOLEAN STB_TuneIsOpened(U8BIT path);

void STB_TuneUpdateFeUsage(U8BIT path, BOOLEAN use);

BOOLEAN STB_TuneIsTvPlatform();

void STB_TuneSetSearchMode(U8BIT path, BOOLEAN mode);

BOOLEAN STB_TuneIsSearchMode(U8BIT path);

void STB_TuneAllStart();

void STB_TuneAllStop();

void STB_TuneSetFrontendFd(U8BIT path, U32BIT fe_fd);

E_STB_TUNE_SYSTEM_TYPE STB_TuneGetActualSysType(U8BIT path);

BOOLEAN STB_Tune_BlindScan(U8BIT path, E_STB_TUNE_SYSTEM_TYPE sys_type, STB_Tnue_BlindCallback_t cb, void *user_data,
                           unsigned int start_freq, unsigned int stop_freq, E_STB_TUNE_BlindUnicable_t unicable);

BOOLEAN STB_Tune_BlindContinue(U8BIT path);

BOOLEAN STB_Tune_BlindExit(U8BIT path);

void STB_Tune_BlindGetTPCount(U8BIT path, U16BIT *count);

BOOLEAN STB_Tune_BlindGetTPInfo(U8BIT path, void *para, U16BIT *count);

E_TUNER_EVENT STB_TuneGetLockStatus(U8BIT path);
int STB_TuneGetEwbsFlag(U8BIT path);


#endif //  _STBHWTUN_H

