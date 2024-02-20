#ifndef _TF_FRONTEND_H
#define _TF_FRONTEND_H
#include "techtype.h"
#ifdef __cplusplus
extern "C" {
#endif
//! C/C++
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
//!JNI


#define BUFFER_SIZE_SECTION_DEFAULT 1024 * 4L
#define INVALID_TUNER_ID 0xFFFF
#define INVALID_TUNER_PATH 0xFF


//---Constant and macro definitions for public use-----------------------------
#define SYMBOL_RATE_AUTO         0
#define INVALID_FD               -1

#define MAX_PLP_NUMBER      256

#define WRPPER_HW_EV_CLASS_TUNER             3
#define WRPPER_HW_EV_TYPE_LOCKED             2
#define WRPPER_HW_EV_TYPE_NOTLOCKED          3


//---Enumerations for public use-----------------------------------------------
typedef enum ew_stb_tune_system_type
{
    WRAPPER_TUNE_SYSTEM_TYPE_UNKNOWN = 0,
    WRAPPER_TUNE_SYSTEM_TYPE_DVBT = 1,
    WRAPPER_TUNE_SYSTEM_TYPE_DVBT2 = 2,
    WRAPPER_TUNE_SYSTEM_TYPE_DVBS = 3,
    WRAPPER_TUNE_SYSTEM_TYPE_DVBS2 = 4,
    WRAPPER_TUNE_SYSTEM_TYPE_DVBC = 5,
    WRAPPER_TUNE_SYSTEM_TYPE_ISDBT = 6,
    WRAPPER_TUNE_SYSTEM_TYPE_ANALOG = 7,
    WRAPPER_TUNE_SYSTEM_TYPE_END = 8
} EW_STB_TUNE_SYSTEM_TYPE;


typedef enum ew_stb_tune_signal_type
{
    WRAPPER_TUNE_SIGNAL_NONE = 0,
    WRAPPER_TUNE_SIGNAL_QPSK = 1,
    WRAPPER_TUNE_SIGNAL_COFDM = 2,//T
    WRAPPER_TUNE_SIGNAL_QAM = 4,
    WRAPPER_TUNE_SIGNAL_ISDBT = 5,
    WRAPPER_TUNE_SIGNAL_ANALOG = 8
} EW_STB_TUNE_SIGNAL_TYPE;

/* Terrestrial */
typedef enum ew_stb_tune_tmode
{
    WRAPPER_TUNE_MODE_COFDM_1K = 0,
    WRAPPER_TUNE_MODE_COFDM_2K = 1,
    WRAPPER_TUNE_MODE_COFDM_4K = 2,
    WRAPPER_TUNE_MODE_COFDM_8K = 3,
    WRAPPER_TUNE_MODE_COFDM_16K = 4,
    WRAPPER_TUNE_MODE_COFDM_32K = 5,
    WRAPPER_TUNE_MODE_COFDM_UNDEFINED = 255
} EW_STB_TUNE_TMODE;

typedef enum ew_stb_tune_tbwidth
{
    WRAPPER_TUNE_TBWIDTH_8MHZ = 0,
    WRAPPER_TUNE_TBWIDTH_7MHZ = 1,
    WRAPPER_TUNE_TBWIDTH_6MHZ = 2,
    WRAPPER_TUNE_TBWIDTH_5MHZ = 3,
    WRAPPER_TUNE_TBWIDTH_10MHZ = 4,
} EW_STB_TUNE_TBWIDTH;

typedef enum ew_stb_tune_tconst
{
    WRAPPER_TUNE_TCONST_QPSK = 0,
    WRAPPER_TUNE_TCONST_QAM16 = 1,
    WRAPPER_TUNE_TCONST_QAM64 = 2,
    WRAPPER_TUNE_TCONST_QAM128 = 3,
    WRAPPER_TUNE_TCONST_QAM256 = 4,
    WRAPPER_TUNE_TCONST_UNDEFINED = 255
} EW_STB_TUNE_TCONST;

typedef enum ew_stb_tune_hierarchy
{
    WRAPPER_TUNE_HIERARCHY_NONE = 0,
    WRAPPER_TUNE_HIERARCHY_1 = 1,
    WRAPPER_TUNE_HIERARCHY_2 = 2,
    WRAPPER_TUNE_HIERARCHY_4 = 4,
    WRAPPER_TUNE_HIERARCHY_8 = 8,
    WRAPPER_TUNE_HIERARCHY_16 = 16,
    WRAPPER_TUNE_HIERARCHY_32 = 32,
    WRAPPER_TUNE_HIERARCHY_64 = 64,
    WRAPPER_TUNE_HIERARCHY_128 = 128,
    WRAPPER_TUNE_HIERARCHY_UNDEFINED = 255
} EW_STB_TUNE_HIERARCHY;

typedef enum ew_stb_tune_tcoderate
{
    WRAPPER_TUNE_TCODERATE_1_2 = 0,
    WRAPPER_TUNE_TCODERATE_2_3 = 1,
    WRAPPER_TUNE_TCODERATE_3_4 = 2,
    WRAPPER_TUNE_TCODERATE_5_6 = 3,
    WRAPPER_TUNE_TCODERATE_7_8 = 4,
    WRAPPER_TUNE_TCODERATE_3_5 = 5,
    WRAPPER_TUNE_TCODERATE_4_5 = 6,
    WRAPPER_TUNE_TCODERATE_UNDEFINED = 255
} EW_STB_TUNE_TCODERATE;

typedef enum ew_stb_tune_tguardint
{
    WRAPPER_TUNE_TGUARDINT_1_32 = 0,
    WRAPPER_TUNE_TGUARDINT_1_16 = 1,
    WRAPPER_TUNE_TGUARDINT_1_8 = 2,
    WRAPPER_TUNE_TGUARDINT_1_4 = 3,
    WRAPPER_TUNE_TGUARDINT_1_128 = 4,
    WRAPPER_TUNE_TGUARDINT_19_128 = 5,
    WRAPPER_TUNE_TGUARDINT_19_256 = 6,
    WRAPPER_TUNE_TGUARDINT_UNDEFINED = 255
} EW_STB_TUNE_TGUARDINT;

/* Cable */
typedef enum  ew_stb_tune_cmode
{
    WRAPPER_TUNE_MODE_QAM_4 = 0,
    WRAPPER_TUNE_MODE_QAM_8 = 1,
    WRAPPER_TUNE_MODE_QAM_16 = 2,
    WRAPPER_TUNE_MODE_QAM_32 = 3,
    WRAPPER_TUNE_MODE_QAM_64 = 4,
    WRAPPER_TUNE_MODE_QAM_128 = 5,
    WRAPPER_TUNE_MODE_QAM_256 = 6,
    WRAPPER_TUNE_MODE_QAM_UNDEFINED = 255
} EW_STB_TUNE_CMODE;


/* Satellite */
typedef enum
{
    WRAPPER_TUNE_MOD_AUTO,
    WRAPPER_TUNE_MOD_QPSK,   /* quartenary phase shift key */
    WRAPPER_TUNE_MOD_8PSK,   /* octenary phase shift key */
    WRAPPER_TUNE_MOD_16QAM,  /* Not valid for DVB-S2 */
    WRAPPER_TUNE_MOD_16APSK,
    WRAPPER_TUNE_MOD_32APSK
} EW_STB_TUNE_MODULATION;

typedef enum ew_stb_tune_lnb_voltage
{
    WRAPPER_LNB_VOLTAGE_OFF = 0,
    WRAPPER_LNB_VOLTAGE_14V = 1,
    WRAPPER_LNB_VOLTAGE_18V = 2
} EW_STB_TUNE_LNB_VOLTAGE;

typedef enum ew_stb_tune_fec
{
    WRAPPER_TUNE_FEC_AUTOMATIC = 0,
    WRAPPER_TUNE_FEC_1_2 = 1,
    WRAPPER_TUNE_FEC_2_3 = 2,
    WRAPPER_TUNE_FEC_3_4 = 3,
    WRAPPER_TUNE_FEC_5_6 = 4,
    WRAPPER_TUNE_FEC_7_8 = 5,
    WRAPPER_TUNE_FEC_1_4 = 6,
    WRAPPER_TUNE_FEC_1_3 = 7,
    WRAPPER_TUNE_FEC_2_5 = 8,
    WRAPPER_TUNE_FEC_8_9 = 9,
    WRAPPER_TUNE_FEC_9_10 = 10,
    WRAPPER_TUNE_FEC_3_5 = 11,
    WRAPPER_TUNE_FEC_4_5 = 12
} EW_STB_TUNE_FEC;

/**\brief Frontend blindscan status*/
typedef enum
{
    WRAPPER_AM_FEND_BLIND_START,            /**< Blindscan start*/
    WRAPPER_AM_FEND_BLIND_UPDATEPROCESS,    /**< Blindscan update process*/
    WRAPPER_AM_FEND_BLIND_UPDATETP,         /**< Blindscan update transport program*/
    WRAPPER_AM_FEND_BLIND_START_FAILED,     /**< Blindscan start unsuccessfully*/
    WRAPPER_AM_FEND_BLIND_WAIT              /**< Blindscan wait*/
} EW_STB_TUNE_BlindStatus_t;

typedef enum
{
    WRAPPER_TUNER_STATE_LOCKED,
    WRAPPER_TUNER_STATE_TIMEOUT,
    WRAPPER_TUNER_STATE_UNKNOWN
} EW_TUNER_EVENT;


/**\brief Blindscan event*/
typedef struct
{
    EW_STB_TUNE_BlindStatus_t    status; /**< Blindscan status*/
    unsigned int freq;
    unsigned int srate;
    unsigned int process;
} EW_STB_TUNE_BlindEvent_t;


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
} EW_STB_TUNE_BlindUnicable_t;

