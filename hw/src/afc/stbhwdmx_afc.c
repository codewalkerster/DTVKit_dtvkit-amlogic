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
 * @brief   Set Top Box - Hardware Layer, STB Demux Functions
 * @file    stbhwdmx.c
 * @date    October 2018
 */


/*---includes for this file---------------------------------------------------*/
/* compiler library header files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include "dtv_log.h"
#define TAG  "STBHWDMX"

/* third party header files */
#include <dmx.h>
#include <ca.h>
#include <aml_key.h>

/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"

#include "stbhwcfg.h"
#include "internal.h"
#include "stbhwdef.h"
#include "stbhwc.h"
#include "stbhwos.h"
#include "stbhwdmx.h"
#include "stbhwmem.h"
//#include "linuxdvbdmx_wrapper.h"
#include "stbdpc.h"
#include "stb_utils.h"
#include "stbci.h"
#include "libdsm.h"
#include <jni.h>

#include "stbhwdemux_usb.h"
//#include <Aml_MP/Aml_MP.h>
#include "wrapper_dmx.h"

#define DEMUX_DEBUG 1
/*---constant definitions for this file--------------------------------------*/
#ifdef DEMUX_DEBUG
#define DMX_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define DMX_DBG(x,...)
#endif
#define DMX_ERR(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )


#define DMX_TASK_PRIORITY           12
#define DMX_TASK_STACK_SIZE         4096

#define MAX_SECTION_SIZE            4096
#define MAX_DDB_SECTION_SIZE        (4096*8)
#define OAD_DSI_DDB_MATCH    0x38
#define OAD_DDB_PID         0x280
#define DEMUX_SECTION_FILTER_LENGTH 8

#define MAX_PID_FILTERS             24
//#define MAX_SECTION_FILTERS         16
//#define MAX_FILTERS_PER_PID         8
#define MAX_TEMI_FILTERS            2

#define DEMUX_FILTER_NOT_ALLOCATED  0xFFFF
#define DEMUX_PID_NOT_USED          0xFFFF
#define INVALID_PID                 0x1FFF

#define TEXT_BUFFER_SIZE            (65 * 1024)


/* Local ENUM/TYPE Definitions */

typedef enum
{ /* E_STB_DMX_DESC_TRACK is subset of this enum (first three correspond)*/
   DMX_AUDIO,
   DMX_VIDEO,
   DMX_TEXT,
   DMX_PCR,
   DMX_ADES,
   DMX_PRESELECTION,
   DMX_PID_COUNT
} E_DMX_TRACK;




typedef void(*SectionFilterFunc)(U8BIT path, U16BIT bytes, U16BIT pfilt_id);

/*typedef struct s_section_filter_info
{
   BOOLEAN in_use;
   U8BIT match[DEMUX_SECTION_FILTER_LENGTH];
   U8BIT mask[DEMUX_SECTION_FILTER_LENGTH];
   BOOLEAN check_crc;
   BOOLEAN setup;
   BOOLEAN empty_mask;
} S_SECTION_FILTER_INFO;
*/
#ifdef TEMI_TIMELINES
typedef struct s_temi_filter
{
   U16BIT pid;
   TEMI_FILTER_CALLBACK func_ptr;
   void *context;
   void *sema;
   U64BIT pts;
} S_TEMI_FILTER;
#endif

/*
typedef struct s_pid_filter_info
{
   U8BIT index;
   U16BIT pid;
   int fhandle;
   BOOLEAN started;
   const U8BIT* data_packet;
   U16BIT data_packet_size;
   S_SECTION_FILTER_INFO section_filters[MAX_SECTION_FILTERS];
   FILTER_CALLBACK func_ptr[MAX_FILTERS_PER_PID];
   U8BIT start_count[MAX_FILTERS_PER_PID];
} S_PID_FILTER_INFO;
*/
typedef struct s_des_track_info
{
   int chanid;
   E_STB_DMX_DESC_TYPE type;
   E_STB_DMX_KEY_USAGE usage;
   BOOLEAN iseven;
   BOOLEAN isodd;
   U8BIT even[32];
   U8BIT odd[32];
   int even_key_id;
   int odd_key_id;
} S_DES_TRACK_INFO;

typedef struct
{
   void*  filter_mtx;
   U8BIT path;
   U16BIT caps;

   void *config_mutex;

   E_STB_DMX_DEMUX_SOURCE source;
   U8BIT source_param;
U16BIT demux_cap;
#ifdef TEMI_TIMELINES
   S_TEMI_FILTER temi_filters[MAX_TEMI_FILTERS];
#endif

   U16BIT pids[DMX_PID_COUNT];

   S_DES_TRACK_INFO tracks[DESC_NUM_TRACKS];

   /* Subtitle/teletext PES support vars */
   int text_fhandle;
   U8BIT* text_buffer;
   U8BIT* write_ptr;
   U8BIT* read_ptr;
   U32BIT text_bytes_available;
   void* text_mutex;
   BOOLEAN text_started;

   S_PID_FILTER_INFO filter_info[MAX_PID_FILTERS];

   U8BIT num_pid_filters_started;
} S_DMX_STATUS;

#define MAX_SC2_DSC_DEV             32
#define SC2_DSC_CH_NUM 32
typedef struct s_sc2_dsc_dev_info
{
   int key_fd;
   int dsm_handle;
   uint32_t dsm_token;
   jobject descramble_handle;
   int dsc_fd[MAX_SC2_DSC_DEV];
   int dsc_ref[MAX_SC2_DSC_DEV];
   void *mutex;
   struct s_sc2_dsc_channel
   {
      E_STB_TS_SOURCE src;
      int pid;
      int chan_id;
      int ref;
      E_STB_DSC_CA_TYPE dsc_type;
      U8BIT random_key[4];
      int key_id;
      int iv_key_id;
      int dev_id;
   } dsc_pid_channel[SC2_DSC_CH_NUM];
} S_SC2_DSC_DEV_INFO;


static S_SC2_DSC_DEV_INFO     *sc2_dsc_dev_info = NULL;

/*---local (static) variable declarations for this file----------------------*/
static S_DMX_STATUS* demux_status;
static U8BIT num_paths;

static U8BIT* pes_data = NULL;
static U32BIT pes_data_size = 0;

/*---local function prototypes for this file---------------------------------*/
static BOOLEAN UpdateSectionFilter(U8BIT path, U16BIT filter_index);

static void PidCallback(ST_CALLBACK_T* param);
static void PesCallback(int dev_no, int fhandle, const uint8_t *data, int len, void *user_data);

static void ApplyKey(U8BIT path, E_STB_DMX_DESC_TRACK track);
static void ClearKey(U8BIT path, E_STB_DMX_DESC_TRACK track);
static int key_open(void);

/**
 * @brief   Initialises the demux / programmable transport interface
 * @param   paths Number of demux paths to be initialised
 * @param   inc_pes_collection Not used
 */
void STB_DMXInitialise(U8BIT paths, BOOLEAN inc_pes_collection)
{
   BOOLEAN am_result = FALSE;
   U16BIT i;
   U16BIT j;

   char buf[128];
   char cmd[32];

   FUNCTION_START(STB_DMXInitialise);

   DMX_DBG("%u demuxes--, %s PES colection", paths, inc_pes_collection ? "with" : "no");
   num_paths = paths;

   if (num_paths != 0)
   {
      /* Allocate memory for the status structures (one per path) */
      demux_status = (S_DMX_STATUS*)STB_MEMGetSysRAM(sizeof(S_DMX_STATUS) * num_paths);

      /* Initialise the status for each path and section filter
       * and open any required demuxing handles */
      if (demux_status != NULL)
      {
         memset(demux_status, 0, sizeof(S_DMX_STATUS) * num_paths);
         for (i = 0; i < num_paths; i++)
         {
            am_result = TRUE;//DMX_Open(i);
            if (am_result)
            {
               demux_status[i].path = i;
               demux_status[i].config_mutex = STB_OSCreateMutex();
               if (demux_status[i].config_mutex != NULL)
               {
                  /* All demuxes are not capable of everything ,get cap from cfg*/
                  demux_status[i].caps = aml_hw_cfg.dmx_cap[i];
                  DMX_ERR("dmx%dcap:0x%x", i, demux_status[i].caps);

                  for (j = 0; j < DMX_PID_COUNT; j++)
                  {
                     demux_status[i].pids[j] = 0;
                  }

                  for (j = 0; j != DESC_NUM_TRACKS; j++)
                  {
                     demux_status[i].tracks[j].chanid = -1;
                     demux_status[i].tracks[j].iseven = FALSE;
                     demux_status[i].tracks[j].isodd = FALSE;
                     demux_status[i].tracks[j].even_key_id = -1;
                     demux_status[i].tracks[j].odd_key_id = -1;
                  }

                  /* Set default values */
                  for (j = 0; j < MAX_PID_FILTERS; j++)
                  {
                     memset(&demux_status[i].filter_info[j], 0, sizeof(demux_status[i].filter_info[j]));
                     demux_status[i].filter_info[j].path = i;
                     demux_status[i].filter_info[j].index = j;
                     demux_status[i].filter_info[j].fhandle = -1;
                     demux_status[i].filter_info[j].started = FALSE;
                     demux_status[i].filter_info[j].pid = DEMUX_PID_NOT_USED;
                     demux_status[i].filter_info[j].data_packet = NULL;
                     demux_status[i].filter_info[j].data_packet_size = 0;
                  }

                  /* Default sources for each path */
                  memset(buf, 0, sizeof(buf));
                  memset(cmd, 0, sizeof(cmd));
                  snprintf(buf, sizeof(buf), "/sys/class/stb/demux%d_source", i);
                  snprintf(cmd, sizeof(cmd), "ts%d", aml_hw_cfg.tuners[0].ts_input_idx);
                  am_result = STB_File_Echo(buf, cmd);
                  if (am_result)
                  {
                      demux_status[i].source = DMX_TUNER;
                      demux_status[i].source_param = 0;
                  } else {
                      demux_status[i].source = DMX_MEMORY;
                      demux_status[i].source_param = 255;
                  }
                  DMX_ERR("dmx%d ts_input:ts%d ret:%d", i, aml_hw_cfg.tuners[0].ts_input_idx, am_result);

                  if (inc_pes_collection)
                  {
                     demux_status[i].text_buffer = STB_MEMGetSysRAM(TEXT_BUFFER_SIZE);
                     demux_status[i].write_ptr = demux_status[i].text_buffer;
                     demux_status[i].read_ptr = demux_status[i].text_buffer;
                     demux_status[i].text_mutex = STB_OSCreateMutex();
                     demux_status[i].text_bytes_available = 0;
                     demux_status[i].text_started = FALSE;
                  }
                  demux_status[i].text_fhandle = -1;
                  demux_status[i].num_pid_filters_started = 0;
               }
               else
               {
                  DMX_ERR("Failed to create mutex for demux %u!", i);
               }
            }
            else
            {
               DMX_ERR("Failed to open demux device %u, error %d", i, am_result);
            }
         }

         /*Config Demux DMC memory size */
         for (i = 0; i < aml_hw_cfg.mem_level_num; i++) {
            memset(buf, 0, sizeof(buf));
            memset(cmd, 0, sizeof(cmd));
            snprintf(buf, sizeof(buf), "/sys/class/stb/dmc_mem");
            snprintf(cmd, sizeof(cmd), "%d %d", aml_hw_cfg.dmc_mem[i].level, aml_hw_cfg.dmc_mem[i].size);
            am_result = STB_File_Echo(buf, cmd);
            if (am_result)
            {
                DMX_ERR("Failed to config dmc memory! level %d", aml_hw_cfg.dmc_mem[i].level);
            }
         }
      }
   }
   else
   {
      DMX_DBG("No demuxes found!");
   }

#if 1
      {
         sc2_dsc_dev_info = (S_SC2_DSC_DEV_INFO *)STB_MEMGetSysRAM(sizeof(S_SC2_DSC_DEV_INFO));
         sc2_dsc_dev_info->key_fd = key_open();;
         DMX_DBG("KEY TABLE FD[%d]",sc2_dsc_dev_info->key_fd);

         sc2_dsc_dev_info->descramble_handle = NULL;
         sc2_dsc_dev_info->mutex = STB_OSCreateMutex();
         for (i = 0; i < MAX_SC2_DSC_DEV; i++)
         {
            sc2_dsc_dev_info->dsc_fd[i] = -1;
            sc2_dsc_dev_info->dsc_ref[i] = 0;
         }
         for (i = 0; i < SC2_DSC_CH_NUM; i++)
         {
            sc2_dsc_dev_info->dsc_pid_channel[i].src = STB_TS_SOURCE_MAX;
            sc2_dsc_dev_info->dsc_pid_channel[i].pid = -1;
            sc2_dsc_dev_info->dsc_pid_channel[i].chan_id = -1;
            sc2_dsc_dev_info->dsc_pid_channel[i].dsc_type = -1;
            sc2_dsc_dev_info->dsc_pid_channel[i].key_id = -1;
            sc2_dsc_dev_info->dsc_pid_channel[i].iv_key_id = -1;
         }
      }

      uint32_t token_  = -1;
      sc2_dsc_dev_info->dsm_handle = DSM_OpenSession(0);
      DSM_GenerateToken(sc2_dsc_dev_info->dsm_handle,&token_);
      sc2_dsc_dev_info->dsm_token = token_;
      DMX_DBG("DSM handle[%d] [%d] !", sc2_dsc_dev_info->dsm_handle , sc2_dsc_dev_info->dsm_token);
#endif
   FUNCTION_FINISH(STB_DMXInitialise);
}

