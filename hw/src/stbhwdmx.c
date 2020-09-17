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

#define DEMUX_DEBUG
#define FILTER_PRINTS

/*---includes for this file---------------------------------------------------*/
/* compiler library header files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

/* third party header files */
#include <linux/dvb/dmx.h>
#ifdef USE_TSPLAYER
#include "dvb_utils.h"
#else
#include <am_adp/am_dmx.h>
#endif

/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"

#include "internal.h"
#include "stbhwdef.h"
#include "stbhwc.h"
#include "stbhwos.h"
#include "stbhwdmx.h"
#include "stbhwmem.h"
#include "linuxdvbdmx_wrapper.h"


//#define DEMUX_DEBUG 1
/*---constant definitions for this file--------------------------------------*/
#define DMX_ERR(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
//#define DEMUX_DEBUG
#ifdef DEMUX_DEBUG
#define DMX_DBG(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define DMX_DBG(x,...)
#endif


#define DMX_TASK_PRIORITY           12
#define DMX_TASK_STACK_SIZE         4096

#define MAX_SECTION_SIZE            4096
#define DEMUX_SECTION_FILTER_LENGTH 8

#define MAX_PID_FILTERS             24
#define MAX_SECTION_FILTERS         16
#define MAX_FILTERS_PER_PID         8

#define DEMUX_FILTER_NOT_ALLOCATED  0xFFFF
#define DEMUX_PID_NOT_USED          0xFFFF
#define INVALID_PID                 0x1FFF

#define TEXT_BUFFER_SIZE            (65 * 1024)

#define DSC_DEV_NO                  0

#define STB_TSO_SOURCE "/sys/class/stb/tso_source"
/* Local ENUM/TYPE Definitions */

typedef enum
{ /* E_STB_DMX_DESC_TRACK is subset of this enum (first three correspond)*/
   DMX_AUDIO,
   DMX_VIDEO,
   DMX_TEXT,
   DMX_PCR,
   DMX_ADES,
   DMX_PID_COUNT
} E_DMX_TRACK;


typedef void(*SectionFilterFunc)(U8BIT path, U16BIT bytes, U16BIT pfilt_id);

typedef struct s_section_filter_info
{
   BOOLEAN in_use;
   U8BIT match[DEMUX_SECTION_FILTER_LENGTH];
   U8BIT mask[DEMUX_SECTION_FILTER_LENGTH];
   BOOLEAN check_crc;
   BOOLEAN setup;
   BOOLEAN empty_mask;
} S_SECTION_FILTER_INFO;

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

typedef struct s_des_track_info
{
   int chanid;
   E_STB_DMX_DESC_TYPE type;
   E_STB_DMX_KEY_USAGE usage;
   BOOLEAN iseven;
   BOOLEAN isodd;
   U8BIT even[32];
   U8BIT odd[32];
} S_DES_TRACK_INFO;

