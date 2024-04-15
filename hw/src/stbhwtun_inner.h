/*******************************************************************************
 *  Copyright @ 2023
 *
 *  inner header file only for stbhwtun.c & stbhwtun_ex.c
 *
 *
 *******************************************************************************/

/**
 * @brief   inner header file only for stbhwtun.c & stbhwtun_ex.c
 * @file    stbhwtun_inner.h
 * @date    2023-01-31
 */

#ifndef __STBHWTUN_INNER_H__
#define __STBHWTUN_INNER_H__


/*---constant definitions for this file--------------------------------------*/

#define TUNE_TASK_PRIORITY       11
#define TUNE_TASK_STACK_SIZE     8192

#define WAIT_LOCK_TIMEOUT        6000               /*keep align with Driver*/
#define TUNER_MIN_SRATE          900
#define TUNER_MAX_SRATE          45100
#define M_BS_START_FREQ          (950)				/*The start RF frequency, 950MHz*/
#define M_BS_STOP_FREQ           (2150)				/*The stop RF frequency, 2150MHz*/
#define M_BS_MAX_SYMB            (45)
#define M_BS_MIN_SYMB            (2)
#define FEND_WAIT_TIMEOUT        (500)
#define FEND_BS_MAX_CHANNEL      (512)
#define TUNER_USELESS_TIMEOUT    (10)               /*second*/
#define FEND_FL_LOCK             (1)
#define TUNER_POLLING_TIMEOUT    (50)               /*ms*/
#define TUNER_LOST_LOCK_TIMES    (40)              /*check times in search mode*/

#define BLINDSCAN_UPDATERESULT_OTHERS (0x000)/* blind scan update result others  */

/*---local typedef structs for this file-------------------------------------*/

typedef enum
{
    TUNER_IDLE,
    TUNER_TUNING,
    TUNER_LOCKED,
    TUNER_RELOCKING,
    TUNER_EXITED
} E_TUNER_STATE;

typedef struct
{
    E_STB_TUNE_TMODE tmode;
    E_STB_TUNE_TBWIDTH tbwidth;
} S_TERR_STATUS;

typedef struct
{
    E_STB_TUNE_TBWIDTH tbwidth;
} S_ISDBT_STATUS;

typedef struct
{
    U32BIT srate;
    E_STB_TUNE_CMODE cmode;
} S_CABLE_STATUS;

typedef struct
{
    U32BIT srate;
    E_STB_TUNE_FEC fec;
    S32BIT lo_freq;
    E_STB_TUNE_LNB_VOLTAGE lnb_voltage;
    E_STB_TUNE_MODULATION modulation;
    BOOLEAN use_22khz;
} S_SAT_STATUS;

/**\brief Stores the blind scan configuration parameters.*/
struct DVB_BlindScanAPI_Setting
{
    unsigned short  m_uiChannelCount;								/**< The number of channels detected thus far by the blind scan operation.*/
    struct dvb_frontend_parameters channels[FEND_BS_MAX_CHANNEL];	/**< Stores the channel information that all scan out results.*/
    struct dvbsx_blindscanevent bsEvent;							/**< Stores the information that scan out results by the blind scan procedure.*/
    struct dvbsx_blindscanpara	bsPara;								/**< Stores the blind scan parameters each blind scan procedure.*/
    struct dvbsx_singlecable_parameters singlecablePara;            /**< Stores singlecable parameters each blind scan procedure.*/
};

/**\brief Defines the status of blind scan process.*/
enum DVB_BlindScanAPI_Status
{
    DVB_BS_Status_Init = 0,							/**< = 0 Indicates that the blind scan process is initializing the parameters.*/
    DVB_BS_Status_Start = 1,							/**< = 1 Indicates that the blind scan process is starting to scan.*/
    DVB_BS_Status_Wait = 2,							/**< = 2 Indicates that the blind scan process is waiting for the completion of scanning.*/
    DVB_BS_Status_User_Process = 3,					/**< = 3 Indicates that the blind scan process is in custom code. Customer can add the callback function in this stage such as adding TP information to TP list or lock the TP for parsing PSI.*/
    DVB_BS_Status_Cancel = 4,							/**< = 4 Indicates that the blind scan process is cancelled or the blind scan have completed.*/
    DVB_BS_Status_Exit = 5,							/**< = 5 Indicates that the blind scan process have ended.*/
    DVB_BS_Status_WaitExit = 6						/**< = 6 Indicates that the blind scan process wait user exit.*/
};


typedef struct
{
    U8BIT path;

    char fe_name[24];
    int frontend_fd;

    E_TUNER_STATE state;
    U8BIT lock_flags;



    // B: Mutex & Semaphore
    void *lock;

    void *mutex;
    void *tune_sem;
    void *tune_sem_lock;
    void *tunertask_sem;
    // E: Mutex & Semaphore



    // B: Control Flag
    BOOLEAN search_mode;
    BOOLEAN stop;
    BOOLEAN auto_relock;
    BOOLEAN tuning_params_changed;

    U8BIT frontend_usage;
    // E: Control Flag



    // B: tuner parameters
    U16BIT tuner_types;                     // configured supported tuner type (using bit map)
    E_STB_TUNE_SIGNAL_TYPE signal_type;
    E_STB_TUNE_SYSTEM_TYPE sys_type;
    E_STB_TUNE_SYSTEM_TYPE tuned_sys_type;

    struct dvb_frontend_info fe_info;
    fe_delivery_system_t delivery_system;

    U32BIT freq;
    U8BIT plp_id;
    union
    {
        S_TERR_STATUS terr;
        S_CABLE_STATUS cab;
        S_SAT_STATUS sat;
        S_ISDBT_STATUS isdbt;
    } u;
    // E: tuner parameters



    // B: Blind Scan
    BOOLEAN    enable_blindscan_thread;
    void *blindscan_thread;
    STB_Tnue_BlindCallback_t blindscan_cb;
    void       *blindscan_cb_user_data;
    struct DVB_BlindScanAPI_Setting bs_setting;
    // E: Blind Scan
} S_TUNER_STATUS;

#endif