/* Analog */
typedef enum ew_stb_tune_analog_video_type
{
    WRAPPER_TUNE_ANLG_VIDEO_PAL_I = 0,
    WRAPPER_TUNE_ANLG_VIDEO_PAL_B = 1,
    WRAPPER_TUNE_ANLG_VIDEO_PAL_G = 2,
    WRAPPER_TUNE_ANLG_VIDEO_PAL_D = 3,
    WRAPPER_TUNE_ANLG_VIDEO_PAL_K = 4,
    WRAPPER_TUNE_ANLG_VIDEO_PAL_L = 5,
    WRAPPER_TUNE_ANLG_VIDEO_PAL_LDASH = 6
} EW_STB_TUNE_ANALOG_VIDEO_TYPE;

typedef enum ew_tune_terr_type
{
    E_TERR_TYPE_UNKNOWN,
    E_TERR_TYPE_ANALOG,
    E_TERR_TYPE_ATSC,
    E_TERR_TYPE_ATSC3,
    E_TERR_TYPE_DVBC,
    E_TERR_TYPE_DVBS,
    E_TERR_TYPE_DVBT,
    E_TERR_TYPE_ISDBS,
    E_TERR_TYPE_ISDBS3,
    E_TERR_TYPE_ISDBT,
    E_TERR_TYPE_DTMB,
    E_TERR_TYPE_IPTV
} E_TTYPE;