/**
 * @brief   Returns the capability flags of the given demux
 * @param   path - demux
 * @return  Capability flags
 */
U16BIT STB_DMXGetCapabilities(U8BIT path)
{
   U16BIT caps;

   FUNCTION_START(STB_DMXGetCapabilities);

   if ((path < num_paths) && (demux_status[path].config_mutex != NULL))
   {
      caps = demux_status[path].caps;
   }
   else
   {
      caps = 0;
   }

   FUNCTION_FINISH(STB_DMXGetCapabilities);

   return(caps);
}

/**
 * @brief   Changes the packet IDs for the PCR Video, Audio, Text and Data
 * @param   path The demux path to be configured
 * @param   pcr_pid The PID to use for the Program Clock Reference
 * @param   video_pid The PID to use for the Video PES
 * @param   audio_pid The PID to use for the Audio PES
 * @param   text_pid The PID to use for the Teletext data
 * @param   data_pid The PID to use for the data
 */
void STB_DMXChangeDecodePIDs(U8BIT path, U16BIT pcr_pid, U16BIT video_pid, U16BIT audio_pid,
   U16BIT text_pid, U16BIT data_pid, U16BIT ad_pid, U8BIT preselection_id)
{
   U16BIT *pids;
   FUNCTION_START(STB_DMXChangeDecodePIDs);
   USE_UNWANTED_PARAM(data_pid);

   DMX_DBG("%u: pcr=%u, video=%u, audio=%u, text=%u, ad=%u, preselection_id=%u", path, pcr_pid, video_pid, audio_pid,
      text_pid, ad_pid, preselection_id);

   if ((path < num_paths) && (demux_status[path].config_mutex != NULL))
   {
      pids = demux_status[path].pids;

      pids[DMX_PCR] = pcr_pid;
      pids[DMX_ADES] = ad_pid;
      pids[DMX_PRESELECTION] = preselection_id;

      if (pids[DMX_AUDIO] != audio_pid)
      {
         pids[DMX_AUDIO] = audio_pid;
         if (demux_status[path].source != DMX_MEMORY)
         {
            if (audio_pid != 0)
            {
                #if 1 //EMING
               DMX_DBG("ApplyKey AUDIO");
               //ResetDscChannel(path, DESC_TRACK_AUDIO);
               ApplyKey(path, DESC_TRACK_AUDIO);
               #endif
            }
            else
            {
               #if 1 //EMING
                DMX_DBG("ClearKey AUDIO");
               ClearKey(path, DESC_TRACK_AUDIO);
               #endif
            }
         }
      }
      if (pids[DMX_VIDEO] != video_pid)
      {
         pids[DMX_VIDEO] = video_pid;
         if (demux_status[path].source != DMX_MEMORY)
         {
            if (video_pid != 0)
            {
                DMX_DBG("ApplyKey VIDEO");
               #if 1 //EMING
               //ResetDscChannel(path, DESC_TRACK_VIDEO);
               ApplyKey(path, DESC_TRACK_VIDEO);
               #endif
            }
            else
            {
            DMX_DBG("ClearKey VIDEO");
            #if 1 //EMING
               ClearKey(path, DESC_TRACK_VIDEO);
            #endif
            }
         }
      }
      //dsc_set_aes_output(TRUE);
      STB_DMXChangeTextPID(path, text_pid);
   }

   FUNCTION_FINISH(STB_DMXChangeDecodePIDs);
}

/**
 * @brief   Changes just the teletext PID
 * @param   path The demux path to configure
 * @param   text_pid The PID to use for the teletext data
 */
void STB_DMXChangeTextPID(U8BIT path, U16BIT text_pid)
{
   BOOLEAN am_result;

   struct dmx_pes_filter_params pes_params;

   FUNCTION_START(STB_DMXChangeTextPID);

   if ((path < num_paths) && (demux_status[path].pids[DMX_TEXT] != text_pid))
   {
      if (demux_status[path].text_fhandle >= 0)
      {
         if (demux_status[path].text_started)
         {
            /* Stop the filter and clear the callback */
#if 0
            //DMX_StopFilter(path, demux_status[path].text_fhandle);
            //DMX_SetCallback(path, demux_status[path].text_fhandle, NULL, NULL);
            //DMX_FreeFilter(path, demux_status[path].text_fhandle);
#endif
            demux_status[path].text_fhandle = -1;
            demux_status[path].text_started = FALSE;

            /* PID has been changed or filter has been stopped so clear record of any
             * remaining text PES data */
            STB_OSMutexLock(demux_status[path].text_mutex);
            demux_status[path].write_ptr = demux_status[path].text_buffer;
            demux_status[path].read_ptr = demux_status[path].text_buffer;
            demux_status[path].text_bytes_available = 0;
            STB_OSMutexUnlock(demux_status[path].text_mutex);
         }
      }

      if (demux_status[path].pids[DMX_TEXT] != text_pid)
      {
         demux_status[path].pids[DMX_TEXT] = text_pid;
         if (text_pid != 0)
         {
            #if 0 //EMING
            ResetDscChannel(path, DESC_TRACK_TEXT);
            ApplyKey(path, DESC_TRACK_TEXT);
            #endif
         }
         else
         {
            #if 0 //EMING
            ClearKey(path, DESC_TRACK_TEXT);
            #endif
         }
      }

      if ((text_pid == 0) || (text_pid == 0xffff))
      {
         /* Set invalid PID value */
         text_pid = INVALID_PID;
      }
#if 0
      if (text_pid != INVALID_PID)
      {
         /* Open a demux instance for the text (subtitle) PES */
         am_result = DMX_AllocateFilter(path, &demux_status[path].text_fhandle);
         if (am_result)
         {
            DMX_DBG("%u: Opened text PES filter, handle=%d", path, demux_status[path].text_fhandle);
            DMX_SetBufferSize(path, demux_status[path].text_fhandle, TEXT_BUFFER_SIZE);
            memset(&pes_params, 0, sizeof(pes_params));

            if (demux_status[path].source == DMX_MEMORY)
            {
               pes_params.input = DMX_IN_DVR;
            }
            else
            {
               pes_params.input = DMX_IN_FRONTEND;
            }

            pes_params.output = DMX_OUT_TAP;
            pes_params.pes_type = DMX_PES_SUBTITLE;
            pes_params.pid = text_pid;
            am_result = DMX_SetPesFilter(path, demux_status[path].text_fhandle, &pes_params);
            if (am_result == FALSE)
            {
               DMX_ERR("%u: Failed to set PID %u, handle %u, error %d",
                  path, text_pid, demux_status[path].text_fhandle, am_result);
            }
            else
            {
               if (demux_status[path].pids[DMX_TEXT] != 0)
               {
                  am_result = DMX_SetCallback(path, demux_status[path].text_fhandle, PesCallback,
                     (void *)&demux_status[path]);
                  if (am_result)
                  {
                     /* Can now restart PES collection and the PES task */
                     am_result = DMX_StartFilter(path, demux_status[path].text_fhandle);
                     if (am_result)
                     {
                        demux_status[path].text_started = TRUE;
                     }
                     else
                     {
                        /* Filter not started so clear the callback */
                        DMX_SetCallback(path, demux_status[path].text_fhandle, NULL, NULL);

                        DMX_ERR("Failed to start demux %u text filter, error %d", path, am_result);
                     }
                  }
                  else
                  {
                     DMX_ERR("Failed to set demux %u callback, error %d", path, am_result);
                  }
               }
            }
         }
         else
         {
            DMX_ERR("Failed to open PES filter on demux %u, error %d", path, am_result);
         }

      }
 #endif
   }

   FUNCTION_FINISH(STB_DMXChangeTextPID);
}

#ifdef TEMI_TIMELINES
/**
 * @brief   Start specified TEMI filter collecting data.
 * @param   path Required decode path number.
 * @param   pid Required PID to demux.
 * @param   func Function to report any TEMI related AF descriptor
 * @param   context Parameter to be used as context in callbacks from this filter
 * @return  New TEMI filter identifier or invalid id.
 */
U16BIT STB_DMXStartTemiFilter(U8BIT path, U16BIT pid, TEMI_FILTER_CALLBACK func, void *context)
{
   S_DMX_STATUS *ds;
   U16BIT filt_id;
   U16BIT filter_index;
   S_TEMI_FILTER *filter_ptr;

   FUNCTION_START(STB_DMXStartTemiFilter);

   filt_id = STB_DMX_PID_FILTER_INVALID;
   if (path < num_paths)
   {
      ds = demux_status + path;
      STB_OSMutexLock(ds->filter_mtx);

      /* Find an unused TEMI filter */
      for (filter_index = 0, filter_ptr = &ds->temi_filters[filter_index];
           (filter_ptr->pid != DEMUX_PID_NOT_USED) && (filter_index < MAX_TEMI_FILTERS); filter_ptr++, filter_index++)
         ;

      if (filter_index < MAX_TEMI_FILTERS)
      {
         /* This PID isn't being collected and a free PID filter has been found */
         filter_ptr->sema = STB_OSCreateSemaphore();
         filter_ptr->pid = pid;
         filter_ptr->func_ptr = func;
         filter_ptr->context = context;
         filter_ptr->pts = 0;
         filt_id = filter_index;
      }
      else
      {
         DMX_ERR("(%u): No TEMI filters available to collect PID %u", path, pid);
      }

      STB_OSMutexUnlock(ds->filter_mtx);
   }

   FUNCTION_FINISH(STB_DMXStartTemiFilter);

   return filt_id;
}

