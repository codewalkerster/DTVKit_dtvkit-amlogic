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
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>


/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"

#include "stbhwdef.h"
#include "stbhwtun.h"
#include "stbhwmem.h"
#include "stbhwos.h"
#define CONFIG_AMLOGIC_DVB_COMPAT
/* third party header files */
#include "linux/dvb/frontend.h"


/*---Macro Definitions for this file-----------------------------------------*/
#ifdef TUNER_DEBUG
   #define TUN_DBG(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define TUN_DBG(x,...)
#endif

#define TUN_ERR(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

/*---constant definitions for this file--------------------------------------*/

#define TUNE_TASK_PRIORITY       11
#define TUNE_TASK_STACK_SIZE     8192

#define WAIT_LOCK_TIMEOUT        3000


/*---local typedef structs for this file-------------------------------------*/
typedef enum
{
   TUNER_IDLE,
   TUNER_TUNING,
   TUNER_LOCKED,
   TUNER_RELOCKING
} E_TUNER_STATE;

typedef enum
{
   TUNER_STATE_LOCKED,
   TUNER_STATE_TIMEOUT,
   TUNER_STATE_UNKNOW
} E_TUNER_EVENT;

typedef struct
{
   E_STB_TUNE_TMODE tmode;
   E_STB_TUNE_TBWIDTH tbwidth;
} S_TERR_STATUS;

typedef struct
{
   U32BIT srate;
   E_STB_TUNE_CMODE cmode;
} S_CABLE_STATUS;

typedef struct
{
   U32BIT srate;
   E_STB_TUNE_FEC fec;
   U16BIT lo_freq;
   E_STB_TUNE_LNB_VOLTAGE lnb_voltage;
   E_STB_TUNE_MODULATION modulation;
   BOOLEAN use_22khz;
} S_SAT_STATUS;

typedef struct
{
   U8BIT path;

   char fe_name[24];
   E_TUNER_STATE state;
   BOOLEAN stop;
   E_STB_TUNE_SYSTEM_TYPE tuned_sys_type;

   void *mutex;
   void *tune_sem;

   int frontend_fd;

   struct dvb_frontend_info fe_info;
   fe_delivery_system_t delivery_system;

   U16BIT tuner_types;
   E_STB_TUNE_SIGNAL_TYPE signal_type;
   E_STB_TUNE_SYSTEM_TYPE sys_type;

   BOOLEAN auto_relock;
   BOOLEAN tuning_params_changed;

   U32BIT freq;
   union
   {
      S_TERR_STATUS terr;
      S_CABLE_STATUS cab;
      S_SAT_STATUS sat;
   } u;
} S_TUNER_STATUS;


/*---local (static) variable declarations for this file----------------------*/
static S_TUNER_STATUS *tuner_status = NULL;
static U8BIT num_paths;


/*---local function prototypes for this file---------------------------------*/
static BOOLEAN OpenTuner(S_TUNER_STATUS *tstatus);
static void CloseTuner(S_TUNER_STATUS *tstatus);
static BOOLEAN StartTune(S_TUNER_STATUS *tstatus);
static BOOLEAN IsTunerLocked(S_TUNER_STATUS *tstatus);
static void TunerTask(void *param);
static void ClearTuner(S_TUNER_STATUS *tstatus);
static BOOLEAN SetSysType(S_TUNER_STATUS *tstatus, E_STB_TUNE_SIGNAL_TYPE sig_type);
static BOOLEAN IsDiffSysType(S_TUNER_STATUS * tstatus);
static E_TUNER_EVENT GetTunerLockStatus(U32BIT frontend_fd);




/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialises the tuner component
 * @param   paths number of tuning paths to initialise
 */