typedef void (* Wrapper_SendEvent) (BOOLEAN repeat, U16BIT event_class, U16BIT event_type, void *data, U32BIT data_size);

BOOLEAN tuner_getFrontendIds(U8BIT path);
typedef void (* Wrapper_Tune_BlindCallback_t) (int dev_no, EW_STB_TUNE_BlindEvent_t *evt, void *user_data);


//---Global Function prototypes for public use---------------------------------

/**
 * @brief   Initialises the tuner component
 * @param   paths number of tuning paths to initialise
 */
void Wrapper_TuneInitialise(U8BIT paths);

/**
 * @brief Assign actual ts_input_idx (obtained from driver) to aml_hw_cfg
 * @param path The tuner path to set up
 * @param frontend_fd The FD identifying the fe_name, which is required to
 *                    perform I/O control operation
 */
void Wrapper_TuneSetActualTsInputIdx(U8BIT path, S32BIT frontend_fd);

/**
 * @brief Assign actual supported delivery system type (obtained from driver)
 *        to aml_hw_cfg
 * @param path The tuner path to set up
 * @param frontend_fd The FD identifying the fe_name, which is required to
 *                    perform I/O control operation
 */
void Wrapper_TuneSetActualSupportedSystemType(U8BIT path, S32BIT frontend_fd);

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
void Wrapper_TuneSetSystemType(U8BIT path, EW_STB_TUNE_SYSTEM_TYPE type);

/**
 * @brief   Returns the signal type as set by STB_TuneSetTerrType or as
 *          re-written by the driver.
 * @param   path the tuner path to query
 * @return  Signal type.
 */
EW_STB_TUNE_SYSTEM_TYPE Wrapper_TuneGetSystemType(U8BIT path);

/**
 * @brief   Enables or disabled auto tuner relocking
 * @param   path the tuner path to configure
 * @param   state TRUE enables relocking, FALSE disables it
 */
void Wrapper_TuneAutoRelock(U8BIT path, BOOLEAN state);

/**
 * @brief   Gets the signal types of the given tuner path.
 *          This will be a bitmask of supported types defined by E_STB_TUNE_SIGNAL_TYPE
 * @param   path tuner path
 * @return  the signal types supported by the given tuner
 */