typedef struct
{
   U8BIT path;
   U16BIT caps;

   void *config_mutex;

   E_STB_DMX_DEMUX_SOURCE source;
   U8BIT source_param;

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


/*---local (static) variable declarations for this file----------------------*/
static S_DMX_STATUS* demux_status;
static U8BIT num_paths;

static U8BIT* pes_data = NULL;
static U32BIT pes_data_size = 0;

/*---local function prototypes for this file---------------------------------*/
static BOOLEAN UpdateSectionFilter(U8BIT path, U16BIT filter_index);

static void PidCallback(int dev_no, int fhandle, const uint8_t *data, int len, void *user_data);
static void PesCallback(int dev_no, int fhandle, const uint8_t *data, int len, void *user_data);
static void ApplyKey(U8BIT path, E_STB_DMX_DESC_TRACK track);
static void ClearKey(U8BIT path, E_STB_DMX_DESC_TRACK track);
static void STB_SetTsoutSource(void);
#ifdef USE_TSPLAYER
static DVB_DemuxSource_t GetDemuxSourceByCfg(U8BIT ts_input_idx);
#endif

/*---global function definitions---------------------------------------------*/


/**
 * @brief   Initialises the demux / programmable transport interface
 * @param   paths Number of demux paths to be initialised
 * @param   inc_pes_collection Not used
 */
void STB_DMXInitialise(U8BIT paths, BOOLEAN inc_pes_collection)
{
#ifdef USE_TSPLAYER
   BOOLEAN am_result = FALSE;
#else
   AM_ErrorCode_t am_result;
   AM_DMX_OpenPara_t open_params;
#endif
   U16BIT i;
   U16BIT j;

   char buf[128];
   char cmd[32];

   FUNCTION_START(STB_DMXInitialise);

   DMX_DBG("%u demuxes--, %s PES colection", paths, inc_pes_collection ? "with" : "no");
   STB_SetTsoutSource();
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
#ifdef USE_TSPLAYER
         for (i = 0; i < num_paths; i++)
         {
            am_result = AML_DMX_Open(i);
            if (am_result)
#else
         memset(&open_params, 0, sizeof(open_params));
         for (i = 0; i < num_paths; i++)
         {
            am_result = AM_DMX_Open(i, &open_params);
            if (am_result == AM_SUCCESS)
#endif
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
                  }

                  /* Set default values */
                  for (j = 0; j < MAX_PID_FILTERS; j++)
                  {
                     memset(&demux_status[i].filter_info[j], 0, sizeof(demux_status[i].filter_info[j]));

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
                  am_result = AML_DMX_FileEcho(buf, cmd);
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
      }
   }
   else
   {
      DMX_DBG("No demuxes found!");
   }

#if 0
   for (i = 0; i < 2; i++)
   {
      AM_DSC_OpenPara_t dsc_para;
      BOOLEAN ret;
      memset(&dsc_para, 0, sizeof(dsc_para));
      ret = AML_DSC_Open(i, &dsc_para);
      if (ret != AM_SUCCESS)
      {
         DMX_ERR("AM_DSC_Open error=%x", ret);
      }
      else
      {
         ret = AML_DSC_SetSource(i, i);
         if (ret != AM_SUCCESS)
         {
            DMX_ERR("AM_DSC_SetSource error=%x", ret);
         }
         ret = AML_DSC_Close(i);
         if (ret != AM_SUCCESS)
         {
            DMX_ERR("AM_DSC_Close error=%x", ret);
         }
      }
   }
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
   U16BIT text_pid, U16BIT data_pid, U16BIT ad_pid)
{
   U16BIT *pids;
   FUNCTION_START(STB_DMXChangeDecodePIDs);
   USE_UNWANTED_PARAM(data_pid);

   DMX_DBG("%u: pcr=%u, video=%u, audio=%u, text=%u, ad=%u", path, pcr_pid, video_pid, audio_pid,
      text_pid, ad_pid);

   if ((path < num_paths) && (demux_status[path].config_mutex != NULL))
   {
      pids = demux_status[path].pids;

      pids[DMX_PCR] = pcr_pid;
      pids[DMX_ADES] = ad_pid;

      if (pids[DMX_AUDIO] != audio_pid)
      {
         pids[DMX_AUDIO] = audio_pid;
         if (audio_pid != 0)
         {
            ApplyKey(path, DESC_TRACK_AUDIO);
         }
         else
         {
            ClearKey(path, DESC_TRACK_AUDIO);
         }
      }
      if (pids[DMX_VIDEO] != video_pid)
      {
         pids[DMX_VIDEO] = video_pid;
         if (video_pid != 0)
         {
            ApplyKey(path, DESC_TRACK_VIDEO);
         }
         else
         {
            ClearKey(path, DESC_TRACK_VIDEO);
         }
      }

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
#ifdef USE_TSPLAYER
   BOOLEAN am_result;
#else
   AM_ErrorCode_t am_result;
#endif

   struct dmx_pes_filter_params pes_params;

   FUNCTION_START(STB_DMXChangeTextPID);

   if ((path < num_paths) && (demux_status[path].pids[DMX_TEXT] != text_pid))
   {
      if (demux_status[path].text_fhandle >= 0)
      {
         if (demux_status[path].text_started)
         {
            /* Stop the filter and clear the callback */
#ifdef USE_TSPLAYER
            AML_DMX_StopFilter(path, demux_status[path].text_fhandle);
            AML_DMX_SetCallback(path, demux_status[path].text_fhandle, NULL, NULL);
            AML_DMX_FreeFilter(path, demux_status[path].text_fhandle);
#else
            AM_DMX_StopFilter(path, demux_status[path].text_fhandle);
            AM_DMX_SetCallback(path, demux_status[path].text_fhandle, NULL, NULL);
            AM_DMX_FreeFilter(path, demux_status[path].text_fhandle);
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
            ApplyKey(path, DESC_TRACK_TEXT);
         }
         else
         {
            ClearKey(path, DESC_TRACK_TEXT);
         }
      }

      if ((text_pid == 0) || (text_pid == 0xffff))
      {
         /* Set invalid PID value */
         text_pid = INVALID_PID;
      }

      if (text_pid != INVALID_PID)
      {
         /* Open a demux instance for the text (subtitle) PES */
#ifdef USE_TSPLAYER
         am_result = AML_DMX_AllocateFilter(path, &demux_status[path].text_fhandle);
         if (am_result)
         {
            DMX_DBG("%u: Opened text PES filter, handle=%d", path, demux_status[path].text_fhandle);
            AML_DMX_SetBufferSize(path, demux_status[path].text_fhandle, TEXT_BUFFER_SIZE);
#else
         am_result = AM_DMX_AllocateFilter(path, &demux_status[path].text_fhandle);
         if (am_result == AM_SUCCESS)
         {
            DMX_DBG("%u: Opened text PES filter, handle=%d", path, demux_status[path].text_fhandle);
            AM_DMX_SetBufferSize(path, demux_status[path].text_fhandle, TEXT_BUFFER_SIZE);

#endif
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
#ifdef USE_TSPLAYER
            am_result = AML_DMX_SetPesFilter(path, demux_status[path].text_fhandle, &pes_params);
            if (am_result == FALSE)
#else
            am_result = AM_DMX_SetPesFilter(path, demux_status[path].text_fhandle, &pes_params);
            if (am_result != AM_SUCCESS)
#endif
            {
               DMX_ERR("%u: Failed to set PID %u, handle %u, error %d",
                  path, text_pid, demux_status[path].text_fhandle, am_result);
            }
            else
            {
               if (demux_status[path].pids[DMX_TEXT] != 0)
               {
#ifdef USE_TSPLAYER
                  am_result = AML_DMX_SetCallback(path, demux_status[path].text_fhandle, PesCallback,
                     (void *)&demux_status[path]);
                  if (am_result)
                  {
                     /* Can now restart PES collection and the PES task */
                     am_result = AML_DMX_StartFilter(path, demux_status[path].text_fhandle);
                     if (am_result)
#else
                  am_result = AM_DMX_SetCallback(path, demux_status[path].text_fhandle, PesCallback,
                     (void *)&demux_status[path]);
                  if (am_result == AM_SUCCESS)
                  {
                     /* Can now restart PES collection and the PES task */
                     am_result = AM_DMX_StartFilter(path, demux_status[path].text_fhandle);
                     if (am_result == AM_SUCCESS)
#endif
                     {
                        demux_status[path].text_started = TRUE;
                     }
                     else
                     {
                        /* Filter not started so clear the callback */
#ifdef USE_TSPLAYER
                        AML_DMX_SetCallback(path, demux_status[path].text_fhandle, NULL, NULL);
#else
                        AM_DMX_SetCallback(path, demux_status[path].text_fhandle, NULL, NULL);
#endif

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
   }

   FUNCTION_FINISH(STB_DMXChangeTextPID);
}

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
//printf(">> %s(%u): pid=%u, fd=%d, func=%p, 0x%04x - NEW\n", __FUNCTION__, path, pid, filter_ptr->filter_fd, func_ptr, pfilt_id);
#endif
      }
      else
      {
         DMX_ERR("%u: No more filters available for pid %u", path, pid);
      }

      STB_OSMutexUnlock(demux_status[path].config_mutex);
   }

   FUNCTION_FINISH(STB_DMXGrabPIDFilter);

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
#ifdef FILTER_PRINTS
printf(">> %s(%u, 0x%04x): pid=%u, sfilt=0x%04x\n", __FUNCTION__, path, pfilt_id, filter_ptr->pid, sfilt_id);
#endif
      }

      STB_OSMutexUnlock(demux_status[path].config_mutex);
   }

   FUNCTION_FINISH(STB_DMXGrabSectFilter);

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

   if (path < num_paths)
   {
      /* Section filter ID includes the PID filter ID */
      pid_filter_index = sfilt_id >> 8;
      sect_filter_index = sfilt_id & 0x0f;

      sect_filter = &demux_status[path].filter_info[pid_filter_index].section_filters[sect_filter_index];

      STB_OSMutexLock(demux_status[path].config_mutex);

#ifdef FILTER_PRINTS
printf(">> %s(%u, 0x%04x): in_use=%u\n", __FUNCTION__, path, sfilt_id, sect_filter->in_use);
#endif
      if (sect_filter->in_use)
      {
         sect_filter->in_use = FALSE;
         sect_filter->setup = FALSE;

         UpdateSectionFilter(path, pid_filter_index);
      }

      STB_OSMutexUnlock(demux_status[path].config_mutex);
   }

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
#ifdef USE_TSPLAYER
   BOOLEAN am_result = TRUE;
#else
   AM_ErrorCode_t am_result;
#endif

   FUNCTION_START(STB_DMXStartPIDFilter);

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

      if (!pid_filter->started)
      {
         if (pid_filter->fhandle == -1)
         {
#ifdef USE_TSPLAYER
            am_result = AML_DMX_AllocateFilter(path, &pid_filter->fhandle);
            if (am_result)
            {
               am_result = AML_DMX_SetBufferSize(path, pid_filter->fhandle,
                  8 * MAX_SECTION_SIZE);
               if (!am_result)
#else
            am_result = AM_DMX_AllocateFilter(path, &pid_filter->fhandle);
            if (am_result == AM_SUCCESS)
            {
               am_result = AM_DMX_SetBufferSize(path, pid_filter->fhandle,
                  8 * MAX_SECTION_SIZE);
               if (am_result != AM_SUCCESS)
#endif
               {
                  DMX_ERR("%u: Failed to set buffer size for section filter %u, error %d", path,
                     filter_index, am_result);
               }
            }
            UpdateSectionFilter(path, filter_index);
         }

         if (pid_filter->fhandle != -1)
         {
#ifdef USE_TSPLAYER
            am_result = AML_DMX_SetCallback(path, pid_filter->fhandle, PidCallback, (void *)pid_filter);
            if (am_result)
            {
               am_result = AML_DMX_StartFilter(path, pid_filter->fhandle);
               if (am_result)
#else
            am_result = AM_DMX_SetCallback(path, pid_filter->fhandle, PidCallback, (void *)pid_filter);
            if (am_result == AM_SUCCESS)
            {
               am_result = AM_DMX_StartFilter(path, pid_filter->fhandle);
               if (am_result == AM_SUCCESS)
#endif
               {
                  pid_filter->started = TRUE;
                  demux_status[path].num_pid_filters_started++;
               }
               else
               {
                  /* Failed to start filter so clear the callback */
#ifdef USE_TSPLAYER
                  AML_DMX_SetCallback(path, pid_filter->fhandle, NULL, NULL);
#else
                  AM_DMX_SetCallback(path, pid_filter->fhandle, NULL, NULL);
#endif
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

      STB_OSMutexUnlock(demux_status[path].config_mutex);
   }

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
#ifdef USE_TSPLAYER
   BOOLEAN am_result;
#else
   AM_ErrorCode_t am_result;
#endif
   FUNCTION_START(STB_DMXStopPIDFilter);

   if (path < num_paths)
   {
      filter_index = (pfilt_id >> 8) & 0xff;
      i = (pfilt_id >> 4) & 0xf;

      pid_filter = &demux_status[path].filter_info[filter_index];

      STB_OSMutexLock(demux_status[path].config_mutex);

      if (pid_filter->started)
      {
#ifdef FILTER_PRINTS
printf(">> %s(%u, 0x%04x): start_count=%u", __FUNCTION__, path, pfilt_id, pid_filter->start_count[i]);
#endif
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
            /* Stop the filter and clear the callback */
#ifdef USE_TSPLAYER
            am_result = AML_DMX_StopFilter(path, pid_filter->fhandle);
            if (!am_result)
            {
               DMX_ERR("%u: Failed to stop PID filter 0x%04x on PID %u, error %d", path,
                  pfilt_id, pid_filter->pid, am_result);
            }

            AML_DMX_SetCallback(path, pid_filter->fhandle, NULL, NULL);
            AML_DMX_FreeFilter(path, pid_filter->fhandle);
#else
            am_result = AM_DMX_StopFilter(path, pid_filter->fhandle);
            if (am_result != AM_SUCCESS)
            {
               DMX_ERR("%u: Failed to stop PID filter 0x%04x on PID %u, error %d", path,
                  pfilt_id, pid_filter->pid, am_result);
            }

            AM_DMX_SetCallback(path, pid_filter->fhandle, NULL, NULL);
            AM_DMX_FreeFilter(path, pid_filter->fhandle);
#endif
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
 * @brief   Returns the maximum number of section filters available on this hw
 * @return  The number of filters
 */
static U8BIT inline _GetDmxDMASourceById(int id)
{
   U8BIT source = DVB_DEMUX_SOURCE_DMA0;
   switch (id)
   {
      case 0:
         source = DVB_DEMUX_SOURCE_DMA0;
         break;
      case 1:
         source = DVB_DEMUX_SOURCE_DMA1;
         break;
      case 2:
         source = DVB_DEMUX_SOURCE_DMA2;
         break;
      case 3:
         source = DVB_DEMUX_SOURCE_DMA3;
         break;
      case 4:
         source = DVB_DEMUX_SOURCE_DMA4;
         break;
      case 5:
         source = DVB_DEMUX_SOURCE_DMA5;
         break;
      case 6:
         source = DVB_DEMUX_SOURCE_DMA6;
         break;
      case 7:
         source = DVB_DEMUX_SOURCE_DMA7;
         break;
      default:
         break;
   }
   DMX_ERR("path:%d source:%d", id, source);
   return source;
}

/**
 * @brief   Configures the source of the demux
 * @param   path the demux path to configure
 * @param   source the source to use
 * @param   param source specific parameters (e.g. tuner number)
 */
void STB_DMXSetDemuxSource(U8BIT path, E_STB_DMX_DEMUX_SOURCE source, U8BIT param)
{
   int tuner_index;
#ifdef USE_TSPLAYER
   int ret;
   BOOLEAN am_result;
   DVB_DemuxSource_t dmx_src_cfg, dmx_src_cur;
#else
   AM_ErrorCode_t am_result;
#endif

   FUNCTION_START(STB_DMXSetDemuxSource);

   if (path >= num_paths)
   {
       DMX_ERR("path:%d error", path);
       return;
   }
#ifdef USE_TSPLAYER
   dmx_src_cfg = GetDemuxSourceByCfg(aml_hw_cfg.tuners[tuner_index].ts_input_idx);
   dvb_get_demux_source(path, &dmx_src_cur);
   DMX_DBG("Demux source [config:cur_node] = [%d:%d]", dmx_src_cfg, dmx_src_cur);
   if ((source != demux_status[path].source) || (param != demux_status[path].source_param) || (dmx_src_cfg != dmx_src_cur))
#else
   if ((source != demux_status[path].source) || (param != demux_status[path].source_param))
#endif
   {
      DMX_DBG("%u: new=%u, %u; old=%u, %u", path, source, param, demux_status[path].source, demux_status[path].source_param);
      demux_status[path].source = source;
      demux_status[path].source_param = param;

      if (source == DMX_TUNER)
      {
         tuner_index = param >= aml_hw_cfg.tuner_num ? aml_hw_cfg.tuner_num-1 : param;
#ifdef USE_TSPLAYER
         ret = dvb_set_demux_source(path, dmx_src_cfg);
         if (ret == -1)
#else
         am_result = AM_DMX_SetSource(path, aml_hw_cfg.tuners[tuner_index].ts_input_idx);
         if (am_result != AM_SUCCESS)
#endif
         {
            DMX_ERR("Failed to set demux %u source to %u, error %d", path, param, am_result);
         }
      }
      else if(source == DMX_MEMORY)
      {
         DMX_DBG("setting source to MEMORY");
#ifdef USE_TSPLAYER
         if (dmx_src_cur != _GetDmxDMASourceById(path))
         {
            ret = dvb_set_demux_source(path, _GetDmxDMASourceById(path));
            if (ret == -1)
            {
                DMX_ERR("Failed to set demux %u source to %u ", path, param);
            }
         }
#else
         am_result = AM_DMX_SetSource(path, AM_DMX_SRC_HIU);
         if (am_result != AM_SUCCESS)
         {
             DMX_ERR("Failed to set demux %u source to %u, error %d", path, param, am_result);
         }
#endif
      }
   }

   FUNCTION_FINISH(STB_DMXSetDemuxSource);
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


/**
 * @brief   change the source of the demux when cam card plug or unplug
 *          we need check "is_set_tssource" is 0 or not,if it value is 0,
 *          we do nothing now.
 * @param   slot  cam card slot
 * @param   plug 0:cam card unplug, 1：camc card plug
 */
void STB_DMXChangeAllDemuxSource(U8BIT slot, U8BIT plug)
{
   int i = 0;
   int tuner_index = 0;
   FUNCTION_START(STB_DMXChangeAllDemuxSource);
   //no used now, only one cam card
   slot = 0;
   if (aml_hw_cfg.cam[slot].is_set_tssource == 0) {
      //not set source at cfg file,so we return now,
      return;
   }

   for (i = 0; i < aml_hw_cfg.tuner_num; i++) {
      if (plug == 0) {
         //cam card is unplug.used camUnplug_tssource to
         //set ts_input_idx for dmx source
         aml_hw_cfg.tuners[i].ts_input_idx = aml_hw_cfg.cam[slot].camUnplug_tssource;
          DMX_DBG("index[%d]unplug[%d]", i, aml_hw_cfg.cam[slot].camUnplug_tssource);
      } else if(plug == 1) {
         //cam card is plug.used camPlug_tssource to
         //set ts_input_idx for dmx source
         aml_hw_cfg.tuners[i].ts_input_idx = aml_hw_cfg.cam[slot].camPlug_tssource;
         DMX_DBG("index[%d]plug[%d]", i, aml_hw_cfg.cam[slot].camPlug_tssource);
      }
   }
   for (i = 0; i < num_paths; i++) {
      //change ts_input_idx
      STB_DMXSetDemuxSource(i, DMX_TUNER, tuner_index);
   }
   FUNCTION_FINISH(STB_DMXChangeAllDemuxSource);
}

/**
 * @brief   set the tsout source when ts route is "tsin->tsout->tsin"
 * get ts out source from cfg
 */
static void STB_SetTsoutSource(void)
{
   FUNCTION_START(STB_SetTsoutSource);
   DMX_DBG("set demux source is set %d src %d", aml_hw_cfg.cam[0].is_set_tsout, aml_hw_cfg.cam[0].tsout_source);
   if (aml_hw_cfg.cam[0].is_set_tsout)
   {
      char buf[32];
      char *cmd;
      int src = aml_hw_cfg.cam[0].tsout_source;
      sprintf(buf, STB_TSO_SOURCE);

      switch (src)
      {
         case STB_TS_SOURCE0:
            cmd = "ts0";
         break;
         case STB_TS_SOURCE1:
            cmd = "ts1";
         break;
         case STB_TS_SOURCE2:
            cmd = "ts2";
         break;
         case STB_TS_SOURCE3:
            cmd = "ts3";
         break;
         default:
            DMX_DBG("do not support demux source %d", src);
         return;
      }
#ifdef USE_TSPLAYER
      dvr_file_echo(buf, cmd);
#else
      AM_FileEcho(buf, cmd);
#endif
      return;
   }

   FUNCTION_FINISH(STB_SetTsoutSource);
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

   AV_InjectData(path,data,size);

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
#if 1
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
#else
   DMX_DBG("path %u track %u", path, track);
   if (path < AM_DSC_SRC_BYPASS && track < DESC_NUM_TRACKS)
   {
      ret = AM_DSC_SetSource(DSC_DEV_NO, path);
      if (ret != AM_SUCCESS)
      {
         DMX_ERR("AM_DSC_SetSource error=%x", ret);
         result = FALSE;
      }
      else
      {
         demux_status[path].tracks[track].chanid = -2; /* prepare to allocate channel on next call to ApplyKey() */
         result = TRUE;
      }
   }
   else
   {
      DMX_ERR("Invalid: path %u track %u", path, track);
      result = FALSE;
   }
#endif
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
#if 1
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
#else
   DMX_DBG("path %u track %u", path, track);
   if (path < AM_DSC_SRC_BYPASS && track < DESC_NUM_TRACKS)
   {
      pdmx = demux_status + path;
      ptrk = pdmx->tracks + track;
      if (ptrk->chanid == -2)
      {
         ptrk->chanid = -1;
         ptrk->iseven = FALSE;
         ptrk->isodd = FALSE;
         result = TRUE;
      }
      else
      {
         ret = AM_DSC_FreeChannel(DSC_DEV_NO, ptrk->chanid);
         if (ret != AM_SUCCESS)
         {
            DMX_ERR("AM_DSC_FreeChannel error=%x", ret);
            result = FALSE;
         }
         else
         {
            ptrk->chanid = -1;
            ptrk->iseven = FALSE;
            ptrk->isodd = FALSE;
            result = TRUE;
         }
      }
   }
   else
   {
      result = FALSE;
   }
#endif
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
#if 1
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
#else
   DMX_DBG("path %u track %u parity %u data %02x %02x", path, track, parity, data[0], data[1]);
   if (path < AM_DSC_SRC_BYPASS && track < DESC_NUM_TRACKS)
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
      ApplyKey(path, track);
      result = TRUE;
   }
   else
   {
      result = FALSE;
   }
#endif
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
#if 1
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
#else
   if (path < AM_DSC_SRC_BYPASS && track < DESC_NUM_TRACKS)
   {
      *key_usage = demux_status[path].tracks[track].usage;
   }
   else
   {
      result = FALSE;
   }
#endif
   FUNCTION_FINISH(STB_DMXGetKeyUsage);

   return result;
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
#if 1
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
#else
   DMX_DBG("path %u track %u key_usg %u", path, track, key_usage);
   if (path < AM_DSC_SRC_BYPASS && track < DESC_NUM_TRACKS)
   {
      demux_status[path].tracks[track].usage = key_usage;
   }
   else
   {
      result = FALSE;
   }
#endif
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
#if 1
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
#else
   if (path < AM_DSC_SRC_BYPASS && track < DESC_NUM_TRACKS)
   {
      *type = demux_status[path].tracks[track].type;
   }
   else
   {
      result = FALSE;
   }
#endif
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
#if 1
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
#else
   DMX_DBG("path %u track %u type %u", path, track, type);
   if (path < AM_DSC_SRC_BYPASS && track < DESC_NUM_TRACKS)
   {
      demux_status[path].tracks[track].type = type;
   }
   else
   {
      result = FALSE;
   }
#endif
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
BOOLEAN DMXGetDecodePIDs(U8BIT path, U16BIT *pcr_pid, U16BIT *video_pid, U16BIT *audio_pid,
   U16BIT *ad_pid)
{
   BOOLEAN retval;

   FUNCTION_START(DMXGetDecodePIDs);

   if (path < num_paths)
   {
      *pcr_pid = demux_status[path].pids[DMX_PCR];
      *video_pid = demux_status[path].pids[DMX_VIDEO];
      *audio_pid = demux_status[path].pids[DMX_AUDIO];
      *ad_pid = demux_status[path].pids[DMX_ADES];
   }
   else
   {
      retval = FALSE;
   }

   FUNCTION_FINISH(DMXGetDecodePIDs);

   return(retval);
}

/*---local function definitions----------------------------------------------*/

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
#if 1
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
#else
   AM_DSC_KeyType_t ktyp;

   pdmx = demux_status + path;
   ptrk = pdmx->tracks + track;

   if (ptrk->chanid == -2 && pdmx->pids[track] != 0)
   {
      ret = AM_DSC_AllocateChannel(DSC_DEV_NO, &desc_chan);
      if (ret != AM_SUCCESS)
      {
         DMX_ERR("AM_DSC_AllocateChannel error=%x", ret);
      }
      else
      {
         ptrk->chanid = desc_chan;
         ret = AM_DSC_SetChannelPID(DSC_DEV_NO, ptrk->chanid, pdmx->pids[track]);
         if (ret != AM_SUCCESS)
         {
            DMX_ERR("AM_DSC_SetChannelPID error=%x", ret);
         }
         else
         {
            if (ptrk->iseven)
            {
               switch (ptrk->type)
               {
                  case DESC_TYPE_DVB:
                  {
                     ktyp = AM_DSC_KEY_TYPE_EVEN;
                     break;
                  }
                  case DESC_TYPE_AES:
                  {
                     /* CI+ AES */
                     ktyp = AM_DSC_KEY_TYPE_AES_EVEN;
                     break;
                  }
                  case DESC_TYPE_AES_SCTE_52:
                  {
                     /* MHEG ICS */
                     ktyp = AM_DSC_KEY_TYPE_AES_IV_EVEN;
                     break;
                  }
                  case DESC_TYPE_DES:
                  case DESC_TYPE_TDES:
                  {
                     /* Not supported ? */
                     ktyp = AM_DSC_KEY_TYPE_EVEN;
                     break;
                  }
               }
               ret = AM_DSC_SetKey(DSC_DEV_NO, ptrk->chanid, ktyp, ptrk->even);
               if (ret != AM_SUCCESS)
               {
                  DMX_ERR("AM_DSC_SetKey error=%x", ret);
               }
               else
               {
                  DMX_DBG("SetKey EVEN typ=%u pid=%u", ptrk->type, pdmx->pids[track]);
               }
            }

            if (ptrk->isodd)
            {
               switch (ptrk->type)
               {
                  case DESC_TYPE_DVB:
                  {
                     ktyp = AM_DSC_KEY_TYPE_ODD;
                     break;
                  }
                  case DESC_TYPE_AES:
                  {
                     /* CI+ AES */
                     ktyp = AM_DSC_KEY_TYPE_AES_ODD;
                     break;
                  }
                  case DESC_TYPE_AES_SCTE_52:
                  {
                     /* MHEG ICS */
                     ktyp = AM_DSC_KEY_TYPE_AES_IV_ODD;
                     break;
                  }
                  case DESC_TYPE_DES:
                  case DESC_TYPE_TDES:
                  {
                     /* Not supported ? */
                     ktyp = AM_DSC_KEY_TYPE_ODD;
                     break;
                  }
               }
               ret = AM_DSC_SetKey(DSC_DEV_NO, ptrk->chanid, ktyp, ptrk->odd);
               if (ret != AM_SUCCESS)
               {
                  DMX_ERR("AM_DSC_SetKey error=%x", ret);
               }
               else
               {
                  DMX_DBG("SetKey ODD typ=%u pid=%u", ptrk->type, pdmx->pids[track]);
               }
            }
         }
      }
   }
#endif
}

/**
 * @brief   Clear descrambler keys
 * @param   param - demux path
 */
static void ClearKey(U8BIT path, E_STB_DMX_DESC_TRACK track)
{
   BOOLEAN ret;
#if 1
	  USE_UNWANTED_PARAM(path);
	  USE_UNWANTED_PARAM(track);
#else

   if (demux_status[path].tracks[track].chanid >= 0)
   {
      ret = AM_DSC_FreeChannel(DSC_DEV_NO, demux_status[path].tracks[track].chanid);
      if (ret != AM_SUCCESS)
      {
         DMX_ERR("AM_DSC_FreeChannel error=%x", ret);
      }
      else
      {
         DMX_DBG("[%u] trk=%u", path, track);
         demux_status[path].tracks[track].chanid = -2;
      }
   }
#endif
}

/**
 * @brief   Callback function that receives data for PID and section filters
 */
static void PidCallback(int dev_no, int fhandle, const uint8_t *data, int len, void *user_data)
{
   S_PID_FILTER_INFO *pid_filter;
   U8BIT i, j;
   S_SECTION_FILTER_INFO *sect_filter;
   FILTER_CALLBACK func_ptr;
   U8BIT result;
   U16BIT sfi;

   FUNCTION_START(PidCallback);

   if ((data != NULL) && (len != 0) && (user_data != NULL))
   {
      pid_filter = (S_PID_FILTER_INFO *)user_data;

      if (pid_filter->fhandle == fhandle)
      {
         pid_filter->data_packet = data;
         pid_filter->data_packet_size = len;

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
                        (*func_ptr)(dev_no, (U16BIT)len, ((pid_filter->index << 8) + (j << 4)));
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
                  result = sect_filter->mask[0] & (data[0] ^ sect_filter->match[0]);

                  /* Different tables can be on the same PID, so if result doesn't equal
                   * 0 then this data is for a different table id */
                  if (result == 0)
                  {
                     for (sfi = 1; sfi < DEMUX_SECTION_FILTER_LENGTH; ++sfi)
                     {
                        /* Skip section length field */
                        result |= (sect_filter->mask[sfi] & (data[sfi+2] ^ sect_filter->match[sfi]));
                     }

                     if (result == 0)
                     {
                        for (j = 0; j < MAX_FILTERS_PER_PID; j++)
                        {
                           if (pid_filter->func_ptr[j] != NULL)
                           {
                              func_ptr = pid_filter->func_ptr[j];
                              (*func_ptr)(dev_no, (U16BIT)len, ((pid_filter->index << 8) + (j << 4)));
                           }
                        }
                     }
                     else
                     {
                        const U8BIT*p = data;
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
            dev_no, fhandle, pid_filter->fhandle);
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
#ifdef USE_TSPLAYER
   BOOLEAN am_result;
#else
   AM_ErrorCode_t am_result;
#endif
   FUNCTION_START(UpdateSectionFilter);

   pid_filter = &demux_status[path].filter_info[filter_index];
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
      if (pid_filter->started)
      {
         if (pid_filter->fhandle != -1)
         {
            /* Stop the filter while it's updated */
#ifdef USE_TSPLAYER
            am_result = AML_DMX_StopFilter(path, pid_filter->fhandle);
            if (!am_result)
#else
            am_result = AM_DMX_StopFilter(path, pid_filter->fhandle);
            if (am_result != AM_SUCCESS)
#endif
            {
               DMX_ERR("%u: Failed to stop PID filter %d, error %d", path, pid_filter->fhandle, am_result);
            }
         }
      }

      if (pid_filter->fhandle == -1)
      {
#ifdef USE_TSPLAYER
         am_result = AML_DMX_AllocateFilter(path, &pid_filter->fhandle);
         if (am_result)
         {
            am_result = AML_DMX_SetBufferSize(path, pid_filter->fhandle, 8 * MAX_SECTION_SIZE);
            if (!am_result)
#else
         am_result = AM_DMX_AllocateFilter(path, &pid_filter->fhandle);
         if (am_result == AM_SUCCESS)
         {
            am_result = AM_DMX_SetBufferSize(path, pid_filter->fhandle, 8 * MAX_SECTION_SIZE);
            if (am_result != AM_SUCCESS)
#endif
            {
               DMX_ERR("%u: Failed to set buffer size for section filter %u, error %d", path,
                  filter_index, am_result);
            }
         }
      }

      if (pid_filter->fhandle != -1)
      {
#ifdef USE_TSPLAYER
         am_result = AML_DMX_SetSecFilter(path, pid_filter->fhandle, &dvb_filt_p);
         if (am_result)
#else
         am_result = AM_DMX_SetSecFilter(path, pid_filter->fhandle, &dvb_filt_p);
         if (am_result == AM_SUCCESS)
#endif
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
#ifdef USE_TSPLAYER
            am_result = AML_DMX_StartFilter(path, pid_filter->fhandle);
            if (!am_result)
#else
            am_result = AM_DMX_StartFilter(path, pid_filter->fhandle);
            if (am_result != AM_SUCCESS)
#endif
            {
               DMX_ERR("%u: Failed to restart PID filter %d, error %d", path, pid_filter->fhandle, am_result);
            }
         }
      }
   }

   FUNCTION_FINISH(UpdateSectionFilter);

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

            if (num_bytes > len)
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

#ifdef USE_TSPLAYER
static DVB_DemuxSource_t GetDemuxSourceByCfg(U8BIT ts_input_idx)
{
   DVB_DemuxSource_t demux_source = DVB_DEMUX_SOURCE_TS0;
   switch (ts_input_idx)
   {
       case 0:
           demux_source = DVB_DEMUX_SOURCE_TS0;
           break;
       case 1:
           demux_source = DVB_DEMUX_SOURCE_TS1;
           break;
       case 2:
           demux_source = DVB_DEMUX_SOURCE_TS2;
           break;
       default:
           DMX_DBG("do not support demux source:ts%d", ts_input_idx);
       break;
   }
   return demux_source;
}
#endif
