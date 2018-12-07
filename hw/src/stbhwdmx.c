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

//#define DEMUX_DEBUG
//#define FILTER_PRINTS

/*---includes for this file---------------------------------------------------*/
/* compiler library header files */
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <error.h>
#include <errno.h>
#include <stdint.h>

/* third party header files */
#include <linux/dvb/dmx.h>

/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"

#include "stbhwdef.h"
#include "stbhwc.h"
#include "stbhwos.h"
#include "stbhwdmx.h"
#include "stbhwmem.h"


/*---constant definitions for this file--------------------------------------*/
#define DMX_ERR(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

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

#define PES_PACKET_SIZE             184
#define PES_BUFFER_SIZE             (8 * PES_PACKET_SIZE)
#define TEXT_BUFFER_SIZE            (3008 * 24)

/* Local ENUM/TYPE Definitions */
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
   U16BIT pid;
   int filter_fd;
   BOOLEAN started;
   U8BIT* data_packet;
   U16BIT data_packet_size;
   S_SECTION_FILTER_INFO section_filters[MAX_SECTION_FILTERS];
   FILTER_CALLBACK func_ptr[MAX_FILTERS_PER_PID];
   U8BIT start_count[MAX_FILTERS_PER_PID];
} S_PID_FILTER_INFO;

typedef struct
{
   U8BIT path;
   U16BIT caps;

   void *config_mutex;

   E_STB_DMX_DEMUX_SOURCE source;
   U8BIT source_param;

   U16BIT pcr_pid;
   U16BIT video_pid;
   U16BIT audio_pid;
   U16BIT ad_pid;

   uint64_t stc_value;
   U32BIT stc_time;

   /* Subtitle/teletext PES support vars */
   int text_fd;
   U16BIT text_pid;
   U8BIT* text_buffer;
   U8BIT* write_ptr;
   U8BIT* read_ptr;
   U32BIT text_bytes_available;
   void* text_mutex;
   BOOLEAN pes_task_running;
   void* start_pes_task;
   void* pes_task_stopped;

   S_PID_FILTER_INFO filter_info[MAX_PID_FILTERS];

   U8BIT num_pid_filters_started;
   void* dmx_task_start;
   void* dmx_task_stopped;
   volatile BOOLEAN dmx_task_running;
} S_DMX_STATUS;


/*---local (static) variable declarations for this file----------------------*/
static S_DMX_STATUS* demux_status;
static U8BIT num_paths;

static U8BIT* pes_data = NULL;
static U32BIT pes_data_size = 0;


/*---local function prototypes for this file---------------------------------*/
static void DMXTask(void *param);
static BOOLEAN UpdateSectionFilter(U8BIT path, U16BIT filter_index);

static void PesDataTask(void* param);

static void OpenSectionFilters(char *demux_name, S_DMX_STATUS *pdmx);
static void CloseSectionFilters(U8BIT path);


/*---global function definitions---------------------------------------------*/


/**
 * @brief   Initialises the demux / programmable transport interface
 * @param   paths Number of demux paths to be initialised
 * @param   inc_pes_collection Not used
 */