EW_STB_TUNE_SIGNAL_TYPE Wrapper_TuneGetSignalType(U8BIT path);
E_TTYPE Wrapper_TuneGetActualSignalType(U8BIT path);

/**
 * @brief   This function is only relevant for tuners that support more than one signal type;
 *          for tuners that don't support more than one signal type it can be a blank function.
 *          It will be called to inform the platform which of the supported signal types is being
 *          used.
 * @param   path tuner path
 * @param   type signal type that is being used for this tuner
 */
void Wrapper_TuneSetSignalType(U8BIT path, EW_STB_TUNE_SIGNAL_TYPE type);

/**
 * @brief   Returns the minimum tuner frequency in KHz
 * @param   path the tuner path to query
 * @return  minimum frequency in Khz
 */
S64BIT Wrapper_TuneGetMinTunerFreqKHz(U8BIT path);

/**
 * @brief   Returns the maximum tuner frequency in KHz
 * @param   path the tuner path to query
 * @return  maximum frequency in Khz
 */
S64BIT Wrapper_TuneGetMaxTunerFreqKHz(U8BIT path);

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
void Wrapper_TuneStartTuner(U8BIT path, U32BIT freq, U32BIT srate, EW_STB_TUNE_FEC fec,
                                   EW_STB_TUNE_TMODE tmode, EW_STB_TUNE_TBWIDTH tbwidth,
                                   EW_STB_TUNE_CMODE cmode);

/**
 * @brief   Restarts tuner and attempts to lock to signal in StartTuner call
 * @param   path the tuner path to restart
 */
void Wrapper_TuneRestartTuner(U8BIT path);

/**
 * @brief   Stops any locking attempt, or unlocks if locked
 * @param   path the tuner path to stop
 */
void Wrapper_TuneStopTuner(U8BIT path);

/**
 * @brief   Returns the current signal strength
 * @param   path the tuner path to query
 * @return  the signal strength as percentage of maximum (0-100)
 */
U32BIT Wrapper_TuneGetSignalStrength(U8BIT path);

/**
 * @brief   Returns the current data integrity
 * @param   path the tuner path to query
 * @return  the signal ber (*e10)
 */
U32BIT Wrapper_TuneGetDataIntegrity(U8BIT path);

/**
 * @brief   Returns the current signal quality
 * @param   path the tuner path to query
 * @return  the signal quality
 * @todo     Confirm DVB API BER units
 */
U32BIT Wrapper_TuneGetSignalQuality(U8BIT path);

/**
 * @brief   Returns the actual frequency of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the frequency in Hz
 */
U32BIT Wrapper_TuneGetActualTerrFrequency(U8BIT path);

/**
 * @brief   Returns the actual freq offset of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the frequency offset in Hz
 */
S8BIT Wrapper_TuneGetActualTerrFreqOffset(U8BIT path);

/**
 * @brief   Returns the actual mode of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the tuning mode
 */
EW_STB_TUNE_TMODE Wrapper_TuneGetActualTerrMode(U8BIT path);

/**
 * @brief   Returns the actual bandwidth of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the signal bandwidth
 */
EW_STB_TUNE_TBWIDTH Wrapper_TuneGetActualTerrBwidth(U8BIT path);

/**
 * @brief   Returns the constellation of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the constellation
 */
EW_STB_TUNE_TCONST Wrapper_TuneGetActualTerrConstellation(U8BIT path);

/**
 * @brief   Returns the hierarchy of the current terrestrial signal.
 * @param   path the tuner path to query
 * @return  the hierarchy, i.e. the maximum PLP id possibly present at the current frequency.
 */
EW_STB_TUNE_HIERARCHY Wrapper_TuneGetActualTerrHierarchy(U8BIT path);

/**
 * @brief   Returns the LP code rate of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  The LP code rate
 */
EW_STB_TUNE_TCODERATE Wrapper_TuneGetActualTerrLpCodeRate(U8BIT path);

/**
 * @brief   Returns the HP code rate of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  The HP code rate
 */
EW_STB_TUNE_TCODERATE Wrapper_TuneGetActualTerrHpCodeRate(U8BIT path);

/**
 * @brief   Returns the guard interval of the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the guard interval
 */
EW_STB_TUNE_TGUARDINT Wrapper_TuneGetActualTerrGuardInt(U8BIT path);

/**
 * @brief   Returns the cell id the current terrestrial signal
 * @param   path the tuner path to query
 * @return  the cell id
 */
U16BIT Wrapper_TuneGetActualTerrCellId(U8BIT path);