/**
 * @brief   Stop specified TEMI filter.
 * @param   path Required Decode Path Number.
 * @param   filter_id Required TEMI filter identifier.
 */
void STB_DMXStopTemiFilter(U8BIT path, U16BIT filt_id)
{
   S_DMX_STATUS *ds;
   S_TEMI_FILTER *filter_ptr;

   FUNCTION_START(STB_DMXStopTemiFilter);

   if ((path < num_paths) && (filt_id < MAX_TEMI_FILTERS))
   {
      ds = demux_status + path;
      filter_ptr = ds->temi_filters + filt_id;

      STB_OSMutexLock(ds->filter_mtx);

      if (filter_ptr->pid != DEMUX_PID_NOT_USED)
      {
         STB_OSDeleteSemaphore(filter_ptr->sema);
         filter_ptr->pid = DEMUX_PID_NOT_USED;
      }

      STB_OSMutexUnlock(ds->filter_mtx);
   }

   FUNCTION_FINISH(STB_DMXStopTemiFilter);
}
#endif

/**
 * @brief   Get a New PID Filter & Setup Associated Buffer and Callback
 *          Function Address.
 * @param   path Required Decode Path Number.
 * @param   pid Required PID to Demux.
 * @param   func_ptr User's Interrupt Procedure Function Address.
 * @return  New PID filter identifier or invalid id.
 */
U16BIT  STB_DMXGrabPIDFilter(U8BIT path, U16BIT pid, FILTER_CALLBACK func_ptr)
{
   U16BIT i;
   U16BIT filter_index;
   U16BIT pfilt_id;
   S_PID_FILTER_INFO* filter_ptr;

   FUNCTION_START(STB_DMXGrabPIDFilter);
   DMX_DBG("STB_DMXGrabPIDFilter Start");

   pfilt_id = DEMUX_FILTER_NOT_ALLOCATED;

   if (path < num_paths)
   {
      STB_OSMutexLock(demux_status[path].config_mutex);

      /* Find an unused PID filter */
      for (filter_index = 0, filter_ptr = &demux_status[path].filter_info[filter_index];
         (filter_ptr->pid != DEMUX_PID_NOT_USED) && (filter_index < MAX_PID_FILTERS); filter_ptr++, filter_index++);

      if (filter_index < MAX_PID_FILTERS)
      {
         /* This PID isn't being collected and a free PID filter has been found */
         filter_ptr = &demux_status[path].filter_info[filter_index];

         /* Store associated information for the PID filter */
         filter_ptr->pid = pid;
         filter_ptr->func_ptr[0] = func_ptr;
         filter_ptr->start_count[0] = 0;
         filter_ptr->data_packet_size = 0;
         filter_ptr->started = FALSE;

         for (i = 0; i < MAX_SECTION_FILTERS; i++)
         {
            memset(&filter_ptr->section_filters[i], 0, sizeof(S_SECTION_FILTER_INFO));
         }

         pfilt_id = (filter_index << 8);
#ifdef FILTER_PRINTS
DMX_DBG(">> %s(%u): pid=%u, fd=%d, func=%p, 0x%04x - NEW\n", __FUNCTION__, path, pid, filter_ptr->filter_fd, func_ptr, pfilt_id);
#endif
      }
      else
      {
         DMX_ERR("%u: No more filters available for pid %u", path, pid);
      }

      STB_OSMutexUnlock(demux_status[path].config_mutex);
   }

   FUNCTION_FINISH(STB_DMXGrabPIDFilter);
   DMX_DBG("STB_DMXGrabPIDFilter end");

   return(pfilt_id);
}

/**
 * @brief   Releases a previously allocated PID filter
 * @param   path the demux path of the filter
 * @param   pfilt_id the handle of the filter
 */
void STB_DMXReleasePIDFilter(U8BIT path, U16BIT pfilt_id)
{
   U16BIT filter_index, handler_index;
   S_PID_FILTER_INFO* filter_ptr;
   BOOLEAN no_more_funcs;
   U8BIT i;
   DMX_DBG("STB_DMXReleasePIDFilter Start");

   FUNCTION_START(STB_DMXReleasePIDFilter);

   if (path < num_paths)
   {
      filter_index = (pfilt_id >> 8) & 0xff;
      handler_index = (pfilt_id >> 4) & 0x0f;

      filter_ptr = &demux_status[path].filter_info[filter_index];

      /* Wait until nothing else is using the configuration */
      STB_OSMutexLock(demux_status[path].config_mutex);

      /* Clear the callback function for this filter */
      filter_ptr->func_ptr[handler_index] = NULL;

      /* The filter should only be release if there are no more registered callback functions */
      no_more_funcs = TRUE;

      for (i = 0; no_more_funcs && (i < MAX_FILTERS_PER_PID); i++)
      {
         if (filter_ptr->func_ptr[i] != NULL)
         {
            no_more_funcs = FALSE;
         }
      }

      if (no_more_funcs)
      {
         /* Check whether all section filters have been released */
         for (i = 0; i < MAX_SECTION_FILTERS; i++)
         {
            if (filter_ptr->section_filters[i].setup)
            {
               DMX_ERR("%u: section filter %u on PID %u hasn't been released!", path, i,
                  filter_ptr->pid);

               filter_ptr->section_filters[i].setup = FALSE;
            }
         }

         if (filter_ptr->started)
         {
            /* Stop the filter */
            STB_DMXStopPIDFilter(path, pfilt_id);
         }

#ifdef FILTER_PRINTS
//printf(">> %s(%u, 0x%04x): pid=%u, fd=%d - FREED\n", __FUNCTION__, path, pfilt_id, filter_ptr->pid,
//filter_ptr->filter_fd);
#endif
         /* Mark the filter as no longer allocated */
         filter_ptr->pid = DEMUX_PID_NOT_USED;
         filter_ptr->started = FALSE;
      }
#ifdef FILTER_PRINTS
      else
      {
  // printf(">> %s(%u, 0x%04x): pid=%u, fd=%d\n", __FUNCTION__, path, pfilt_id, filter_ptr->pid, filter_ptr->filter_fd);
      }
#endif

      STB_OSMutexUnlock(demux_status[path].config_mutex);
   }
   DMX_DBG("STB_DMXReleasePIDFilter end");

   FUNCTION_FINISH(STB_DMXReleasePIDFilter);
}

/**
 * @brief   Allocated a new section filter on the specified PID filter
 * @param   path the demux path to use
 * @param   pfilt_id the PID filter to assign the section filter to
 * @return  The section filter handle
 */
U16BIT STB_DMXGrabSectFilter(U8BIT path, U16BIT pfilt_id)
{
   U16BIT filter_index;
   U16BIT sfilt_id;
   S_PID_FILTER_INFO* filter_ptr;
   U16BIT i;

   FUNCTION_START(STB_DMXGrabSectFilter);
   DMX_DBG("STB_DMXGrabSectFilter Start");

   sfilt_id = DEMUX_FILTER_NOT_ALLOCATED;

   if (path < num_paths)
   {
      filter_index = (pfilt_id >> 8) & 0xff;

      filter_ptr = &demux_status[path].filter_info[filter_index];

      STB_OSMutexLock(demux_status[path].config_mutex);

      /* Find a free section filter for the given PID filter */
      for (i = 0; (i < MAX_SECTION_FILTERS) && (sfilt_id == DEMUX_FILTER_NOT_ALLOCATED); i++)
      {
         if (!filter_ptr->section_filters[i].in_use)
         {
            sfilt_id = i;
         }
      }

      if (sfilt_id != DEMUX_FILTER_NOT_ALLOCATED)
      {
         /* Have found a free section filter */
         filter_ptr->section_filters[sfilt_id].in_use = TRUE;

         /* Clear the filter's match/mask data */
         memset(filter_ptr->section_filters[sfilt_id].match, 0, DEMUX_SECTION_FILTER_LENGTH);
         memset(filter_ptr->section_filters[sfilt_id].mask, 0, DEMUX_SECTION_FILTER_LENGTH);

         /* The section filter ID is a combination involving the PID filter ID
          * so that we can get back to it.  This relies on there not being more
          * than 16 section filters for each PID filter */
         sfilt_id += pfilt_id;
//#ifdef FILTER_PRINTS
DMX_DBG(">> %s(%u, 0x%04x): pid=%u, sfilt=0x%04x\n", __FUNCTION__, path, pfilt_id, filter_ptr->pid, sfilt_id);
//#endif
      }

      STB_OSMutexUnlock(demux_status[path].config_mutex);
   }

   FUNCTION_FINISH(STB_DMXGrabSectFilter);
   DMX_DBG("STB_DMXGrabSectFilter end");

   return(sfilt_id);
}

/**
 * @brief   Releases a previously allocated section filter
 * @param   path the demux path of the filter
 * @param   sfilt_id the handle of the section filter
 */
void STB_DMXReleaseSectFilter(U8BIT path, U16BIT sfilt_id)
{
   U8BIT pid_filter_index;
   U8BIT sect_filter_index;
   S_SECTION_FILTER_INFO* sect_filter;

   FUNCTION_START(STB_DMXReleaseSectFilter);
   DMX_DBG("STB_DMXReleaseSectFilter Start");

   if (path < num_paths)
   {
      /* Section filter ID includes the PID filter ID */
      pid_filter_index = sfilt_id >> 8;
      sect_filter_index = sfilt_id & 0x0f;

      sect_filter = &demux_status[path].filter_info[pid_filter_index].section_filters[sect_filter_index];

      STB_OSMutexLock(demux_status[path].config_mutex);

//#ifdef FILTER_PRINTS
DMX_DBG(">> %s(%u, 0x%04x): in_use=%u\n", __FUNCTION__, path, sfilt_id, sect_filter->in_use);
//#endif
      if (sect_filter->in_use)
      {
         sect_filter->in_use = FALSE;
         sect_filter->setup = FALSE;

         UpdateSectionFilter(path, pid_filter_index);
      }

      STB_OSMutexUnlock(demux_status[path].config_mutex);
   }
   DMX_DBG("STB_DMXReleaseSectFilter end");

   FUNCTION_FINISH(STB_DMXReleaseSectFilter);
}

/**
 * @brief   Configures a match and mask for a specified section filter
 * @param   path the demux path of the section filter
 * @param   sfilt_id the handle of the section filter
 * @param   match_ptr pointer to the match bytes
 * @param   mask_ptr pointer to the mask bytes
 * @param   not_equal_byte_index the byte position for a not equal compare
 * @param   crc TRUE to use CRC checking FALSE to ignore
 */