void STB_TuneInitialise(U8BIT paths)
{
   char fe_name[24];
   struct stat file_status;
   BOOLEAN adapter_found;
   U8BIT i;

   FUNCTION_START(STB_TuneInitialise);
   USE_UNWANTED_PARAM(paths);

   /* Find out how many tuners are available */
   for (num_paths = 0, adapter_found = TRUE; adapter_found && (num_paths < aml_hw_cfg.tuner_num); )
   {
      snprintf(fe_name, sizeof(fe_name), "/dev/dvb0.frontend%u", num_paths);
      if (stat(fe_name, &file_status) == 0)
      {
         TUN_DBG("found %s", fe_name);
         num_paths++;
      }
      else
      {
         adapter_found = FALSE;
      }
   }

   if (num_paths != 0)
   {
      tuner_status = (S_TUNER_STATUS *)STB_MEMGetSysRAM(sizeof(S_TUNER_STATUS) * num_paths);
      if (tuner_status != NULL)
      {
         memset(tuner_status, 0, sizeof(S_TUNER_STATUS) * num_paths);

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

            if (STB_OSCreateTask(TunerTask, (void *)&tuner_status[i], TUNE_TASK_STACK_SIZE,
               TUNE_TASK_PRIORITY, (U8BIT *)"TunerTask") == NULL)
            {
               TUN_ERR("Failed to create task for tuner %u", i);
            }
            else
            {
               OpenTuner(&tuner_status[i]);
            }
         }
      }
   }
   else
   {
      TUN_ERR("No tuners found!");
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

      TUN_DBG("%u: current type=%u, new type=%u frontend_fd:%d", path, tstatus->signal_type, type, tstatus->frontend_fd);

      if (tstatus->signal_type != type)
      {
         if (tstatus->frontend_fd != INVALID_FD)
         {
            STB_OSMutexLock(tstatus->mutex);
            state = tstatus->state;
            STB_OSMutexUnlock(tstatus->mutex);

            if (state != TUNER_IDLE)
            {
               STB_TuneStopTuner(path);
            }

            tstatus->signal_type = TUNE_SIGNAL_NONE;
         }

         if (type != TUNE_SIGNAL_NONE && ((tstatus->tuner_types & type) != 0) && SetSysType(tstatus, type))
         {
            tstatus->signal_type = type;
         }
      }
   }

   FUNCTION_FINISH(STB_TuneSetSignalType);
}