/**
 * @brief   Sets the Physical Layer Pipe to be acquired
 * @param   path the tuner path to set up
 * @param   plp Physical Layer Pipe to be acquired
 */
void Wrapper_TuneSetPLP(U8BIT path, U8BIT plp);

/**
 * @brief   Gets the Physical Layer Pipe to be acquired
 * @param   path  the tuner path to query
 * @return  Physical Layer Pipe to be acquired
 */
U8BIT Wrapper_TuneGetPLP(U8BIT path);

U16BIT Wrapper_TuneGetMPLPIDList(U8BIT path, U8BIT *plp_list, U16BIT listlen);

/**
 * @brief   Returns the actual symbol rate when a tuner has locked
 * @param   path tuner path
 * @return  Symbol rate in symbols per second
*/
U32BIT Wrapper_TuneGetActualSymbolRate(U8BIT path);

/**
 * @brief   Returns the system type supported by the path. This function
 *          differs from STB_TuneGetSystemType which only returns T2 or S2 if
 *          the tuner is currently performing T2 or S2 operations.
 * @param   path  the tuner path to query
 */
void Wrapper_TuneGetSupportedSystemType(U8BIT path, U8BIT *support_sys);

/**
 * @brief   Returns the maxmum tuner symbol rate
 * @param   path the tuner path to query
 * @return  maxmum tuner symbol rate
 */
U32BIT Wrapper_TuneGetMaxTunerSymbolRate(U8BIT path);

/**
 * @brief   Returns the minimum tuner symbol rate
 * @param   path the tuner path to query
 * @return  minimum tuner symbol rate
 */
U32BIT Wrapper_TuneGetMinTunerSymbolRate(U8BIT path);

BOOLEAN Wrapper_TuneOpen(U8BIT path);

BOOLEAN Wrapper_TuneIsOpened(U8BIT path);

void Wrapper_TuneUpdateFeUsage(U8BIT path, BOOLEAN use);

BOOLEAN Wrapper_TuneIsTvPlatform();

void Wrapper_TuneSetSearchMode(U8BIT path, BOOLEAN mode);

BOOLEAN Wrapper_TuneIsSearchMode(U8BIT path);

void Wrapper_TuneAllStart();

void Wrapper_TuneAllStop();

EW_TUNER_EVENT Wrapper_TuneGetLockStatus(U8BIT path);

void Wrapper_RegisterCallback(Wrapper_SendEvent callback);

EW_STB_TUNE_CMODE Wrapper_TuneGetActualCableMode(U8BIT path);

EW_STB_TUNE_MODULATION Wrapper_TuneGetModulation(U8BIT path);
void Wrapper_TuneSetModulation(U8BIT path, EW_STB_TUNE_MODULATION modulation);
void Wrapper_TuneSetLOFrequency(U8BIT path, S32BIT lo_freq);
EW_STB_TUNE_LNB_VOLTAGE Wrapper_TuneGetLNBVoltage(U8BIT path);
void Wrapper_TuneSetLNBVoltage(U8BIT path, EW_STB_TUNE_LNB_VOLTAGE voltage, BOOLEAN retune);
void Wrapper_TuneSetVoltageInterface(U8BIT path, EW_STB_TUNE_LNB_VOLTAGE voltage);
BOOLEAN Wrapper_TuneGet22kState(U8BIT path);
void Wrapper_TuneSet22kState(U8BIT path, BOOLEAN state, BOOLEAN retune);
BOOLEAN Wrapper_TuneSetTone(U8BIT path, BOOLEAN use_22khz);
void Wrapper_TuneSendDISEQCMessage(U8BIT path, U8BIT *data, U8BIT size);
void Wrapper_TuneSendBurstMessage(U8BIT path, U8BIT data);
void Wrapper_TuneReceiveDISEQCReply(U8BIT path, U8BIT *data, U8BIT size, U32BIT timeout);

BOOLEAN Wrapper_Tune_BlindScan(U8BIT path, E_TTYPE sys_type, Wrapper_Tune_BlindCallback_t cb, void *user_data,
                                      unsigned int start_freq, unsigned int stop_freq, EW_STB_TUNE_BlindUnicable_t unicable);
BOOLEAN Wrapper_Tune_BlindExit(U8BIT path);
void Wrapper_Tune_BlindGetTPCount(U8BIT path, U16BIT *count);
BOOLEAN Wrapper_Tune_BlindGetTPInfo(U8BIT path, U32BIT** freq, U32BIT** srate, U16BIT *count);

#ifdef __cplusplus
}
#endif

#endif