void STB_DMXSetupSectFilter(U8BIT path, U16BIT sfilt_id, U8BIT *match_ptr, U8BIT *mask_ptr,
   U8BIT not_equal_byte_index, BOOLEAN crc)
{
   U16BIT pid_filter_index;
   U16BIT sect_filter_index;
   S_PID_FILTER_INFO* pid_filter;
   S_SECTION_FILTER_INFO *sect_filter;
   U8BIT do_masking;
   U8BIT i;
   BOOLEAN am_result;
   DMX_DBG("STB_DMXSetupSectFilter Start");

   FUNCTION_START(STB_DMXSetupSectFilter);
   USE_UNWANTED_PARAM(not_equal_byte_index);

   if (path < num_paths)
   {
      /* Section filter id includes the PID filter id */
      pid_filter_index = sfilt_id >> 8;
      sect_filter_index = sfilt_id & 0x0f;

      pid_filter = &demux_status[path].filter_info[pid_filter_index];
      sect_filter = &pid_filter->section_filters[sect_filter_index];

      STB_OSMutexLock(demux_status[path].config_mutex);

#ifdef FILTER_PRINTS
printf(">> %s(%u, 0x%04x, crc=%u)\n", __FUNCTION__, path, sfilt_id, crc);
#endif

      /* Save the match and mask data so that the correct section filter
       * can be found when data is received */
      memcpy(sect_filter->match, match_ptr, DEMUX_SECTION_FILTER_LENGTH);
      memcpy(sect_filter->mask, mask_ptr, DEMUX_SECTION_FILTER_LENGTH);

      /* If mask is empty then can skip checking this when section data arrives */
      for (i = 0, do_masking = 0; (do_masking == 0) && (i < DEMUX_SECTION_FILTER_LENGTH); i++)
      {
         do_masking |= mask_ptr[i];
      }

      if (do_masking == 0)
      {
         sect_filter->empty_mask = TRUE;
      }

      sect_filter->check_crc = crc;
      sect_filter->setup = TRUE;

      UpdateSectionFilter(path, pid_filter_index);


      STB_OSMutexUnlock(demux_status[path].config_mutex);
   }
   DMX_DBG("STB_DMXSetupSectFilter end");

   FUNCTION_FINISH(STB_DMXSetupSectFilter);
}

/**
 * @brief   Start Specified PID Filter Collecting Data.
 * @param   path Required Decode Path Number.
 * @param   pfilter_id Required PID Filter Identifier.
 */
void  STB_DMXStartPIDFilter(U8BIT path, U16BIT pfilt_id)
{
   U16BIT filter_index;
   U16BIT handler_index;
   S_PID_FILTER_INFO *pid_filter;
   BOOLEAN am_result = TRUE;

   FUNCTION_START(STB_DMXStartPIDFilter);
   DMX_DBG("STB_DMXStartPIDFilter Start");

   if (path < num_paths)
   {
      filter_index = (pfilt_id >> 8) & 0xff;
      handler_index = (pfilt_id >> 4) & 0x0f;

      pid_filter = &demux_status[path].filter_info[filter_index];

      STB_OSMutexLock(demux_status[path].config_mutex);

      /* Increment the number of times the filter has been started */
      pid_filter->start_count[handler_index]++;

#ifdef FILTER_PRINTS
printf(">> %s(%u, 0x%04x): start_count=%u, started=%u\n", __FUNCTION__, path, pfilt_id,
   pid_filter->start_count[handler_index], pid_filter->started);
#endif
//HOOK
#if 0
      if (!pid_filter->started)
      {
         if (pid_filter->fhandle == -1)
         {
            am_result = DMX_AllocateFilter(path, &pid_filter->fhandle);
            if (am_result)
            {
               if (Is_DDB_Filter(pid_filter))
               {
                   am_result = DMX_SetBufferSize(path, pid_filter->fhandle,
                   8 * MAX_DDB_SECTION_SIZE);
               }
               else
               {
                   am_result = DMX_SetBufferSize(path, pid_filter->fhandle,
                  8 * MAX_SECTION_SIZE);
               }
               if (!am_result)
               {
                  DMX_ERR("%u: Failed to set buffer size for section filter %u, error %d", path,
                     filter_index, am_result);
               }
            }
            UpdateSectionFilter(path, filter_index);
         }

         if (pid_filter->fhandle != -1)
         {
            am_result = DMX_SetCallback(path, pid_filter->fhandle, PidCallback, (void *)pid_filter);
            if (am_result)
            {
               am_result = DMX_StartFilter(path, pid_filter->fhandle);
               if (am_result)
               {
                  pid_filter->started = TRUE;
                  demux_status[path].num_pid_filters_started++;
               }
               else
               {
                  /* Failed to start filter so clear the callback */
                  DMX_SetCallback(path, pid_filter->fhandle, NULL, NULL);
                  DMX_ERR("%u: Failed to start PID filter 0x%04x, error %d", path, pfilt_id, am_result);
               }
            }
            else
            {
               DMX_ERR("%u: Failed to set callback for PID filter 0x%04x, error %d", path,
                  pfilt_id, am_result);
            }
         }
      }
#endif


      if (!pid_filter->started)
      {
             if (pid_filter->fhandle != -1)
             {
                   am_result = DMX_StartFilter(pid_filter->fhandle);
                   if (am_result)
                   {
                      pid_filter->started = TRUE;
                      demux_status[path].num_pid_filters_started++;
                   }
            }
            else
            {
                   DMX_ERR("%u: Failed to set callback for PID filter 0x%04x, error %d", path,
                      pfilt_id, am_result);
            }
      }
      STB_OSMutexUnlock(demux_status[path].config_mutex);
   }
   DMX_DBG("STB_DMXStartPIDFilter end");

   FUNCTION_FINISH(STB_DMXStartPIDFilter);
}

/**
 * @brief   Stop Specified PID Filter Collecting Data.
 * @param   path Required Decode Path Number.
 * @param   pfilter_id Required PID Filter Identifier.
 */
void  STB_DMXStopPIDFilter(U8BIT path, U16BIT pfilt_id)
{
   U16BIT filter_index;
   U16BIT handler_index;
   S_PID_FILTER_INFO *pid_filter;
   BOOLEAN all_counts_zero;
   U8BIT i;
   BOOLEAN am_result;
   FUNCTION_START(STB_DMXStopPIDFilter);

   if (path < num_paths)
   {
      filter_index = (pfilt_id >> 8) & 0xff;
      i = (pfilt_id >> 4) & 0xf;

      pid_filter = &demux_status[path].filter_info[filter_index];

      STB_OSMutexLock(demux_status[path].config_mutex);

      if (pid_filter->started)
      {
//#ifdef FILTER_PRINTS
DMX_DBG(">> %s(%u, 0x%04x): start_count=%u", __FUNCTION__, path, pfilt_id, pid_filter->start_count[i]);
//#endif
         if (pid_filter->start_count[i] > 0)
         {
            pid_filter->start_count[i]--;
         }

         /* Check to see whether any filters are still running */
         all_counts_zero = TRUE;
         for (handler_index = 0; (handler_index < MAX_FILTERS_PER_PID) && all_counts_zero; handler_index++)
         {
            if (pid_filter->start_count[handler_index] != 0)
            {
               all_counts_zero = FALSE;
            }
         }

         if (all_counts_zero)
         {
#ifdef FILTER_PRINTS
printf(" - STOP");
#endif
//HOOK
#if 0
            /* Stop the filter and clear the callback */
            am_result = DMX_StopFilter(path, pid_filter->fhandle);
            if (!am_result)
            {
               DMX_ERR("%u: Failed to stop PID filter 0x%04x on PID %u, error %d", path,
                  pfilt_id, pid_filter->pid, am_result);
            }

            DMX_SetCallback(path, pid_filter->fhandle, NULL, NULL);
            DMX_FreeFilter(path, pid_filter->fhandle);
#endif
            DMX_StopFilter(pid_filter->fhandle);
            DMX_CloseFilter(pid_filter->fhandle);
            pid_filter->fhandle = -1;
            pid_filter->started = FALSE;

            demux_status[path].num_pid_filters_started--;
         }
#ifdef FILTER_PRINTS
printf("\n");
#endif
      }

      STB_OSMutexUnlock(demux_status[path].config_mutex);
   }

   FUNCTION_FINISH(STB_DMXStopPIDFilter);
}

/**
 * @brief   Copies a filtered section to caller's buffer
 * @param   path the demux path to use
 * @param   buffer the caller's buffer
 * @param   size the size of the caller's buffer
 * @param   pfilt_id the handle of the PID filter to read from
 * @return  TRUE copied ok
 * @return  FALSE no data to copy
 */
BOOLEAN STB_DMXCopyPIDFilterSect(U8BIT path, U8BIT *buffer, U16BIT size, U16BIT pfilt_id)
{
   BOOLEAN retval;
   U16BIT bytes_to_copy;
   U16BIT filter_index;
   S_PID_FILTER_INFO *pid_filter;

   FUNCTION_START(STB_DMXCopyPIDFilterSect);

   retval = FALSE;

   if (path < num_paths)
   {
      filter_index = (pfilt_id >> 8) & 0xff;
      pid_filter = &demux_status[path].filter_info[filter_index];

      /* If a data packet is available then copy the contents to the supplied buffer */
      if (pid_filter->data_packet_size > 0)
      {
         if (pid_filter->data_packet_size > size)
         {
            bytes_to_copy = size;
         }
         else
         {
            bytes_to_copy = pid_filter->data_packet_size;
         }

         if (bytes_to_copy > 0)
         {
            //DebugPrintBuffer((U8BIT *)pid_filter->data_packet,bytes_to_copy);
            //DMX_DBG("pid [%d ] data_packet %p  bytes_to_copy %d",pid_filter->pid,pid_filter->data_packet,bytes_to_copy);
            memcpy(buffer, pid_filter->data_packet, bytes_to_copy);
         }

         retval = TRUE;
      }
   }

   FUNCTION_FINISH(STB_DMXCopyPIDFilterSect);

   return retval;
}

/**
 * @brief   Flushes (emDMXes) the buffer of a speficied PID filter
 * @param   path the demux path of the filter
 * @param   pfilt_id the handle of the PID filter
 */
void STB_DMXFlushPIDFilterBuffer(U8BIT path, U16BIT pfilt_id)
{
   FUNCTION_START(STB_DMXFlushPIDFilterBuffer);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(pfilt_id);
   FUNCTION_FINISH(STB_DMXFlushPIDFilterBuffer);
}

/**
 * @brief   Skips (discards) a section in the PID filter buffer
 * @param   path the demux path of the filter
 * @param   pfilt_id the PID filter handle
 */
void STB_DMXSkipPIDFilterSect(U8BIT path, U16BIT pfilt_id)
{
   FUNCTION_START(STB_DMXSkipPIDFilterSect);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(pfilt_id);
   FUNCTION_FINISH(STB_DMXSkipPIDFilterSect);
}

/**
 * @brief   Returns the maximum number of section filters available on this hw
 * @return  The number of filters
 */
U8BIT STB_DMXGetMaxSectionFilters(void)
{
   FUNCTION_START(STB_DMXGetMaxSectionFilters);
   FUNCTION_FINISH(STB_DMXGetMaxSectionFilters);

   return MAX_SECTION_FILTERS;
}

/**
 * @brief   Gets the current source of a given demux
 * @param   path the demux path to query
 * @param   source the source of the demux
 * @param   param the source specific parameter (e.g. tuner number)
 */