void STB_DMXInitialise(U8BIT paths, BOOLEAN inc_pes_collection)
{
   BOOLEAN adapter_found;
   char demux_name[32];
   struct stat file_status;
   U16BIT i;
   U16BIT j;

   FUNCTION_START(STB_DMXInitialise);
   USE_UNWANTED_PARAM(paths);
   USE_UNWANTED_PARAM(inc_pes_collection);

   /* Find out how many demuxes are available */
   for (num_paths = 0, adapter_found = TRUE; adapter_found; )
   {
      snprintf(demux_name, sizeof(demux_name), "/dev/dvb0.demux%u", num_paths);
      if (stat(demux_name, &file_status) == 0)
      {
         DMX_DBG("found %s", demux_name);
         num_paths++;
      }
      else
      {
         adapter_found = FALSE;
      }
   }

   if (num_paths != 0)
   {
      /* Allocate memory for the status structures (one per path) */
      demux_status = (S_DMX_STATUS*)STB_MEMGetSysRAM(sizeof(S_DMX_STATUS) * num_paths);

      /* Initialise the status for each path and section filter
       * and open any required demuxing handles
       */
      if (demux_status != NULL)
      {
         memset(demux_status, 0, sizeof(S_DMX_STATUS) * num_paths);

         for (i = 0; i < num_paths; i++)
         {
            demux_status[i].path = i;
            demux_status[i].config_mutex = STB_OSCreateMutex();

            /* All demuxes are capable of everything */
            demux_status[i].caps = DMX_CAPS_LIVE | DMX_CAPS_RECORDING | DMX_CAPS_PLAYBACK |
               DMX_CAPS_MONITOR_SI;

            demux_status[i].pcr_pid = 0;
            demux_status[i].video_pid = 0;
            demux_status[i].audio_pid = 0;
            demux_status[i].text_pid = 0;
            demux_status[i].ad_pid = 0;

            demux_status[i].stc_value = 0;
            demux_status[i].stc_time = 0;

            /* Set default values */
            for (j = 0; j < MAX_PID_FILTERS; j++)
            {
               memset(&demux_status[i].filter_info[j], 0, sizeof(demux_status[i].filter_info[j]));

               demux_status[i].filter_info[j].filter_fd = -1;
               demux_status[i].filter_info[j].started = FALSE;
               demux_status[i].filter_info[j].pid = DEMUX_PID_NOT_USED;
               demux_status[i].filter_info[j].data_packet = STB_MEMGetSysRAM(MAX_SECTION_SIZE);
               demux_status[i].filter_info[j].data_packet_size = 0;
            }

            /* Default sources for each path */
            demux_status[i].source = DMX_MEMORY;
            demux_status[i].source_param = 255;

            demux_status[i].text_buffer = STB_MEMGetSysRAM(TEXT_BUFFER_SIZE);
            demux_status[i].write_ptr = demux_status[i].text_buffer;
            demux_status[i].read_ptr = demux_status[i].text_buffer;
            demux_status[i].text_mutex = STB_OSCreateMutex();
            demux_status[i].start_pes_task = STB_OSCreateSemaphore();
            demux_status[i].pes_task_stopped = STB_OSCreateSemaphore();
            demux_status[i].text_bytes_available = 0;

            snprintf(demux_name, sizeof(demux_name), "/dev/dvb0.demux%u", i);

            /* Open a demux instance for the text (subtitle) PES */
            if ((demux_status[i].text_fd = open(demux_name, O_RDWR | O_NONBLOCK)) >= 0)
            {
               ioctl(demux_status[i].text_fd, DMX_SET_BUFFER_SIZE, TEXT_BUFFER_SIZE);
            }

            DMX_DBG("%u: Opened text PES filter, fd=%d", i, demux_status[i].text_fd);

            OpenSectionFilters(demux_name, &demux_status[i]);

            STB_OSSemaphoreWait(demux_status[i].start_pes_task);
            STB_OSSemaphoreWait(demux_status[i].pes_task_stopped);

            demux_status[i].pes_task_running = FALSE;

            STB_OSCreateTask(PesDataTask, (void*)&demux_status[i], DMX_TASK_STACK_SIZE,
               DMX_TASK_PRIORITY, (U8BIT*)"PesDataTask");

            demux_status[i].num_pid_filters_started = 0;

            demux_status[i].dmx_task_start = STB_OSCreateSemaphore();
            STB_OSSemaphoreWait(demux_status[i].dmx_task_start);

            demux_status[i].dmx_task_stopped = STB_OSCreateSemaphore();
            STB_OSSemaphoreWait(demux_status[i].dmx_task_stopped);

            demux_status[i].dmx_task_running = FALSE;

            STB_OSCreateTask(DMXTask, (void*)&demux_status[i], DMX_TASK_STACK_SIZE,
               DMX_TASK_PRIORITY, (U8BIT*)"DMXTask");

            /* Allow the DMX task to start running */
            STB_OSSemaphoreSignal(demux_status[i].dmx_task_start);
         }
      }
   }
   else
   {
      DMX_DBG("No demuxes found!");
   }

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

   if (path < num_paths)
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
   FUNCTION_START(STB_DMXChangeDecodePIDs);
   USE_UNWANTED_PARAM(data_pid);

   DMX_DBG("%u: pcr=%u, video=%u, audio=%u, text=%u, ad=%u", path, pcr_pid, video_pid, audio_pid,
      text_pid, ad_pid);

   if (path < num_paths)
   {
      demux_status[path].pcr_pid = pcr_pid;
      demux_status[path].video_pid = video_pid;
      demux_status[path].audio_pid = audio_pid;
      demux_status[path].ad_pid = ad_pid;

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
   struct dmx_pes_filter_params p;

   FUNCTION_START(STB_DMXChangeTextPID);

   if ((path < num_paths) && (demux_status[path].text_pid != text_pid))
   {
      demux_status[path].text_pid = text_pid;

      if (demux_status[path].text_fd >= 0)
      {
         if (demux_status[path].pes_task_running)
         {
            /* Signal the PES task to stop and wait for it */
            demux_status[path].pes_task_running = FALSE;
            STB_OSSemaphoreWait(demux_status[path].pes_task_stopped);
         }

         if ((text_pid == 0) || (text_pid == 0xffff))
         {
            /* Set invalid PID value */
            text_pid = INVALID_PID;
         }

         memset(&p, 0, sizeof(p));

#if 0
         if (path == PLAYBACK_PATH)
         {
            p.input = DMX_IN_DVR;
         }
         else
#endif
         {
            p.input = DMX_IN_FRONTEND;
         }

         p.output = DMX_OUT_TAP;
         p.pes_type = DMX_PES_SUBTITLE;
         p.pid = text_pid;

         if (ioctl(demux_status[path].text_fd, DMX_SET_PES_FILTER, &p) < 0)
         {
            DMX_ERR("%u: Failed to set PID %u, fd %u, errno %d",
               path, text_pid, demux_status[path].text_fd, errno);
         }
         else
         {
            /* PID has been changed or filter has been stopped so clear record of any
             * remaining text PES data */
            STB_OSMutexLock(demux_status[path].text_mutex);
            demux_status[path].write_ptr = demux_status[path].text_buffer;
            demux_status[path].read_ptr = demux_status[path].text_buffer;
            demux_status[path].text_bytes_available = 0;
            STB_OSMutexUnlock(demux_status[path].text_mutex);

            if (demux_status[path].text_pid != 0)
            {
               /* Can now restart PES collection and the PES task */
               if (ioctl(demux_status[path].text_fd, DMX_START, 0) == 0)
               {
                  STB_OSSemaphoreSignal(demux_status[path].start_pes_task);
               }
               else
               {
                  DMX_ERR("Failed to start text filter, errno %d", errno);
               }
            }
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
printf(">> %s(%u): pid=%u, fd=%d, func=%p, 0x%04x - NEW\n", __FUNCTION__, path, pid, filter_ptr->filter_fd, func_ptr, pfilt_id);
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
printf(">> %s(%u, 0x%04x): pid=%u, fd=%d - FREED\n", __FUNCTION__, path, pfilt_id, filter_ptr->pid,
filter_ptr->filter_fd);
#endif
         /* Mark the filter as no longer allocated */
         filter_ptr->pid = DEMUX_PID_NOT_USED;
         filter_ptr->started = FALSE;
      }
#ifdef FILTER_PRINTS
      else
      {
   printf(">> %s(%u, 0x%04x): pid=%u, fd=%d\n", __FUNCTION__, path, pfilt_id, filter_ptr->pid, filter_ptr->filter_fd);
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
         if (ioctl(pid_filter->filter_fd, DMX_START, 0) == 0)
         {
            pid_filter->started = TRUE;

            demux_status[path].num_pid_filters_started++;
         }
         else
         {
            DMX_ERR("%u: Failed to start PID filter 0x%04x, errno=%d", path, pfilt_id, errno);
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
            /* Stop the filter */
            if (ioctl(pid_filter->filter_fd, DMX_STOP, 0) < 0)
            {
               DMX_ERR("%u: Failed to stop PID filter 0x%04x on PID %u, errno 0x%x", path,
                  pfilt_id, pid_filter->pid, errno);
            }

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
 * @brief   Configures the source of the demux
 * @param   path the demux path to configure
 * @param   source the source to use
 * @param   param source specific parameters (e.g. tuner number)
 */
void STB_DMXSetDemuxSource(U8BIT path, E_STB_DMX_DEMUX_SOURCE source, U8BIT param)
{
   dmx_source_t dmx_source;
   char dmx_source_file[32];
   char *source_name;
   int tuner_index;
   char cmd[64];

   FUNCTION_START(STB_DMXSetDemuxSource);

   if ((path < num_paths) &&
      ((source != demux_status[path].source) || (param != demux_status[path].source_param)))
   {
      DMX_DBG("%u: new=%u, %u; old=%u, %u", path, source, param,
         demux_status[path].source, demux_status[path].source_param);

      demux_status[path].source = source;
      demux_status[path].source_param = param;

      if (source == DMX_TUNER)
      {
         dmx_source = DMX_SOURCE_FRONT0 + param;
         if (dmx_source < DMX_SOURCE_FRONT3)
         {
            snprintf(dmx_source_file, sizeof(dmx_source_file), "/sys/class/stb/demux%u_source", path);
            tuner_index = param >= aml_hw_cfg.tuner_num ? aml_hw_cfg.tuner_num-1 : param;
            switch (aml_hw_cfg.tuners[tuner_index].ts_input_idx)
            {
               case DMX_SOURCE_FRONT0:
                  source_name = "ts0";
                  break;
               case DMX_SOURCE_FRONT1:
                  source_name = "ts1";
                  break;
               case DMX_SOURCE_FRONT2:
                  source_name = "ts2";
                  break;
               default:
                  source_name = NULL;
                  break;
            }

            if (source_name != NULL)
            {
               snprintf(cmd, sizeof(cmd), "echo \"%s\">%s", source_name, dmx_source_file);
               system(cmd);
            }
         }
         else
         {
            DMX_ERR("Tuner source %u not supported", param);
         }
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

   if ((path < num_paths) && (demux_status[path].text_fd >= 0))
   {
      STB_OSMutexLock(demux_status[path].text_mutex);

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

      STB_OSMutexUnlock(demux_status[path].text_mutex);
   }

   FUNCTION_FINISH(STB_DMXReadTextPES);
}

#if 0
/**
 * @brief   Writes data to the demux from memory
 * @param   path the demux path to be written
 * @param   data the data to be written
 * @param   size the number of bytes to be written
 */
void STB_DMXWriteDemux(U8BIT path, U8BIT *data, U32BIT size)
{
   FUNCTION_START(STB_DMXWriteDemux);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(data);
   USE_UNWANTED_PARAM(size);
   FUNCTION_FINISH(STB_DMXWriteDemux);
}
#endif

/**
 * @brief   Acquires a descrambler for the specified track on this path
 * @param   path the demux path for which the descrambler is acquired
 * @param   track enum representing audio, video or subtitles PES
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXGetDescramblerKey(U8BIT path, E_STB_DMX_DESC_TRACK track)
{
   FUNCTION_START(STB_DMXGetDescramblerKey);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   FUNCTION_FINISH(STB_DMXGetDescramblerKey);
   return(FALSE);
}

/**
 * @brief   Frees the descrambler for the specified track on this path
 * @param   path the demux path for which the descrambler is freed
 * @param   track enum representing audio, video or subtitles PES
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXFreeDescramblerKey(U8BIT path, E_STB_DMX_DESC_TRACK track)
{
   FUNCTION_START(STB_DMXFreeDescramblerKey);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   FUNCTION_FINISH(STB_DMXFreeDescramblerKey);
   return(FALSE);
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
   FUNCTION_START(STB_DMXSetDescramblerKeyData);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   USE_UNWANTED_PARAM(parity);
   USE_UNWANTED_PARAM(data);
   FUNCTION_FINISH(STB_DMXSetDescramblerKeyData);
   return(FALSE);
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
   FUNCTION_START(STB_DMXGetKeyUsage);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   USE_UNWANTED_PARAM(key_usage);
   FUNCTION_FINISH(STB_DMXGetKeyUsage);
   return(FALSE);
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
   FUNCTION_START(STB_DMXSetKeyUsage);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   USE_UNWANTED_PARAM(key_usage);
   FUNCTION_FINISH(STB_DMXSetKeyUsage);
   return(FALSE);
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
   FUNCTION_START(STB_DMXGetDescramblerType);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   USE_UNWANTED_PARAM(type);
   FUNCTION_FINISH(STB_DMXGetDescramblerType);
   return(FALSE);
}

/**
 * @brief   Set the descrambler type for the specified track on this path
 * @param   path the demux path that the descrambler type refers to
 * @param   type descrambler type (DES, AES, etc...)
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXSetDescramblerType(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_DESC_TYPE type)
{
   FUNCTION_START(STB_DMXSetDescramblerType);
   USE_UNWANTED_PARAM(path);
   USE_UNWANTED_PARAM(track);
   USE_UNWANTED_PARAM(type);
   FUNCTION_FINISH(STB_DMXSetDescramblerType);
   return(FALSE);
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
      *pcr_pid = demux_status[path].pcr_pid;
      *video_pid = demux_status[path].video_pid;
      *audio_pid = demux_status[path].audio_pid;
      *ad_pid = demux_status[path].ad_pid;
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
 * @brief   Opens section filters on the given demux path
 * @param   demux_name name of Linux DVB demux device
 * @param   path demux
 */
static void OpenSectionFilters(char *demux_name, S_DMX_STATUS *pdmx)
{
   U8BIT j;
   int res;

   for (j = 0; j < MAX_PID_FILTERS; j++)
   {
      if ((pdmx->filter_info[j].filter_fd = open(demux_name, O_RDWR | O_NONBLOCK)) >= 0)
      {
         res = ioctl(pdmx->filter_info[j].filter_fd, DMX_SET_BUFFER_SIZE, 8 * MAX_SECTION_SIZE);
         if (res != 0)
         {
            DMX_ERR("%u: Failed to set buffer size for PID filter %u, errno %d", pdmx->path, j, errno);
         }
      }
      else
      {
         DMX_ERR("%u: Failed to open filter %u, %s, errno %d", pdmx->path, j, demux_name, errno);
      }
   }
}

/**
 * @brief   Close all open section filter handles on the given demux path
 * @param   path - demux
 */
static void CloseSectionFilters(U8BIT path)
{
   int res;
   U8BIT j;

   for (j = 0; j < MAX_PID_FILTERS; j++)
   {
      if (demux_status[path].filter_info[j].filter_fd >= 0)
      {
         res = close(demux_status[path].filter_info[j].filter_fd);
         demux_status[path].filter_info[j].filter_fd = -1;
         if (res != 0)
         {
            DMX_ERR("%u: Failed to close filter %u, errno %d", path, j, errno);
         }
      }
   }
}

/**
 * @brief   Task to monitor the section filters
 * @param   param - demux path
 */
static void DMXTask(void *param)
{
   U8BIT path;
   S_DMX_STATUS* pdmx;
   U32BIT filt_index;
   S32BIT nbytes;
   struct pollfd fds[MAX_PID_FILTERS];
   U8BIT result;
   U16BIT sfi;
   U8BIT i, j;
   S_PID_FILTER_INFO *pid_filter;
   S_SECTION_FILTER_INFO *sect_filter;
   FILTER_CALLBACK func_ptr;

   pdmx = (S_DMX_STATUS*)param;
   path = pdmx->path;

   while(TRUE)
   {
      /* Wait for the task to be started */
      STB_OSSemaphoreWait(pdmx->dmx_task_start);

      pdmx->dmx_task_running = TRUE;

      /* Setup the file descriptors to be polled */
      for (filt_index = 0; filt_index < MAX_PID_FILTERS; filt_index++)
      {
         fds[filt_index].fd = pdmx->filter_info[filt_index].filter_fd;
         fds[filt_index].events = POLLIN;
         fds[filt_index].revents = 0;
      }

      while (pdmx->dmx_task_running)
      {
         /* Clear the events that have been received */
         for (filt_index = 0; filt_index < MAX_PID_FILTERS; filt_index++)
         {
            fds[filt_index].revents = 0;
         }

         /* Check if any data is available */
         if (poll(fds, MAX_PID_FILTERS, -1) > 0)
         {
            /* Look for any filters that have data available to be read */
            for (filt_index = 0; filt_index < MAX_PID_FILTERS; filt_index++)
            {
               if (((fds[filt_index].revents & POLLIN) != 0) && pdmx->filter_info[filt_index].started)
               {
                  STB_OSMutexLock(pdmx->config_mutex);

                  pid_filter = &pdmx->filter_info[filt_index];

                  /* Read the data from the demux on the filter's file descriptor */
                  nbytes = read(pid_filter->filter_fd, (void*)pid_filter->data_packet,
                     (size_t)MAX_SECTION_SIZE);

                  if (nbytes > 0)
                  {
#if 0
U16BIT sect_len;
BOOLEAN data_used = FALSE;
if (pid_filter->pid != 18)
{
sect_len = (((pid_filter->data_packet[1] & 0x0f) << 8) | pid_filter->data_packet[2]) + 3;
if (nbytes != sect_len)
   printf("## DMXTask(%u): nbytes=%u, sect_len=%u\n", pid_filter->pid, (U16BIT)nbytes, sect_len);
else
   printf("DMXTask(%u): nbytes=%u\n", pid_filter->pid, (U16BIT)nbytes);
}
#endif
                     pid_filter->data_packet_size = nbytes;

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
                                    STB_OSMutexUnlock(pdmx->config_mutex);
                                    (*func_ptr)(path, (U16BIT)nbytes, ((filt_index << 8) + (j << 4)));
                                    STB_OSMutexLock(pdmx->config_mutex);
#if 0
data_used = TRUE;
#endif
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
                              result = sect_filter->mask[0] &
                                 (pid_filter->data_packet[0] ^ sect_filter->match[0]);

                              /* Different tables can be on the same PID, so if result doesn't equal
                               * 0 then this data is for a different table id */
                              if (result == 0)
                              {
                                 for (sfi = 1; sfi < DEMUX_SECTION_FILTER_LENGTH; ++sfi)
                                 {
                                    /* Skip section length field */
                                    result |= (sect_filter->mask[sfi] &
                                       (pid_filter->data_packet[sfi+2] ^ sect_filter->match[sfi]));
                                 }

                                 if (result == 0)
                                 {
                                    for (j = 0; j < MAX_FILTERS_PER_PID; j++)
                                    {
                                       /* No section filtering: Call the handler */
                                       if (pid_filter->func_ptr[j] != NULL)
                                       {
                                          func_ptr = pid_filter->func_ptr[j];
                                          STB_OSMutexUnlock(pdmx->config_mutex);
                                          (*func_ptr)(path, (U16BIT)nbytes, ((filt_index << 8) + (j << 4)));
                                          STB_OSMutexLock(pdmx->config_mutex);
#if 0
data_used = TRUE;
#endif
                                       }
                                    }
                                 }
                                 else
                                 {
                                    U8BIT*p = pid_filter->data_packet;
                                    printf("  corrupt?: 0x%02x%02x%02x%02x%02x%02x%02x%02x match=0x%02x, mask=0x%02x\n",
                                       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
                                       sect_filter->match[0], sect_filter->mask[0]);
                                 }
                              }
                           }
                        }
                     }

                     /* Ensure that the packet of data is no longer available */
                     pid_filter->data_packet_size = 0;
#if 0
if (!data_used)
   printf("## DMXTask(pid=%u): not used, tid=0x%02x\n", pid_filter->pid, pid_filter->data_packet[0]);
#endif
                  }
                  else
                  {
                     DMX_ERR("Error reading section data, nbytes=%ld", nbytes);
                  }

                  STB_OSMutexUnlock(pdmx->config_mutex);
               }
            }
         }
      }

      STB_OSSemaphoreSignal(pdmx->dmx_task_stopped);
   }
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
         /* Stop the filter while it's updated */
         if (ioctl(pid_filter->filter_fd, DMX_STOP, 0) != 0)
         {
            DMX_ERR("%u: Failed to stop PID filter, errno 0x%x", path, errno);
         }
      }

      if (ioctl(pid_filter->filter_fd, DMX_SET_FILTER, &dvb_filt_p) >= 0)
      {
         success = TRUE;
      }
      else
      {
         DMX_ERR("%u: Failed to setup section filter, errno 0x%x", path, errno);
      }

      if (pid_filter->started)
      {
         /* Restart the filter */
         if (ioctl(pid_filter->filter_fd, DMX_START, 0) != 0)
         {
            DMX_ERR("%u: Failed to restart PID filter, errno 0x%x", path, errno);
         }
      }
   }

   FUNCTION_FINISH(UpdateSectionFilter);

   return success;
}

/**
 * @brief   Background task to collect PES data, which arrives as TP packets of 192 bytes
 *          and so needs to be gathered so that whole PES packets can be given to the middleware.
 * @param   param - pointer to demux status
 */
static void PesDataTask(void* param)
{
   S_DMX_STATUS* pdmx;
   U8BIT* pes_buffer;
   int bytes_read, num_bytes;
   U8BIT* end_ptr;
   struct pollfd fds[1];

   FUNCTION_START(PesDataTask);

   pdmx = (S_DMX_STATUS *)param;

   end_ptr = pdmx->text_buffer + TEXT_BUFFER_SIZE;

   pes_buffer = (U8BIT*)STB_MEMGetSysRAM(PES_BUFFER_SIZE);
   if (pes_buffer != NULL)
   {
      while(TRUE)
      {
         STB_OSSemaphoreWait(pdmx->start_pes_task);

         pdmx->pes_task_running = TRUE;

         fds[0].fd = pdmx->text_fd;
         fds[0].events = POLLIN;

         while (pdmx->pes_task_running)
         {
            fds[0].revents = 0;

            if ((poll(fds, 1, 10) > 0) && ((fds[0].revents & POLLIN) != 0))
            {
               /* Read the next PES TP packet(s) */
               bytes_read = read(fds[0].fd, pes_buffer, PES_BUFFER_SIZE);

               if (bytes_read > 0)
               {
                  STB_OSMutexLock(pdmx->text_mutex);

                  /* Copy the data to the PES buffer */
                  num_bytes = end_ptr - pdmx->write_ptr;

                  if (pdmx->text_bytes_available + bytes_read <= TEXT_BUFFER_SIZE)
                  {
                     if (num_bytes > bytes_read)
                     {
                        /* There's room for all the data */
                        memcpy(pdmx->write_ptr, pes_buffer, bytes_read);
                        pdmx->write_ptr += bytes_read;
                     }
                     else
                     {
                        /* Wrap round to write all the data */
                        memcpy(pdmx->write_ptr, pes_buffer, num_bytes);
                        memcpy(pdmx->text_buffer, pes_buffer + num_bytes,
                           bytes_read - num_bytes);
                        pdmx->write_ptr = pdmx->text_buffer +
                           (bytes_read - num_bytes);
                     }

                     pdmx->text_bytes_available += bytes_read;
                  }
                  else
                  {
                     DMX_ERR("Buffer is full!");
                  }

                  STB_OSMutexUnlock(pdmx->text_mutex);
               }
            }
         }

         STB_OSSemaphoreSignal(pdmx->pes_task_stopped);
      }

      STB_MEMFreeSysRAM(pes_buffer);
   }

   FUNCTION_FINISH(PesDataTask);
}