static BOOLEAN SetFeProperty(int fe_fd, E_STB_TUNE_SYSTEM_TYPE tuned_sys_type)
{
    int fe_mode = SYS_UNDEFINED;

    switch (tuned_sys_type)
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
       default:
            TUN_ERR("not support type:%d", tuned_sys_type);
            return FALSE;
    }

    struct dtv_property p = {.cmd = DTV_DELIVERY_SYSTEM, .u.data = fe_mode};
    struct dtv_properties props = {.num = 1, .props = &p};
    if (ioctl(fe_fd, FE_SET_PROPERTY, &props) == -1) {
        TUN_ERR("Failed to FE_SET_PROPERTY, errno %d", errno);
        return FALSE;
    }

    return TRUE;
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

   FUNCTION_START(STB_TuneStartTuner);
   USE_UNWANTED_PARAM(freq_off);
   USE_UNWANTED_PARAM(anlg_vtype);

   if (path < num_paths)
   {
      tstatus = &tuner_status[path];

      TUN_DBG("%u: freq %lu, sys_type %s", path, freq,
        ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT) ? "DVB-T" :
        ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2) ? "DVB-T2" :
        ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS) ? "DVB-S" :
        ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS2) ? "DVB-S2" :
        ((tstatus->signal_type == TUNE_SIGNAL_QAM) ? "DVB-C" : "UNSUPPORTED"))))));

      if (((tstatus->signal_type == TUNE_SIGNAL_COFDM) &&
         ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT) ||
         ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2) && (tstatus->delivery_system == SYS_DVBT2)))) ||
         ((tstatus->signal_type == TUNE_SIGNAL_QPSK) &&
         ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS) ||
         ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS2) && (tstatus->delivery_system == SYS_DVBS2)))) ||
         ((tstatus->signal_type == TUNE_SIGNAL_QAM) && (tstatus->delivery_system == SYS_DVBC_ANNEX_A)))
      {
         start_tuning = FALSE;

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

         STB_OSMutexLock(tstatus->mutex);
         state = tstatus->state;
         STB_OSMutexUnlock(tstatus->mutex);

         switch (tstatus->signal_type)
         {
            case TUNE_SIGNAL_COFDM:
               if ((tstatus->u.terr.tmode != tmode) || (tstatus->u.terr.tbwidth != tbwidth)/* ||
                  (state != TUNER_LOCKED)*/)
               {
                  start_tuning = TRUE;
                  tstatus->u.terr.tmode = tmode;
                  tstatus->u.terr.tbwidth = tbwidth;
               }
               break;

            case TUNE_SIGNAL_QAM:
               if ((tstatus->u.cab.cmode != cmode) || (tstatus->u.cab.srate != srate)/* ||
                  (state != TUNER_LOCKED)*/)
               {
                  start_tuning = TRUE;
                  tstatus->u.cab.cmode = cmode;
                  tstatus->u.cab.srate = srate;
               }
               break;

            case TUNE_SIGNAL_QPSK:
               if ((tstatus->u.sat.fec != fec) || (tstatus->u.sat.srate != srate)/* ||
                  (state != TUNER_LOCKED)*/)
               {
                  start_tuning = TRUE;
                  tstatus->u.sat.fec = fec;
                  tstatus->u.sat.srate = srate;
               }
               break;

            default:
               break;
         }

         if (start_tuning || tstatus->tuning_params_changed || GetTunerLockStatus(tstatus->frontend_fd) != TUNER_STATE_LOCKED)
         {
            TUN_DBG("start_tuning: %d tuning_params_changed:%d", start_tuning,tstatus->tuning_params_changed);
            if (state != TUNER_IDLE)
            {
               STB_TuneStopTuner(path);
            }

            tstatus->tuning_params_changed = FALSE;

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
         else
         {
            /* Already tuned to the required transport */
            TUN_DBG("%u: Already tuned", tstatus->path);
            if (state == TUNER_IDLE)
            {
               STB_OSMutexLock(tstatus->mutex);
               tstatus->state = TUNER_LOCKED;
               state = tstatus->state;
               STB_OSMutexUnlock(tstatus->mutex);
               STB_OSSemaphoreSignal(tstatus->tune_sem);
            }
            STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_LOCKED, &tstatus->path, sizeof(U8BIT));
         }
      }
      else
      {
         TUN_ERR("%u: system type %u not supported", tstatus->path, tstatus->sys_type);

         STB_OSMutexLock(tstatus->mutex);
         state = tstatus->state;
         STB_OSMutexUnlock(tstatus->mutex);

         if (state != TUNER_IDLE)
         {
            STB_TuneStopTuner(path);
         }

         STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path, sizeof(U8BIT));
      }
   }

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

   if (path < num_paths)
   {
      tstatus = &tuner_status[path];

      STB_OSMutexLock(tstatus->mutex);
      state = tstatus->state;
      STB_OSMutexUnlock(tstatus->mutex);

      if (state != TUNER_IDLE)
      {
         TUN_DBG("%u: Stopping tuning...", tstatus->path);

         STB_OSMutexLock(tstatus->mutex);
         tstatus->stop = TRUE;
         STB_OSMutexUnlock(tstatus->mutex);

         while (state != TUNER_IDLE)
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

         tstatus->tuned_sys_type = TUNE_SYSTEM_TYPE_UNKNOWN;
      }

      TUN_DBG("%u: Tuner stopped", tstatus->path);
   }

   FUNCTION_FINISH(STB_TuneStopTuner);
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

/**
 * @brief   Returns the current signal strength
 * @param   path the tuner path to query
 * @return  the signal strength as percentage of maximum (0-100)
 */