void STB_DMXGetDemuxSource(U8BIT path, E_STB_DMX_DEMUX_SOURCE *source, U8BIT *param)
{
   FUNCTION_START(STB_DMXGetDemuxSource);

   if (path < num_paths)
   {
      *source = demux_status[path].source;
      *param = demux_status[path].source_param;
   }

   FUNCTION_FINISH(STB_DMXGetDemuxSource);
}
void STB_DMXSetDemuxSource(U8BIT path, E_STB_DMX_DEMUX_SOURCE source, U8BIT param, U16BIT demux_cap)
{
   FUNCTION_START(STB_DMXSetDemuxSource);
   DMX_DBG("path [%u], source[%u]  param[%0x] ,demux_cap[%0x] ",path,source,param,demux_cap);
   if ((path < num_paths) &&
       ((source != demux_status[path].source) || (param != demux_status[path].source_param)||(demux_cap != demux_status[path].demux_cap)))
   {
      DMX_DBG("path %u: new:  [%u], [%u]  [%0x];   old:  [%u], [%u]  [%0x];", path, source, param,demux_cap,
         demux_status[path].source, demux_status[path].source_param, demux_status[path].demux_cap);

      demux_status[path].source = source;
      demux_status[path].source_param = param;
      demux_status[path].demux_cap = demux_cap;
   }

   FUNCTION_FINISH(STB_DMXSetDemuxSource);
}

/**
 * @brief   Reset the source of the demux
 * @param   path the demux path to configure
 */
void STB_DMXResetDemuxSource(U8BIT path)
{
    STB_DMXSetDemuxSource(path, DMX_TUNER, 0, 0);
}

/**
 * @brief   Reads Teletext PES data from the demux
 * @param   path the demux path to read
 * @param   buffer pointer to PES data
 * @param   num_bytes the number of bytes of data
 */
void STB_DMXReadTextPES(U8BIT path, U8BIT **buffer, U32BIT *num_bytes)
{
   U8BIT* read_ptr;
   U8BIT* end_ptr;
   U32BIT bytes_available;
   U32BIT bytes_to_copy;

   FUNCTION_START(STB_DMXReadTextPES);

   *num_bytes = 0;
   *buffer = NULL;

   if ((path < num_paths) && demux_status[path].text_started)
   {
      STB_OSMutexLock(demux_status[path].text_mutex);

      if (demux_status[path].text_bytes_available != 0)
      {
         read_ptr = demux_status[path].read_ptr;
         end_ptr = demux_status[path].text_buffer + TEXT_BUFFER_SIZE;

         if (pes_data == NULL)
         {
            /* Create a buffer to copy the PES data into */
            pes_data = STB_MEMGetSysRAM(demux_status[path].text_bytes_available);
            pes_data_size = demux_status[path].text_bytes_available;
         }
         else if (pes_data_size < demux_status[path].text_bytes_available)
         {
            /* Buffer needs to be increased */
            pes_data = STB_MEMResizeSysRAM(pes_data, demux_status[path].text_bytes_available);
            pes_data_size = demux_status[path].text_bytes_available;
         }

         if (pes_data != NULL)
         {
            bytes_available = end_ptr - read_ptr;

            if (demux_status[path].text_bytes_available < bytes_available)
            {
               /* Data can be copied in one go */
               memcpy(pes_data, read_ptr, demux_status[path].text_bytes_available);

               demux_status[path].read_ptr += demux_status[path].text_bytes_available;
            }
            else
            {
               /* Data has wrapped round in the buffer */
               memcpy(pes_data, read_ptr, bytes_available);
               bytes_to_copy = demux_status[path].text_bytes_available - bytes_available;
               memcpy(pes_data + bytes_available, demux_status[path].text_buffer, bytes_to_copy);

               demux_status[path].read_ptr = demux_status[path].text_buffer + bytes_to_copy;
            }

            *num_bytes = demux_status[path].text_bytes_available;
            *buffer = pes_data;

            demux_status[path].text_bytes_available = 0;
         }
      }

      STB_OSMutexUnlock(demux_status[path].text_mutex);
   }

   FUNCTION_FINISH(STB_DMXReadTextPES);
}

/**
 * @brief   Writes data to the demux from memory
 * @param   path the demux path to be written
 * @param   data the data to be written
 * @param   size the number of bytes to be written
 */
void STB_DMXWriteDemux(U8BIT path, U8BIT *data, U32BIT size)
{
   FUNCTION_START(STB_DMXWriteDemux);

   //AV_InjectData(path,data,size);

   FUNCTION_FINISH(STB_DMXWriteDemux);
}

/**
 * @brief   Acquires a descrambler for the specified track on this path
 * @param   path the demux path for which the descrambler is acquired
 * @param   track enum representing audio, video or subtitles PES
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXGetDescramblerKey(U8BIT path, E_STB_DMX_DESC_TRACK track)
{
   BOOLEAN ret;
   BOOLEAN result = TRUE;

   FUNCTION_START(STB_DMXGetDescramblerKey);

   if ((path < num_paths) && (track < DESC_NUM_TRACKS))
   {
#if 0 //EMING
       int     dsc_dev;
       S_DSC_DEV_INFO *dsc;
      dsc_dev = dmx_model_sc2 ? path : DSC_DEV_NO;
      dsc = &dsc_dev_info[dsc_dev];
      ClearKey(path, track);
      STB_DMXDscSetSrc(dsc_dev, path);
#endif
      ClearKey(path, track);
      result = TRUE;
   }
   else
   {
      DMX_DBG("illegal path %u or track %u", path, track);

      result = FALSE;
   }

   FUNCTION_FINISH(STB_DMXGetDescramblerKey);

   return result;
}

/**
 * @brief   Frees the descrambler for the specified track on this path
 * @param   path the demux path for which the descrambler is freed
 * @param   track enum representing audio, video or subtitles PES
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXFreeDescramblerKey(U8BIT path, E_STB_DMX_DESC_TRACK track)
{
   S_DMX_STATUS* pdmx;
   S_DES_TRACK_INFO *ptrk;
   BOOLEAN ret;
   BOOLEAN result = TRUE;

   FUNCTION_START(STB_DMXFreeDescramblerKey);

   DMX_DBG("path %u track %u", path, track);

#if 0 //EMING
   int     dsc_dev;
   S_DSC_DEV_INFO *dsc;
   dsc_dev = dmx_model_sc2 ? path : DSC_DEV_NO;
   dsc     = &dsc_dev_info[dsc_dev];
#endif
   if ((path < num_paths) && (track < DESC_NUM_TRACKS))
   {
      pdmx = demux_status + path;
      ptrk = pdmx->tracks + track;
#if 1 //EMING
      ClearKey(path, track);
#endif
      result = TRUE;
   }
   else
   {
      result = FALSE;
   }

   FUNCTION_FINISH(STB_DMXFreeDescramblerKey);

   return result;
}

/**
 * @brief   Set the descrambler key data for the specified track on this path
 * @param   path the demux path for which the descrambler key data is set
 * @param   track enum representing audio, video or subtitles PES
 * @param   parity even or odd
 * @param   data pointer to the key data, its length depends on the descrambler
 *          type (see STB_DMXSetDescramblerType)
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXSetDescramblerKeyData(U8BIT path, E_STB_DMX_DESC_TRACK track,
   E_STB_DMX_DESC_KEY_PARITY parity, U8BIT *data)
{
   BOOLEAN result = TRUE;

   FUNCTION_START(STB_DMXSetDescramblerKeyData);

   DMX_DBG("path %u track %u parity %u data %02x %02x", path, track, parity, data[0], data[1]);
   if ((path < num_paths) && (track < DESC_NUM_TRACKS))
   {
      if (parity == KEY_PARITY_EVEN)
      {
         demux_status[path].tracks[track].iseven = TRUE;
         memcpy(demux_status[path].tracks[track].even, data, 32);
      }
      else
      {
         demux_status[path].tracks[track].isodd = TRUE;
         memcpy(demux_status[path].tracks[track].odd, data, 32);
      }

#if 1 //EMING
      ApplyKey(path, track);
#endif
      result = TRUE;
   }
   else
   {
      result = FALSE;
   }

   FUNCTION_FINISH(STB_DMXSetDescramblerKeyData);

   return result;
}

/**
 * @brief   Get the descrambler key usage for the specified track on this path as set by
 *          STB_DMXSetKeyUsage
 * @param   path the demux path that the descrambler key usage refers to
 * @param   track enum representing audio, video or subtitles PES
 * @param   key_usage whether the descrambler has been set to operate at PES level, transport
 *          level or all.
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXGetKeyUsage(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_KEY_USAGE *key_usage)
{
   BOOLEAN result = TRUE;

   FUNCTION_START(STB_DMXGetKeyUsage);

   if ((path < num_paths) && (track < DESC_NUM_TRACKS))
   {
      *key_usage = demux_status[path].tracks[track].usage;
   }
   else
   {
      result = FALSE;
   }

   FUNCTION_FINISH(STB_DMXGetKeyUsage);

   return result;
}

/**
 * @brief   Get Board type, different board has different demux structure.
 * @return  Board type E_STB_BOARD_TYPE.
 */
E_STB_BOARD_TYPE STB_DMXGetModel()
{
    struct stat st;
    int         r;
    static BOOLEAN                dmx_model_sc2 = FALSE;

   r = stat("/dev/key", &st);
   if (r == 0)
   {
      dmx_model_sc2 = TRUE;
   }

   if (dmx_model_sc2)
      return STB_DMX_MODEL_SC2;
   else
      return STB_DMX_MODEL_905X2;
}

/**
 * @brief   Set the descrambler key usage for the specified track on this path
 * @param   path the demux path that the descrambler key usage refers to
 * @param   track enum representing audio, video or subtitles PES
 * @param   key_usage whether the descrambler operates at PES level, transport
 *          level or all.
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXSetKeyUsage(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_KEY_USAGE key_usage)
{
   BOOLEAN result = TRUE;

   FUNCTION_START(STB_DMXSetKeyUsage);

   DMX_DBG("path %u track %u key_usg %u", path, track, key_usage);
   if ((path < num_paths) && (track < DESC_NUM_TRACKS))
   {
      demux_status[path].tracks[track].usage = key_usage;
   }
   else
   {
      result = FALSE;
   }

   FUNCTION_FINISH(STB_DMXSetKeyUsage);

   return result;
}

/**
 * @brief   Get the descrambler type for the specified track on this path, as set by
 *          STB_DMXSetDescramblerType
 * @param   path the demux path that the descrambler type refers to
 * @param   type descrambler type (DES, AES, etc...)
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXGetDescramblerType(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_DESC_TYPE *type)
{
   BOOLEAN result = TRUE;

   FUNCTION_START(STB_DMXGetDescramblerType);

   if ((path < num_paths) && (track < DESC_NUM_TRACKS))
   {
      *type = demux_status[path].tracks[track].type;
   }
   else
   {
      result = FALSE;
   }

   FUNCTION_FINISH(STB_DMXGetDescramblerType);

   return result;
}

/**
 * @brief   Set the descrambler type for the specified track on this path
 * @param   path the demux path that the descrambler type refers to
 * @param   type descrambler type (DES, AES, etc...)
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXSetDescramblerType(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_DESC_TYPE type)
{
   BOOLEAN result = TRUE;

   FUNCTION_START(STB_DMXSetDescramblerType);

   DMX_DBG("path %u track %u type %u", path, track, type);
   if ((path < num_paths) && (track < DESC_NUM_TRACKS))
   {
      demux_status[path].tracks[track].type = type;
   }
   else
   {
      result = FALSE;
   }

   FUNCTION_FINISH(STB_DMXSetDescramblerType);

   return result;
}


/**
 * @brief   Internal function that returns the decode PIDs for the given demux
 * @param   path demux path
 * @param   pcr_pid pointer for returned PCR PID value
 * @param   video_pid pointer for returned video PID value
 * @param   audio_pid pointer for returned audio PID value
 * @param   ad_pid pointer for returned AD PID value
 * @return  TRUE if demux is valid and PIDs are returned, FALSE otherwise
 */
BOOLEAN DMXGetDecodePIDs(U8BIT path, U16BIT *pcr_pid, U16BIT *video_pid, U16BIT *audio_pid, U16BIT *ad_pid, U8BIT *preselection_id)
{
   BOOLEAN retval = TRUE;

   FUNCTION_START(DMXGetDecodePIDs);

   if (path < num_paths)
   {
      *pcr_pid = demux_status[path].pids[DMX_PCR];
      *video_pid = demux_status[path].pids[DMX_VIDEO];
      *audio_pid = demux_status[path].pids[DMX_AUDIO];
      *ad_pid = demux_status[path].pids[DMX_ADES];
      *preselection_id = demux_status[path].pids[DMX_PRESELECTION];
      retval = TRUE;
   }
   else
   {
      retval = FALSE;
   }

   FUNCTION_FINISH(DMXGetDecodePIDs);

   return(retval);
}


/**
 * @brief   Callback function that receives data for PID and section filters
 */
//int fhandle, const uint8_t *data, int len, void *user_data
void PidCallback(ST_CALLBACK_T* param)
{
   S_PID_FILTER_INFO *pid_filter;
   U8BIT i, j;
   S_SECTION_FILTER_INFO *sect_filter;
   FILTER_CALLBACK func_ptr;
   U8BIT result;
   U16BIT sfi;

   FUNCTION_START(PidCallback);
//HOOK
   if (param->un32_userdata != NULL)
   {
      pid_filter = (S_PID_FILTER_INFO *)param->un32_userdata;

      if (pid_filter->fhandle == param->un32filterID)
      {
         pid_filter->data_packet = param->pun8_buffer;
         pid_filter->data_packet_size = param->un32_length;

         for (i = 0; i < MAX_SECTION_FILTERS; i++)
         {
            sect_filter = &pid_filter->section_filters[i];

            if (sect_filter->in_use)
            {
               if (!sect_filter->setup || sect_filter->empty_mask)
               {
                  for (j = 0; j < MAX_FILTERS_PER_PID; j++)
                  {
                     /* No section filtering: Call the handler */
                     if (pid_filter->func_ptr[j] != NULL)
                     {
                        func_ptr = pid_filter->func_ptr[j];
                        (*func_ptr)(pid_filter->path, (U16BIT)pid_filter->data_packet_size, ((pid_filter->index << 8) + (j << 4)));
                     }
                  }
               }
               else
               {
                  /* Check section filter
                   *
                   * We have:
                   *   s - actual section bit (0 or 1)
                   *   v - section filter value (0 or 1)
                   *   m - section filter mask (0 = disabled, 1 = enabled)
                   *   d - section filter mode (always 0)
                   *
                   * To check:
                   *   m & (s^v) == 0 if
                   *     m == 0 (don't care), or
                   *     s == v (good)
                   *   m & (s^v) == 1 if
                   *     m == 1 (check) and s != v (bad)
                   *
                   * So m & (s^v) == 0 if and only if section is good.
                   */
                  result = sect_filter->mask[0] & (param->pun8_buffer[0] ^ sect_filter->match[0]);

                  /* Different tables can be on the same PID, so if result doesn't equal
                   * 0 then this data is for a different table id */
                  if (result == 0)
                  {
                     for (sfi = 1; sfi < DEMUX_SECTION_FILTER_LENGTH; ++sfi)
                     {
                        /* Skip section length field */
                        result |= (sect_filter->mask[sfi] & (param->pun8_buffer[sfi+2] ^ sect_filter->match[sfi]));
                     }

                     if (result == 0)
                     {
                        for (j = 0; j < MAX_FILTERS_PER_PID; j++)
                        {
                           if (pid_filter->func_ptr[j] != NULL)
                           {
                              func_ptr = pid_filter->func_ptr[j];
                              //DebugPrintBuffer((U8BIT *)pid_filter->data_packet,pid_filter->data_packet_size);
                               //DMX_DBG("pid_filter->path [0x%x] pid[0x%x ]  SIZE[0x%x ]  pfilt_id[0x%x] ",pid_filter->path , pid_filter->pid,(U16BIT)pid_filter->data_packet_size,((pid_filter->index << 8) + (j << 4)));
                              (*func_ptr)(pid_filter->path, (U16BIT)pid_filter->data_packet_size, ((pid_filter->index << 8) + (j << 4)));
                              (*func_ptr)(0, (U16BIT)pid_filter->data_packet_size, ((pid_filter->index << 8) + (j << 4)));
                           }
                        }
                     }
                     else
                     {
                        const U8BIT*p = pid_filter->data_packet;
                        printf("  corrupt?: 0x%02x%02x%02x%02x%02x%02x%02x%02x match=0x%02x, mask=0x%02x\n",
                           p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
                           sect_filter->match[0], sect_filter->mask[0]);
                     }
                  }
               }
            }
         }

         /* Ensure that the packet of data is no longer available */
         pid_filter->data_packet = NULL;
         pid_filter->data_packet_size = 0;
      }
      else
      {
         DMX_ERR("Callback for demux %d, filter %d, user data is for %d!",
            pid_filter->index, param->un32filterID, pid_filter->fhandle);
      }
   }

   FUNCTION_FINISH(PidCallback);
}

/**
 * @brief    Update section filters for a PID filter
 * @param    path         - Required Decode Path Number.
 * @param    filter_index - index of (real) PID filter
 * @return   Whether the operation was successful.
 */
static BOOLEAN UpdateSectionFilter(U8BIT path, U16BIT filter_index)
{
   S_PID_FILTER_INFO *pid_filter;
   S_SECTION_FILTER_INFO *sect_filter;
   BOOLEAN check_crc_all, check_crc_some;
   U8BIT mask_all[DEMUX_SECTION_FILTER_LENGTH];
   U8BIT mask_some[DEMUX_SECTION_FILTER_LENGTH];
   U8BIT mask[DEMUX_SECTION_FILTER_LENGTH];
   U16BIT i, sfi;
   BOOLEAN success;
   struct dmx_sct_filter_params dvb_filt_p;
   U16BIT num_filters;
   BOOLEAN am_result;
   U16BIT source_type = 0;
   U8BIT source_path = 0;
   FUNCTION_START(UpdateSectionFilter);
   DMX_DBG("UpdateSectionFilter Start path: [%d] filter_index[%d] source[0x%x] source_param[0x%x]",path,filter_index,\
    demux_status[path].source,\
    demux_status[path].source_param);

   pid_filter = &demux_status[path].filter_info[filter_index];
   source_type = demux_status[path].source;
   source_path = demux_status[path].source_param;
   success = FALSE;

   /* Find new mask/match and CRC status
    *
    * The basic idea here is this: if a1,a2,... are boolean
    * variables, and
    *   b := (not ((a1 and a2 and ...) xor (a1 or a2 or ...)))
    * then b is true if a1=a2=... and false otherwise.
    *
    * This can be used for simple booleans (crc_check) or
    * for creating bit-masks for multiple matches
    *
    * In this context, "all" signifies that all are true, and
    * "some" signifies that some are true. If all == some then
    * we know that all the variables are the same:
    *
    *                          all
    *                   true          false
    *                +--------------------------------------+
    *          true  |  all true      some true, some false |
    *    some        |                                      |
    *          false |  impossible    all false             |
    *                +--------------------------------------+
    *
    * To calculate "all" and "some":
    * all  := true and a1 and a2 and ...
    * some := false or a1 or a2 or ...
    */

   /* Initialise "all" and "some" variables */
   memset(mask_all, 0xff, DEMUX_SECTION_FILTER_LENGTH);
   memset(mask_some, 0x00, DEMUX_SECTION_FILTER_LENGTH);
   memset(mask, 0xff, DEMUX_SECTION_FILTER_LENGTH);
   check_crc_all = TRUE;
   check_crc_some = FALSE;

   /* Create a match/mask from all section filters on this PID filter */
   for (i = 0, num_filters = 0; i < MAX_SECTION_FILTERS; i++)
   {
      sect_filter = &pid_filter->section_filters[i];

      if (sect_filter->setup)
      {
         /* Update "all" and "some" variables */
         for (sfi = 0; sfi < DEMUX_SECTION_FILTER_LENGTH; ++sfi)
         {
            mask_all[sfi] &= sect_filter->match[sfi];
            mask_some[sfi] |= sect_filter->match[sfi];
            mask[sfi] &= sect_filter->mask[sfi];
         }

         check_crc_all = check_crc_all && sect_filter->check_crc;
         check_crc_some = check_crc_some || sect_filter->check_crc;

         num_filters++;
      }
   }

   if (num_filters > 0)
   {
      /* Prepare final mask */
      for (sfi = 0; sfi < DEMUX_SECTION_FILTER_LENGTH; ++sfi)
      {
         mask[sfi] &= ~(mask_all[sfi] ^ mask_some[sfi]);
      }

      /* Final match can be taken from mask_all or mask_some - they
       * are exactly the same where it matters (according to mask).
       */

      /* Configure the driver's data structure */
      memset(&dvb_filt_p, 0, sizeof(struct dmx_sct_filter_params));

      dvb_filt_p.pid = pid_filter->pid;

      /* Update CRC checking */
      if (check_crc_all)
      {
         /* All interested in CRC checking */
         dvb_filt_p.flags |= DMX_CHECK_CRC;
      }

      /* section length is ignored by the AMLogic Linux DVB, so the filter and mask data
       * need to be passed in as they are */
      memcpy(&dvb_filt_p.filter.filter[0], &mask_all[0], DEMUX_SECTION_FILTER_LENGTH);
      memcpy(&dvb_filt_p.filter.mask[0], &mask[0], DEMUX_SECTION_FILTER_LENGTH);
      memcpy(sect_filter->match, &dvb_filt_p.filter.filter[0], DEMUX_SECTION_FILTER_LENGTH);
      memcpy(sect_filter->mask, &dvb_filt_p.filter.mask[0], DEMUX_SECTION_FILTER_LENGTH);

#if 0
//if (pid_filter->pid == 17)
{
   __u8 *pfilter = &dvb_filt_p.filter.filter[0];
   __u8 *pmask = &dvb_filt_p.filter.mask[0];

   printf("match=%02x%02x%02x%02x%02x%02x%02x%02x mask=%02x%02x%02x%02x%02x%02x%02x%02x\n",
      pfilter[0], pfilter[1], pfilter[2], pfilter[3], pfilter[4], pfilter[5], pfilter[6], pfilter[7],
      pmask[0], pmask[1], pmask[2], pmask[3], pmask[4], pmask[5], pmask[6], pmask[7]);
}
#endif
//HOOK
#if 0
      if (pid_filter->started)
      {
         if (pid_filter->fhandle != -1)
         {
            /* Stop the filter while it's updated */
            am_result = DMX_StopFilter(path, pid_filter->fhandle);
            if (!am_result)
            {
               DMX_ERR("%u: Failed to stop PID filter %d, error %d", path, pid_filter->fhandle, am_result);
            }
         }
      }

      if (pid_filter->fhandle == -1)
      {
         am_result = DMX_AllocateFilter(path, &pid_filter->fhandle);
         if (am_result)
         {
            if (Is_DDB_Filter(pid_filter))
            {
                am_result = DMX_SetBufferSize(path, pid_filter->fhandle,8 * MAX_DDB_SECTION_SIZE);
            }
            else
            {
                am_result = DMX_SetBufferSize(path, pid_filter->fhandle, 8 * MAX_SECTION_SIZE);
            }
            if (!am_result)
            {
               DMX_ERR("%u: Failed to set buffer size for section filter %u, error %d", path,
                  filter_index, am_result);
            }
         }
      }

      if (pid_filter->fhandle != -1)
      {
         am_result = DMX_SetSecFilter(path, pid_filter->fhandle, &dvb_filt_p);
         if (am_result)
         {
            success = TRUE;
         }
         else
         {
            DMX_ERR("%u: Failed to setup section filter %d, error %d", path, pid_filter->fhandle, am_result);
         }

         if (pid_filter->started)
         {
            /* Restart the filter */
            am_result = DMX_StartFilter(path, pid_filter->fhandle);
            if (!am_result)
            {
               DMX_ERR("%u: Failed to restart PID filter %d, error %d", path, pid_filter->fhandle, am_result);
            }
         }
      }
#endif

        if (pid_filter->started)
        {
          if (pid_filter->fhandle != -1)
          {
             /* Stop the filter while it's updated */
             DMX_StopFilter( pid_filter->fhandle);
          }
        }

        if (pid_filter->fhandle == -1)
        {
           //alloc
           pid_filter->fhandle = DMX_OpenFilter(source_path, PidCallback, (void*)pid_filter,source_type);
        }

        if (pid_filter->fhandle != -1)
        {
          /*setup*/
          //BOOLEAN DMX_SetupFilter(int un32filterID ,U16BIT pid,SECTION_FILTER_INFO params )
          /////
          ////
          DMX_SetupFilter(pid_filter->fhandle, pid_filter->pid, sect_filter);
          if (pid_filter->started)
          {
             /* Restart the filter */
             DMX_StartFilter(pid_filter->fhandle);
          }
        }

   }

   FUNCTION_FINISH(UpdateSectionFilter);
   DMX_DBG("UpdateSectionFilter end");

   return success;
}