U8BIT STB_TuneGetSignalStrength(U8BIT path)
{
   U8BIT retval;
   uint16_t strength;

   FUNCTION_START(STB_TuneGetSignalStrength);

   retval = 0;

   if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
   {
      if (IsTunerLocked(&tuner_status[path]))
      {
         /* New method of reading signal strength not supported, so use the old API */
         if (ioctl(tuner_status[path].frontend_fd, FE_READ_SIGNAL_STRENGTH, &strength) >= 0)
         {
            /* Strength is returned as a percentage */
            retval = (U8BIT)strength;
            TUN_DBG("%u: %u%%", path, retval);
         }
         else
         {
            TUN_ERR("%u: Failed to get signal strength, errno %d", path, errno);
         }
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
U8BIT STB_TuneGetDataIntegrity(U8BIT path)
{
   U8BIT retval;
   __u32 ber;

   FUNCTION_START(STB_TuneGetDataIntegrity);

   retval = 0;

   if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
   {
      if (IsTunerLocked(&tuner_status[path]))
      {
         if (ioctl(tuner_status[path].frontend_fd, FE_READ_BER, &ber) >= 0)
         {
             retval = (U8BIT)ber;
             TUN_DBG("%u: BER=%u%%", path, retval);
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

/**
 * @brief   Returns the current signal quality
 * @param   path the tuner path to query
 * @return  the signal quality
 * @todo     Confirm DVB API BER units
 */
U8BIT STB_TuneGetSignalQuality(U8BIT path)
{
   U8BIT retval;
   uint16_t quality;

   FUNCTION_START(STB_TuneGetSignalQuality);

   retval = 0;

   if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
   {
      if (IsTunerLocked(&tuner_status[path]))
      {
         if (ioctl(tuner_status[path].frontend_fd, FE_READ_SNR, &quality) >= 0)
         {
             retval = (U8BIT)quality;
             TUN_DBG("%u: Quality=%u%%", path, retval);
         }
         else
         {
            TUN_ERR("%u: FE_READ_SNRfailed, errno %d", path, errno);
         }
      }
   }

   FUNCTION_FINISH(STB_TuneGetSignalQuality);

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
   U8BIT retval;
   struct dtv_property cmd;
   struct dtv_properties props;
   uint8_t plp_ids[256];

   FUNCTION_START(STB_TuneGetActualTerrHierarchy);

   retval = TUNE_THIERARCHY_NONE;

   if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD))
   {
      if (GetTunerLockStatus(tuner_status[path].frontend_fd) == TUNER_STATE_LOCKED)
      {
         memset(&cmd, 0, sizeof(struct dtv_property));

         if (tuner_status[path].sys_type == TUNE_SYSTEM_TYPE_DVBT2)
         {
            cmd.cmd = DTV_DVBT2_PLP_ID;
            cmd.u.buffer.reserved1[1] = UINT_MAX;
            cmd.u.buffer.reserved2 = plp_ids;

            props.num = 1;
            props.props = &cmd;

            if (ioctl(tuner_status[path].frontend_fd, FE_GET_PROPERTY, &props) >= 0)
            {
               retval = cmd.u.buffer.reserved1[0];
               if (retval != 0)
               {
                  /* Return the value of the max PLP id */
                  retval--;
               }

               TUN_DBG("%u: Num PLPs=%u", path, retval);
            }
            else
            {
               TUN_ERR("%u: Failed to get number of PLPs, errno %d", path, errno);
               retval = TUNE_THIERARCHY_NONE;
            }
         }
         else
         {
            cmd.cmd = DTV_HIERARCHY;
            props.num = 1;
            props.props = &cmd;

            if (ioctl(tuner_status[path].frontend_fd, FE_GET_PROPERTY, &props) >= 0)
            {
               TUN_DBG("%u: hierarchy=%lu", path, cmd.u.data);
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
               TUN_ERR("%u: Failed to get hierarchy, errno %d", path, errno);
            }
         }
      }
   }

   FUNCTION_FINISH(STB_TuneGetActualTerrHierarchy);

   return retval;
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

   if ((path < num_paths) && (tuner_status[path].signal_type == TUNE_SIGNAL_QPSK))
   {
      if (tuner_status[path].u.sat.lnb_voltage != voltage)
      {
         tuner_status[path].u.sat.lnb_voltage = voltage;
         tuner_status[path].tuning_params_changed = TRUE;
      }
   }

   FUNCTION_FINISH(STB_TuneSetLNBVoltage);
}
void STB_TuneSetFrontendFd(U8BIT path, U32BIT fe_fd)
{
   tuner_status[path].frontend_fd = fe_fd;
   TUN_DBG("STB_TuneSetFrontendFd path:%d fd:%d", path, tuner_status[path].frontend_fd);
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

    if (ioctl(tuner_status[path].frontend_fd, FE_SET_VOLTAGE, voltage) == -1)
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

/**
 * @brief   Turns the 22 kHz tone on or off
 * @param   path tuner path
 * @param   state TRUE to turn the tone on, FALSE to turn it off
 */
void STB_TuneSet22kState(U8BIT path, BOOLEAN state)
{
   FUNCTION_START(STB_TuneSet22kState);

   if ((path < num_paths) && (tuner_status[path].signal_type == TUNE_SIGNAL_QPSK))
   {
      if (tuner_status[path].u.sat.use_22khz != state)
      {
         tuner_status[path].u.sat.use_22khz = state;
         tuner_status[path].tuning_params_changed = TRUE;
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
   struct dvb_diseqc_master_cmd cmd;
   memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

   for (int i = 0; i < size; i++)
   {
       cmd.msg[i] = data[i];
       TUN_DBG("STB_TuneSendDISEQCMessage cmd:0x%x", data[i]);
   }
   cmd.msg_len = size;

   if (ioctl(tuner_status[path].frontend_fd, FE_DISEQC_SEND_MASTER_CMD, &cmd) == -1)
   {
       TUN_DBG("ioctl FE_DISEQC_SEND_MASTER_CMD failed, path:%d error:%d", path, errno);
   }

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
void STB_TuneSetLOFrequency(U8BIT tuner, U16BIT lo_freq)
{
   FUNCTION_START(STB_TuneSetLOFrequency);

   if (tuner < num_paths)
   {
      tuner_status[tuner].u.sat.lo_freq = lo_freq;
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
 * @brief   Sets the Physical Layer Pipe to be acquired
 * @param   path the tuner path to set up
 * @param   plp Physical Layer Pipe to be acquired
 */
void STB_TuneSetPLP(U8BIT path, U8BIT plp)
{
   struct dtv_property cmd;
   struct dtv_properties props;

   FUNCTION_START(STB_TuneSetPLP);

   if ((path < num_paths) && (tuner_status[path].frontend_fd != INVALID_FD) &&
      (tuner_status[path].sys_type == TUNE_SYSTEM_TYPE_DVBT2))
   {
       TUN_DBG("%u: PLP %u", path, plp);

       memset(&cmd, 0, sizeof(struct dtv_property));

       cmd.cmd = DTV_DVBT2_PLP_ID;
       cmd.u.data = plp;

       props.num = 1;
       props.props = &cmd;

       if (ioctl(tuner_status[path].frontend_fd, FE_SET_PROPERTY, &props) < 0)
       {
          TUN_ERR("%u: Failed to set number of PLPs, errno %d", path, errno);
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
   U32BIT srate;

   FUNCTION_START(STB_TuneGetActualSymbolRate);

   srate = 0;

   if (path < num_paths)
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
 * @return  (E_STB_TUNE_SYSTEM_TYPE) the system type supported by this path.
 *          TUNE_SYSTEM_TYPE_DVBT2 means both DVBT and DVBT2 are supported,
 *          TUNE_SYSTEM_TYPE_DVBS2 means both DVBS and DVBS2 are supported
 */
E_STB_TUNE_SYSTEM_TYPE STB_TuneGetSupportedSystemType(U8BIT path)
{
   E_STB_TUNE_SYSTEM_TYPE type;

   FUNCTION_START(STB_TuneGetSupportedSystemType);

   type = TUNE_SYSTEM_TYPE_UNKNOWN;

   if (path < num_paths)
   {
      switch (tuner_status[path].delivery_system)
      {
         case SYS_DVBT:
            type = TUNE_SYSTEM_TYPE_DVBT;
            break;
         case SYS_DVBT2:
            type = TUNE_SYSTEM_TYPE_DVBT2;
            break;
         case SYS_DVBC_ANNEX_A:
         case SYS_DVBC_ANNEX_B:
            type = TUNE_SYSTEM_TYPE_DVBC;
            break;
         case SYS_DVBS:
            type = TUNE_SYSTEM_TYPE_DVBS;
            break;
         case SYS_DVBS2:
            type = TUNE_SYSTEM_TYPE_DVBS2;
            break;
         default:
            break;
      }
   }

   FUNCTION_FINISH(STB_TuneGetSupportedSystemType);

   return type;
}

void STB_TnueAllStart()
{
    U8BIT i;

    for (i = 0; i != num_paths; i++)
    {
       OpenTuner(&tuner_status[i]);
    }
}

void STB_TuneAllStop()
{
    U8BIT i;
    E_TUNER_STATE state;

    for (i = 0; i != num_paths; i++)
    {
       if (tuner_status[i].frontend_fd != INVALID_FD)
       {
          STB_OSMutexLock(tuner_status[i].mutex);
          state = tuner_status[i].state;
          STB_OSMutexUnlock(tuner_status[i].mutex);
          if (state != TUNER_IDLE)
          {
              STB_TuneStopTuner(i);
          }
       }
       CloseTuner(&tuner_status[i]);
       tuner_status[i].signal_type = TUNE_SIGNAL_NONE;
    }
}

static BOOLEAN STB_TuneSetTone(int frontend_fd, BOOLEAN use_22khz)
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
    if (ioctl(frontend_fd, FE_SET_TONE, tone) >= 0)
        ret = TRUE;

    return ret;
}


/*---local function definitions----------------------------------------------*/

static BOOLEAN SetSysType(S_TUNER_STATUS *tstatus, E_STB_TUNE_SIGNAL_TYPE sig_type)
{
   BOOLEAN retval;
   char fe_name[24];
   int mode;

   retval = FALSE;

   if (tstatus->frontend_fd != INVALID_FD)
   {
	  switch (sig_type)
	  {
		 case TUNE_SIGNAL_QPSK:
			if (tstatus->tuned_sys_type != TUNE_SYSTEM_TYPE_DVBS && tstatus->tuned_sys_type != TUNE_SYSTEM_TYPE_DVBS2)
				tstatus->tuned_sys_type = TUNE_SYSTEM_TYPE_DVBS;
			break;
		 case TUNE_SIGNAL_COFDM:
			if (tstatus->tuned_sys_type != TUNE_SYSTEM_TYPE_DVBT && tstatus->tuned_sys_type != TUNE_SYSTEM_TYPE_DVBT2)
				tstatus->tuned_sys_type = TUNE_SYSTEM_TYPE_DVBT;
			break;
		 case TUNE_SIGNAL_QAM:
			if (tstatus->tuned_sys_type != TUNE_SYSTEM_TYPE_DVBC)
				tstatus->tuned_sys_type = TUNE_SYSTEM_TYPE_DVBC;
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
				  TUN_DBG("Tuner %s configured as DVB-T/T2, min_freq=%lu, max_freq=%lu", fe_name,
				  tstatus->fe_info.frequency_min, tstatus->fe_info.frequency_max);
				  tstatus->signal_type = TUNE_SIGNAL_COFDM;
				  tstatus->delivery_system = SYS_DVBT2;
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
   BOOLEAN retval;
   char fe_name[24];

   retval = TRUE;

   snprintf(fe_name, sizeof(fe_name), "/dev/dvb0.frontend%u", tstatus->path);

   if ((tstatus->frontend_fd = open(fe_name, O_RDWR | O_NONBLOCK)) < 0)
   {
      TUN_ERR("Failed to open %s, errno %d", fe_name, errno);
      retval = FALSE;
   }
   else
   {
      TUN_DBG("open frontend_fd:%d", tstatus->frontend_fd);
   }

   return(retval);
}

static void CloseTuner(S_TUNER_STATUS *tstatus)
{
   if (tstatus->frontend_fd != INVALID_FD)
   {
      TUN_DBG("close frontend_fd:%d", tstatus->frontend_fd);
      close(tstatus->frontend_fd);
      tstatus->frontend_fd = INVALID_FD;
   }
}

static BOOLEAN StartTune(S_TUNER_STATUS *tstatus)
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
         if (STB_TuneSetTone(tstatus->frontend_fd, tstatus->u.sat.use_22khz))
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
                TUN_DBG("%u: Tuning to %lu", tstatus->path, tstatus->freq);
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

static void TunerTask(void *param)
{
   S_TUNER_STATUS *tstatus = param;
   E_TUNER_STATE state;
   BOOLEAN locked;
   BOOLEAN tuner_locked;
   U32BIT start_time;
   BOOLEAN stop;
   U8BIT delay_step;
   struct dvb_frontend_parameters fe_params;
   struct pollfd pfd;
   struct dvb_frontend_event fe_event;

   while (TRUE)
   {
      STB_OSMutexLock(tstatus->mutex);
      state = tstatus->state;
      STB_OSMutexUnlock(tstatus->mutex);

      if (state == TUNER_IDLE)
      {
          /* Wait until tuning has been started */
          TUN_DBG("%u: Waiting for tune request....", tstatus->path);

          STB_OSSemaphoreWait(tstatus->tune_sem);

          STB_OSMutexLock(tstatus->mutex);
          if (tstatus->state == TUNER_LOCKED)
          {
              STB_OSMutexUnlock(tstatus->mutex);
              state = tstatus->state;
              TUN_DBG("##### %u: Already_Tuned fd:%d #####", tstatus->path, tstatus->frontend_fd);
              goto Already_Tuned;
          }
          tstatus->state = TUNER_TUNING;
          stop = tstatus->stop;
          STB_OSMutexUnlock(tstatus->mutex);

          TUN_DBG("%u: Tuning started, checking LOCK status", tstatus->path);

          pfd.fd = tstatus->frontend_fd;
          pfd.events = POLLIN;
          pfd.revents = 0;

          for (locked = FALSE, start_time = STB_OSGetClockMilliseconds();
                       !stop && !locked && (STB_OSGetClockDiff(start_time) < WAIT_LOCK_TIMEOUT); )
          {
              if (poll(&pfd, 1, 50) == 1)
              {
                  if (ioctl(tstatus->frontend_fd, FE_GET_EVENT, &fe_event) >= 0)
                  {
                      TUN_DBG("status=0x%02x", fe_event.status);
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
              TUN_DBG("%u: Tuning stopped", tstatus->path);
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
                  if (ioctl(tstatus->frontend_fd, FE_GET_PROPERTY, &props) != -1)
                  {
                  if ((((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT) && (p.u.data != SYS_DVBT)) ||
                  ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2) && (p.u.data != SYS_DVBT2))) &&
                  (tstatus->signal_type != TUNE_SIGNAL_QAM))
                  {
                     locked = FALSE;
                     TUN_DBG("%u: Ignoring LOCKED status for %s, delivery system is %s", tstatus->path,
                     ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT) ? "DVB-T" :
                     ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2) ? "DVB-T2" : "UNKNOWN")),
                     ((p.u.data == SYS_DVBT) ? "DVB-T" : "DVB-T2"));
                  }
                  }
              }

              if (locked)
              {
                  TUN_DBG("%u: LOCKED", tstatus->path);

                  STB_OSMutexLock(tstatus->mutex);
                  tstatus->state = TUNER_LOCKED;
                  STB_OSMutexUnlock(tstatus->mutex);

                  STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_LOCKED, &tstatus->path,
                  sizeof(U8BIT));
              }
              else
              {
                  TUN_DBG("%u: NOT LOCKED", tstatus->path);

                  //ClearTuner(tstatus);

                  STB_OSMutexLock(tstatus->mutex);
                  tstatus->state = TUNER_RELOCKING;
                  STB_OSMutexUnlock(tstatus->mutex);

                  STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, HW_EV_TYPE_NOTLOCKED, &tstatus->path,
                  sizeof(U8BIT));
              }
          }
      }
      else
      {
Already_Tuned:
          /* Monitor tuner lock status */
          if (state == TUNER_LOCKED)
          {
              locked = TRUE;
              tuner_locked = TRUE;
          }else if (state = TUNER_RELOCKING)
          {
              locked = FALSE;
              tuner_locked = FALSE;
          }

          pfd.fd = tstatus->frontend_fd;
          pfd.events = POLLIN;
          pfd.revents = 0;

          while ((state == TUNER_LOCKED) || (state == TUNER_RELOCKING))
          {
              if (poll(&pfd, 1, 50) == 1)
              {
                  if (ioctl(tstatus->frontend_fd, FE_GET_EVENT, &fe_event) >= 0)
                  {
                      if ((fe_event.status & FE_HAS_LOCK) != 0)
                      {
                          tuner_locked = TRUE;
                          TUN_ERR("FE_GET_EVENT LOCKED:%d state:%d", locked, state);
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

static BOOLEAN IsDiffSysType(S_TUNER_STATUS * tstatus)
{
    BOOLEAN is_diff = FALSE;

    struct dtv_property p = {.cmd = DTV_DELIVERY_SYSTEM, .u.data = 0};
    struct dtv_properties props = {.num = 1, .props = &p};
    if (ioctl(tstatus->frontend_fd, FE_GET_PROPERTY, &props) != -1)
    {
       if ((((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT) && (p.u.data != SYS_DVBT)) ||
       ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2) && (p.u.data != SYS_DVBT2)) ||
       ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS) && (p.u.data != SYS_DVBS)) ||
       ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS2) && (p.u.data != SYS_DVBS2))) &&
       (tstatus->signal_type != TUNE_SIGNAL_QAM))
       {
          TUN_DBG(" different sys_type %s, delivery system is %d",
          ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT) ? "DVB-T" :
          ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBT2) ? "DVB-T2" :
          ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS) ? "DVB-S" :
          ((tstatus->sys_type == TUNE_SYSTEM_TYPE_DVBS2) ? "DVB-S2" : "UNKNOW")))),
          p.u.data);
          is_diff = TRUE;
       }
    }

    return is_diff;
}

static E_TUNER_EVENT GetTunerLockStatus(U32BIT frontend_fd)
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
        TUN_DBG("frontend_fd:%d FE_READ_STATUS errno:%d" ,frontend_fd, errno);
    }

    return tune_event;
}