/**
 * @brief   Callback function that receives PES data for DVB subtitles and EBU teletext
 */
static void PesCallback(int dev_no, int fhandle, const uint8_t *data, int len, void *user_data)
{
   S_DMX_STATUS* pdmx;
   U32BIT num_bytes;

   FUNCTION_START(PesCallback);

   if ((data != NULL) && (len != 0) && (user_data != NULL))
   {
      pdmx = (S_DMX_STATUS *)user_data;

      if ((pdmx->path == dev_no) && (pdmx->text_fhandle == fhandle))
      {
         STB_OSMutexLock(pdmx->text_mutex);

         /* Copy the data to the PES buffer */
         if (pdmx->text_bytes_available + len <= TEXT_BUFFER_SIZE)
         {
            num_bytes = pdmx->text_buffer + TEXT_BUFFER_SIZE - pdmx->write_ptr;

            if (num_bytes > (U32BIT)len)
            {
               /* There's room for all the data */
               memcpy(pdmx->write_ptr, data, len);
               pdmx->write_ptr += len;
            }
            else
            {
               /* Wrap round to write all the data */
               memcpy(pdmx->write_ptr, data, num_bytes);
               memcpy(pdmx->text_buffer, data + num_bytes, len - num_bytes);
               pdmx->write_ptr = pdmx->text_buffer + (len - num_bytes);
            }

            pdmx->text_bytes_available += len;
         }
         else
         {
            DMX_ERR("Buffer is full!");
         }

         STB_OSMutexUnlock(pdmx->text_mutex);
      }
      else
      {
         DMX_ERR("PES callback for demux %d, filter %d, but demux status is for %u and %d!",
            dev_no, fhandle, pdmx->path, pdmx->text_fhandle);
      }
   }

   FUNCTION_FINISH(PesCallback);
}

BOOLEAN STB_DMXSetSource(U8BIT dmx_idx, U8BIT src)
{
    return FALSE ;
}
void STB_DMXDscSetSrc(int dev_id, int dmx_id)
{
    //NA
}
void STB_DMXCI_Set_Demod_Mode(int mode)
{
    //NA
}
void STB_DMXChangeAllDemuxSource(U8BIT slot, U8BIT plug)
{
     if (1 == plug)
    {
        DMX_Route_TS(slot,TRUE);
    }
     else
    {
        DMX_Route_TS(slot,FALSE);
    }
}

static int key_open(void)
{
   char buf[32];
   int s_fd = -1;

   snprintf(buf, sizeof(buf), "/dev/key");
   s_fd = open(buf, O_RDWR);
   if (s_fd == -1)
   {
      DMX_DBG("key_open[%d] [%s]",s_fd,strerror(errno) );;
      return -1;
   }
   DMX_DBG("%s key fd:%d\n", buf, s_fd);
   return s_fd;
}

static int key_close(int fd)
{
   if (fd == -1)
   {
      DMX_DBG("key_close invalid fd\n");
      return -1;
   }
   close(fd);
   return 0;
}

static int key_alloc(int fd, int is_iv)
{
   int ret = 0;
   struct key_alloc param;

   DMX_DBG("fd %d is_iv %d\n", fd, is_iv);
   if (fd == -1)
   {
      DMX_DBG("key alloc fd invalid\n");
      return -1;
   }
   param.is_iv = is_iv;
   param.key_index = -1;

   ret = ioctl(fd, KEY_ALLOC, &param);
   if (ret == 0)
   {
      DMX_DBG("key_alloc index----------:[%x]\n", param.key_index);
      return param.key_index;
   }
   else
   {
      DMX_DBG("key_alloc key fail,fd:%d, is_iv:%d\n", fd, is_iv);
      return -1;
   }
}

static int key_config(int fd, int key_index, int key_userid, int key_algo, unsigned int ext_value)
{
   int ret = 0;
   struct key_config config;

   if (fd == -1)
   {
      DMX_DBG("key config fd invalid\n");
      return -1;
   }
   config.key_userid = key_userid;
   config.key_algo = key_algo;
   config.key_index = key_index;
   config.ext_value = ext_value;

   DMX_DBG("fd %d key_index:%d key_userid %d algo %d\n", fd, key_index, key_userid, key_algo);

   ret = ioctl(fd, KEY_CONFIG, &config);
   if (ret == 0)
   {
      DMX_DBG("key_config index:%d\n", config.key_index);
      return config.key_index;
   }
   else
   {
      DMX_DBG("key_config key fail,fd:%d, key_userid:%d, key_algo:%d\n", fd, key_userid, key_algo);
      return -1;
   }
}

static int key_set(int fd, int key_index, char *key, int key_len)
{
   int ret = 0;
   struct key_descr key_d;

   if (fd == -1 || key_index == -1 || key_len > 32)
   {
      DMX_DBG("key_set invalid parameter, fd:%d, key_index:%d, key_len:%d\n",
             fd, key_index, key_len);
      return -1;
   }

   DMX_DBG("fd %d key_index %d key %02x%02x%02x len %d", fd, key_index, key[0], key[1], key[2], key_len);
   key_d.key_index = key_index;
   memcpy(&key_d.key, key, key_len);
   key_d.key_len = key_len;
   ret = ioctl(fd, KEY_SET, &key_d);
   if (ret == 0)
   {
      DMX_DBG("key_set success\n");
      return 0;
   }
   else
   {
      DMX_DBG("key_set fail\n");
      return -1;
   }
}

static void key_free (int key_fd, int key_id)
{
   DMX_DBG("dev_id %d key_id %d", key_fd, key_id);
   ioctl(key_fd, KEY_FREE, key_id);
}

int STB_DMXDscAlloc(int dev_id, int pid, E_STB_DMX_DESC_TYPE type, E_STB_DSC_CA_TYPE dsc_type)
{
   int chan_id = -1;
   int i, r, id;
   char name[256];

   DMX_DBG("dev %d pid %x dsc_type %d %s", dev_id, pid, type, name);

   //{
      S_SC2_DSC_DEV_INFO *dsc = sc2_dsc_dev_info;
      struct ca_sc2_descr_ex desc = {0};
      enum ca_sc2_algo_type  algo;
      struct s_sc2_dsc_channel *dsc_channel = NULL;
      E_STB_TS_SOURCE ts_src;

      STB_OSMutexLock(dsc->mutex);
      for (i = 0; i < SC2_DSC_CH_NUM; i++)
      {
         dsc_channel = &dsc->dsc_pid_channel[i];
         if (dsc_channel->chan_id == -1 )
         {
             DMX_DBG("+++++++++++++++++++++++++++alloc new pid channel, [%d]", i);
             chan_id = i;
            break ;
         }
      }

      if (dsc_channel == NULL)
          DMX_DBG("@@@@@@@@@@flow wrong");
      {
         switch (type)
         {
         case DESC_TYPE_DVB:
            algo = CA_ALGO_CSA2;
            break;
         case DESC_TYPE_AES:
            algo = CA_ALGO_AES_CBC_CLR_END;
            break;
         case DESC_TYPE_AES_SCTE_52:
            algo = CA_ALGO_AES_CBC_IDSA;
            break;
         case DESC_TYPE_DES:
            algo = CA_ALGO_DES_SCTE52;
            break;
         case DESC_TYPE_TDES:
            algo = CA_ALGO_TDES_ECB_CLR_END;
            break;
         default:
            break;
         }

        /*
        desc.cmd = CA_ALLOC;
        desc.params.alloc_params.pid = pid;
        desc.params.alloc_params.algo = algo;
        desc.params.alloc_params.dsc_type = (enum ca_sc2_dsc_type)dsc_type;
        desc.params.alloc_params.ca_index = -1;

        DMX_DBG("type %d algo %d dsc_type %d", type, algo, dsc_type);
        r = ioctl(dsc->dsc_fd[dev_id], CA_SC2_SET_DESCR_EX, &desc);
        chan_id = desc.params.alloc_params.ca_index;
        */

         dsc_channel->ref = 1;
         dsc_channel->chan_id = chan_id;
         dsc_channel->pid = pid;
         dsc_channel->dsc_type = dsc_type;
         ////////////////////////////
         dsc_channel->key_id = -1;
         dsc_channel->iv_key_id = -1;
        ///////////////////
      }
      STB_OSMutexUnlock(dsc->mutex);
      //ca_dump_channel();
       //tuner hal flow
      // open descramble
      if (dsc->dsc_ref[dev_id] == 0)
      {
           dsc->descramble_handle = DESCRAMBLE_Open();
      }
      dsc->dsc_ref[dev_id]++;
   return chan_id;
}

int STB_DMXSetKey(int dev_id, int chan_id, E_STB_DMX_DESC_TYPE type, E_STB_DSC_CA_TYPE dsc_type, E_STB_DMX_DESC_KEY_PARITY parity, U8BIT *data)
{
   int r = 0;
   int i;
   int dsm_result = -1;
   DMX_DBG("setkey: %x %x %x", data[0], data[1], data[2]);
   DMX_DBG("@@@@@@@@@@@@  dev %d chan_id %d type %d parity %d dsc_type %d is_sc2 %d", dev_id, chan_id, type, parity, dsc_type, 1);

   //{
      S_SC2_DSC_DEV_INFO *dsc = sc2_dsc_dev_info;
      E_KEY_ALGO_SC2 key_algo;
      struct s_sc2_dsc_channel *dsc_channel;
      int key_userid = 0;

      STB_OSMutexLock(dsc->mutex);

      for (i = 0; i < SC2_DSC_CH_NUM; i++)
      {
         dsc_channel = &dsc->dsc_pid_channel[i];
         if (dsc_channel->chan_id == chan_id)
         {
            DMX_DBG("set key found channel");
            break ;
         }
      }
      if (dsc_channel == NULL)
          DMX_DBG("@@@@@@@@@@flow wrong");

      switch (dsc_type)
      {
         case DSC_COMMON_TYPE:
            key_userid = DSC_NETWORK;
            break;
         case DSC_TSD_TYPE:
            key_userid = DSC_LOC_DEC;
            break;
         case DSC_TSE_TYPE:
            key_userid = DSC_LOC_ENC;
            break;
         default:
            key_userid = DSC_NETWORK;
            break;
      }

      switch (type)
      {
         case DESC_TYPE_AES:
            key_algo = KEY_ALGO_AES;
            break;
         case DESC_TYPE_DVB:
            key_algo = KEY_ALGO_CSA2;
            break;
         case DESC_TYPE_DES:
            key_algo = KEY_ALGO_DES;
            break;
         case DESC_TYPE_TDES:
            key_algo = KEY_ALGO_TDES;
            break;
         default:
            DMX_DBG("key type invalid");
            break;
      };

      E_CA_KEY_TYPE_SC2 key_type;
      E_CA_KEY_TYPE_SC2 iv_key_type;

      switch (parity)
      {
      case KEY_PARITY_EVEN:
            key_type = CA_KEY_EVEN_TYPE;
            iv_key_type = CA_KEY_EVEN_IV_TYPE;
            break;
      case KEY_PARITY_ODD:
            key_type = CA_KEY_ODD_TYPE;
            iv_key_type = CA_KEY_ODD_IV_TYPE;
            break;
      case KEY_PARITY_NONE:
            key_type = CA_KEY_00_TYPE;
            iv_key_type = CA_KEY_00_IV_TYPE;
            break;
      }
   /*ALL ES share one Key TABLe*/
      if (dsc->dsc_ref[dev_id] == 1)
    {
          if (dsc_channel->key_id == -1)
          {
                dsc_channel->key_id = key_alloc(dsc->key_fd, FALSE);
                key_config(dsc->key_fd, dsc_channel->key_id, key_userid, key_algo, 0);
          }
          if (dsc_channel->iv_key_id == -1)
          {
                dsc_channel->iv_key_id = key_alloc(dsc->key_fd, TRUE);
                key_config(dsc->key_fd, dsc_channel->iv_key_id, key_userid, key_algo, 0);
          }
          /* set key */
          key_set(dsc->key_fd, dsc_channel->key_id, data, 16);
          //ca_set_key(dev_id, chan_id, key_type, *key_id);
          /* set iv */
          key_set(dsc->key_fd, dsc_channel->iv_key_id, data + 16, 16);
          //ca_set_key(dev_id, chan_id, iv_key_type, *iv_key_id);
          /*
           ca_set_key(dev_id, chan_id, key_type, *key_id);
           {
              static int ca_set_key(int dev_id, int index, int parity, unsigned int key_index)
              desc.cmd = CA_KEY;
              desc.params.key_params.ca_index = index;
              desc.params.key_params.parity = parity;
              desc.params.key_params.key_index = key_index;
              ret = ioctl(fd, CA_SC2_SET_DESCR_EX, &desc);
          }
          */

          dsm_result = DSM_SetProperty(dsc->dsm_handle, DSM_PROP_SC2_DSC_TYPE, DSM_PROP_SC2_DSC_TYPE_TSN);
          DMX_DBG("dsm_result %d",dsm_result);

          dsm_result = DSM_SetProperty(dsc->dsm_handle, DSM_PROP_DEC_SLOT_READY, DSM_PROP_SLOT_IS_READY);
          DMX_DBG("dsm_result %d",dsm_result);

          dsm_result = DSM_SetProperty(dsc->dsm_handle, DSM_PROP_ENC_SLOT_READY, DSM_PROP_SLOT_IS_READY);
          DMX_DBG("dsm_result %d",dsm_result);

          struct dsm_keyslot keyslot;
          keyslot.parity = (parity = KEY_PARITY_EVEN) ? DSM_PARITY_EVEN : DSM_PARITY_ODD;
          keyslot.algo = DSM_ALGO_AES_CBC_IDSA;
          keyslot.id = dsc_channel->key_id;
          keyslot.is_iv = FALSE;
          keyslot.is_enc = FALSE ;
          DMX_DBG("dsm_result %d parity[%x] is_iv[%x]  id[%x]",dsm_result,keyslot.parity,keyslot.is_iv,keyslot.id);
          dsm_result = DSM_AddKeySlot(dsc->dsm_handle, &keyslot);

          struct dsm_keyslot keyslot_iv;
          keyslot_iv.parity = (parity = KEY_PARITY_EVEN) ? DSM_PARITY_EVEN : DSM_PARITY_ODD;
          keyslot_iv.algo = CA_ALGO_AES_CBC_CLR_END;
          keyslot_iv.id = dsc_channel->iv_key_id;
          keyslot_iv.is_iv = TRUE;
          keyslot_iv.is_enc = FALSE ;
          DMX_DBG("dsm_result %d parity[%x] is_iv[%x]  id[%x]",dsm_result,keyslot_iv.parity,keyslot_iv.is_iv,keyslot_iv.id);
          dsm_result = DSM_AddKeySlot(dsc->dsm_handle, &keyslot_iv);
          uint32_t token =dsc->dsm_token;
          DMX_DBG("dsm_token [0x%x] 0[%x]1[%x]2[]3[%x]4[%x]",dsc->dsm_token,(token & 0xFF),((token & 0xFF00) >> 8),((token & 0xFF0000) >> 16),((token >> 24) & 0xFF));
          DESCRAMBLE_SetKeyToken(dsc->descramble_handle,dsc->dsm_token);
    }
      DESCRAMBLE_AddPid(dsc->descramble_handle,dsc_channel->pid);

      //add pid here
      STB_OSMutexUnlock(dsc->mutex);
   //}


   return r;
}

void STB_DMXDscFree(int dev_id, int chan_id)
{
   int i;
   int r;

      S_SC2_DSC_DEV_INFO *dsc = sc2_dsc_dev_info;
      struct ca_sc2_descr_ex desc;
      struct s_sc2_dsc_channel* dsc_channel = NULL;

      STB_OSMutexLock(dsc->mutex);


      for (i = 0; i < SC2_DSC_CH_NUM; i++)
      {
         dsc_channel = &dsc->dsc_pid_channel[i];
         if (dsc_channel->chan_id == chan_id)
         {
            DMX_DBG("free found channel");
            break ;
         }
      }

      if (dsc_channel == NULL)
          DMX_DBG("@@@@@@@@@@flow wrong");

    //desc.cmd = CA_FREE;
    //desc.params.free_params.ca_index = chan_id;
    //r = ioctl(dsc->dsc_fd[dev_id], CA_SC2_SET_DESCR_EX, &desc);

    if (dsc_channel->key_id != -1)
    {
        DSM_RemoveKeySlot(dsc->dsm_handle, dsc_channel->key_id);
        key_free(dsc->key_fd, dsc_channel->key_id);
    }

    if (dsc_channel->iv_key_id != -1)
    {
        DSM_RemoveKeySlot(dsc->dsm_handle, dsc_channel->key_id);
        key_free(dsc->key_fd, dsc_channel->iv_key_id);
    }

      dsc_channel->key_id = -1;
      dsc_channel->iv_key_id = -1;

      dsc_channel->src = STB_TS_SOURCE_MAX;
      dsc_channel->dsc_type = -1;
      dsc_channel->chan_id = -1;
      dsc_channel->ref = 0;
      dsc->dsc_ref[dev_id]--;
      //ca_dump_channel();
      DMX_DBG("---------------------------------free pid channel, [%d]", i);
      STB_OSMutexUnlock(dsc->mutex);

      //tuner hal flow
      //remove pid
      //close descramble
      DESCRAMBLE_RemovePid(dsc->descramble_handle,dsc_channel->pid);
      dsc_channel->pid = -1;

      if (dsc->dsc_ref[dev_id] == 0)
      {
          DESCRAMBLE_close(dsc->descramble_handle);
          dsc->descramble_handle = NULL;
      }
}

/**
 * @brief   Apply descrambler keys
 * @param   param - demux path
 */
static void ApplyKey(U8BIT path, E_STB_DMX_DESC_TRACK track)
{
   S_DMX_STATUS* pdmx;
   S_DES_TRACK_INFO *ptrk;
   BOOLEAN ret;
   int desc_chan;
   int dsc_dev;

   dsc_dev =   path ;

   pdmx = demux_status + path;
   ptrk = pdmx->tracks + track;

   DMX_DBG("path %d ptrk->chanid %d even %d odd %d pid %d track %d", path, ptrk->chanid, ptrk->iseven, ptrk->isodd, pdmx->pids[track], track);

   if (pdmx->pids[track] == 0)
   {
      DMX_DBG("pid is zero, return");
      return;
   }

   if (ptrk->iseven || ptrk->isodd)
   {
      if (ptrk->chanid == -1)
      {
         ptrk->chanid = STB_DMXDscAlloc(dsc_dev, pdmx->pids[track], ptrk->type, DSC_COMMON_TYPE);
         if (ptrk->chanid == -1)
         {
            DMX_DBG("----------------dsc alloc failed");
            return;
         }
      }
    if (ptrk->iseven)
    {
        DMX_DBG("SET EVEN");
        STB_DMXSetKey(dsc_dev, ptrk->chanid, ptrk->type, DSC_COMMON_TYPE, KEY_PARITY_EVEN, ptrk->even);
    }
    if (ptrk->isodd)
    {
        DMX_DBG("ODD EVEN");
        STB_DMXSetKey(dsc_dev, ptrk->chanid, ptrk->type, DSC_COMMON_TYPE, KEY_PARITY_ODD, ptrk->odd);
    }
   }
}

/**
 * @brief   Clear descrambler keys
 * @param   param - demux path
 */
static void ClearKey(U8BIT path, E_STB_DMX_DESC_TRACK track)
{
   S_DMX_STATUS* pdmx;
   S_DES_TRACK_INFO *ptrk;
   BOOLEAN ret;
   int dsc_dev = path;

   DMX_DBG("Clear key track==> [%d]",track);


   pdmx = demux_status + path;
   ptrk = pdmx->tracks + track;

   ptrk->iseven = FALSE;
   memset(ptrk->even, 0, 32);
   ptrk->isodd = FALSE;
   memset(ptrk->odd, 0, 32);

   if (ptrk->chanid != -1)
   {
      STB_DMXDscFree(dsc_dev, ptrk->chanid);
      ptrk->chanid = -1;
   }
}
