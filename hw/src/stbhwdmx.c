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
#include "linuxdvbdmx_wrapper.h"
#include "stbhwini.h"
#include "stb_utils.h"
#include "stbhwdemux_usb.h"
#include <Aml_MP/Aml_MP.h>

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
#define MAX_SECTION_FILTERS         16
#define MAX_FILTERS_PER_PID         8
#define MAX_TEMI_FILTERS            2

#define DEMUX_FILTER_NOT_ALLOCATED  0xFFFF
#define DEMUX_PID_NOT_USED          0xFFFF
#define INVALID_PID                 0x1FFF

#define TEXT_BUFFER_SIZE            (65 * 1024)

#define DSC_DEV_NO                  0
#define DSC_CHAN_NUM                8
#define MAX_DSC_DEV                 3
#define MAX_SC2_DSC_DEV             32

#define STB_TSO_SOURCE "/sys/class/stb/tso_source"
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

/**Demux input source.*/
typedef enum
{
   DVB_DEMUX_SOURCE_TS0,  /**< Hardware TS input port 0.*/
   DVB_DEMUX_SOURCE_TS1,  /**< Hardware TS input port 1.*/
   DVB_DEMUX_SOURCE_TS2,  /**< Hardware TS input port 2.*/
   DVB_DEMUX_SOURCE_TS3,  /**< Hardware TS input port 3.*/
   DVB_DEMUX_SOURCE_TS4,  /**< Hardware TS input port 4.*/
   DVB_DEMUX_SOURCE_TS5,  /**< Hardware TS input port 5.*/
   DVB_DEMUX_SOURCE_TS6,  /**< Hardware TS input port 6.*/
   DVB_DEMUX_SOURCE_TS7,  /**< Hardware TS input port 7.*/
   DVB_DEMUX_SOURCE_DMA0, /**< DMA input port 0.*/
   DVB_DEMUX_SOURCE_DMA1, /**< DMA input port 1.*/
   DVB_DEMUX_SOURCE_DMA2, /**< DMA input port 2.*/
   DVB_DEMUX_SOURCE_DMA3, /**< DMA input port 3.*/
   DVB_DEMUX_SOURCE_DMA4, /**< DMA input port 4.*/
   DVB_DEMUX_SOURCE_DMA5, /**< DMA input port 5.*/
   DVB_DEMUX_SOURCE_DMA6, /**< DMA input port 6.*/
   DVB_DEMUX_SOURCE_DMA7,  /**< DMA input port 7.*/
   DVB_DEMUX_SECSOURCE_DMA0, /**< DMA secure port 0.*/
   DVB_DEMUX_SECSOURCE_DMA1, /**< DMA secure port 1.*/
   DVB_DEMUX_SECSOURCE_DMA2, /**< DMA secure port 2.*/
   DVB_DEMUX_SECSOURCE_DMA3, /**< DMA secure port 3.*/
   DVB_DEMUX_SECSOURCE_DMA4, /**< DMA secure port 4.*/
   DVB_DEMUX_SECSOURCE_DMA5, /**< DMA secure port 5.*/
   DVB_DEMUX_SECSOURCE_DMA6, /**< DMA secure port 6.*/
   DVB_DEMUX_SECSOURCE_DMA7,  /**< DMA secure port 7.*/
   DVB_DEMUX_SOURCE_DMA0_1,  /**< DMA input port 0_1.*/
   DVB_DEMUX_SOURCE_DMA1_1,   /**< DMA input port 1_1.*/
   DVB_DEMUX_SOURCE_DMA2_1,  /**< DMA input port 2_1.*/
   DVB_DEMUX_SOURCE_DMA3_1,   /**< DMA input port 3_1.*/
   DVB_DEMUX_SOURCE_DMA4_1,  /**< DMA input port 4_1.*/
   DVB_DEMUX_SOURCE_DMA5_1,   /**< DMA input port 5_1.*/
   DVB_DEMUX_SOURCE_DMA6_1,  /**< DMA input port 6_1.*/
   DVB_DEMUX_SOURCE_DMA7_1,   /**< DMA input port 7_1.*/
   DVB_DEMUX_SECSOURCE_DMA0_1, /**< DMA secure port 0_1.*/
   DVB_DEMUX_SECSOURCE_DMA1_1, /**< DMA secure port 1_1.*/
   DVB_DEMUX_SECSOURCE_DMA2_1, /**< DMA secure port 2_1.*/
   DVB_DEMUX_SECSOURCE_DMA3_1, /**< DMA secure port 3_1.*/
   DVB_DEMUX_SECSOURCE_DMA4_1, /**< DMA secure port 4_1.*/
   DVB_DEMUX_SECSOURCE_DMA5_1, /**< DMA secure port 5_1.*/
   DVB_DEMUX_SECSOURCE_DMA6_1, /**< DMA secure port 6_1.*/
   DVB_DEMUX_SECSOURCE_DMA7_1,  /**< DMA secure port 7_1.*/
   DVB_DEMUX_SOURCE_TS0_1, /**< DMA secure port 0_1.*/
   DVB_DEMUX_SOURCE_TS1_1, /**< DMA secure port 1_1.*/
   DVB_DEMUX_SOURCE_TS2_1, /**< DMA secure port 2_1.*/
   DVB_DEMUX_SOURCE_TS3_1, /**< DMA secure port 3_1.*/
   DVB_DEMUX_SOURCE_TS4_1, /**< DMA secure port 4_1.*/
   DVB_DEMUX_SOURCE_TS5_1, /**< DMA secure port 5_1.*/
   DVB_DEMUX_SOURCE_TS6_1, /**< DMA secure port 6_1.*/
   DVB_DEMUX_SOURCE_TS7_1, /**< DMA secure port 7_1.*/
} DVB_DemuxSource_t;

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
   /*for TS injection*/
   int dvr_fd;
   BOOLEAN inj_ready;
} S_DMX_STATUS;


/*---local (static) variable declarations for this file----------------------*/
static S_DMX_STATUS* demux_status;
static U8BIT num_paths;

static U8BIT* pes_data = NULL;
static U32BIT pes_data_size = 0;
static BOOLEAN support_tsd = TRUE;
static int ciplus_enable = 0;
static int g_max_dev_num;

/*---local function prototypes for this file---------------------------------*/
static BOOLEAN UpdateSectionFilter(U8BIT path, U16BIT filter_index);

static void PidCallback(int dev_no, int fhandle, const uint8_t *data, int len, void *user_data);
static void PesCallback(int dev_no, int fhandle, const uint8_t *data, int len, void *user_data);
static void ApplyKey(U8BIT path, E_STB_DMX_DESC_TRACK track);
static void ClearKey(U8BIT path, E_STB_DMX_DESC_TRACK track);
static void ResetDscChannel(U8BIT path, E_STB_DMX_DESC_TRACK track);
static DVB_DemuxSource_t GetDemuxSourceByCfg(U8BIT ts_input_idx);
static int DvbSetDemuxSource(int dmx_idx, DVB_DemuxSource_t src);
static int DvbGetDemuxSource(int dmx_idx, DVB_DemuxSource_t *src);
static int DvbEnableCIPlus(int enable);
static int CheckIfDmxIsNew(void);



/*Descrambler device information.*/
typedef struct {
   int path; /*Path number.*/
   int ref;  /*Reference count.*/
   int fd;   /*File descriptor.*/
   int pid[DSC_CHAN_NUM];  /*PID.*/
   int dmx_src;
} S_DSC_DEV_INFO;

#define SC2_DSC_CH_NUM 32
typedef struct s_sc2_dsc_dev_info
{
   int key_fd;
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
      int even_key_id;
      int odd_key_id;
      int iv_even_key_id;
      int iv_odd_key_id;
      int one_key_id;
      int iv_one_key_id;
      int dev_id;
   } dsc_pid_channel[SC2_DSC_CH_NUM];
} S_SC2_DSC_DEV_INFO;

static BOOLEAN                dmx_model_sc2 = FALSE;
static int                    sc2_key_fd    = -1;
static int                    sc2_key_ref   = 0;
static S_DSC_DEV_INFO         *dsc_dev_info  = NULL;
static S_SC2_DSC_DEV_INFO     *sc2_dsc_dev_info = NULL;
static int                    dsc_dev_num   = 0;

/*--- static function definitions---------------------------------------------*/
static void *sc2_find_dsc_channel_by_channel(E_STB_TS_SOURCE src, int chan_id)
{
   int i;
   struct s_sc2_dsc_channel *dsc_channel = NULL;

   DMX_DBG("src %d chan_id 0x%x", src, chan_id);
   if (!sc2_dsc_dev_info)
   {
      DMX_DBG("dsc channel not found.");
      return NULL;
   }
   for (i = 0; i < SC2_DSC_CH_NUM; i++)
   {
      dsc_channel = &sc2_dsc_dev_info->dsc_pid_channel[i];
      if (dsc_channel->ref > 0 &&
          dsc_channel->src == src &&
          dsc_channel->chan_id == chan_id)
      {
         return dsc_channel;
      }
   }
   return NULL;
}
static BOOLEAN Is_DDB_Filter(S_PID_FILTER_INFO * pidfilter)
{
    BOOLEAN result = FALSE;
    S_SECTION_FILTER_INFO *sect_filter = NULL;
    if (pidfilter && pidfilter->pid == OAD_DDB_PID)
    {
        for (int i = 0; i < MAX_SECTION_FILTERS; i++)
        {
           sect_filter = &pidfilter->section_filters[i];
           if (sect_filter && sect_filter->setup)
           {
              if (sect_filter->match[0]== OAD_DSI_DDB_MATCH)
              {
                    result = TRUE;
                    break;
              }
           }
        }
    }
    return result;
}

static void *sc2_find_dsc_channel_by_pid(E_STB_TS_SOURCE src, int pid, E_STB_DSC_CA_TYPE dsc_type, int dev_id)
{
   int i;
   struct s_sc2_dsc_channel *dsc_channel = NULL;

   DMX_DBG("src %d pid 0x%x dsc_type %d", src, pid, dsc_type);
   if (!sc2_dsc_dev_info)
   {
      DMX_DBG("dsc channel not found.");
      return NULL;
   }
   for (i = 0; i < SC2_DSC_CH_NUM; i++)
   {
      dsc_channel = &sc2_dsc_dev_info->dsc_pid_channel[i];
      if (dsc_channel->ref > 0 &&
          dsc_channel->src == src &&
          dsc_channel->pid == pid &&
          dsc_channel->dsc_type == dsc_type &&
          dsc_channel->dev_id == dev_id)
      {
         DMX_DBG("found channel");
         return dsc_channel;
      }
   }
   return NULL;
}

static int key_open(void)
{
   char buf[32];
   int s_fd = -1;

   snprintf(buf, sizeof(buf), "/dev/key");
   s_fd = open(buf, O_RDWR);
   if (s_fd == -1)
   {
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
      DMX_DBG("key_alloc index:%d\n", param.key_index);
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

static int ca_set_scb(int dev_id, int index, int scb_flag)
{
   int ret = 0;
   int fd = 0;
   struct ca_sc2_descr_ex desc = {0};
   S_SC2_DSC_DEV_INFO *dsc = sc2_dsc_dev_info;

   desc.cmd = CA_SET_SCB;
   desc.params.scb_params.ca_index = index;
   desc.params.scb_params.ca_scb = scb_flag;
   desc.params.scb_params.ca_scb_as_is = 0;

   fd = dsc->dsc_fd[dev_id];
   ret = ioctl(fd, CA_SC2_SET_DESCR_EX, &desc);

   if (ret != 0)
   {
      DMX_DBG(" ca_set_scb ioctl fail, dev_id %d fd %d ret:0x%0x\n", dev_id, dsc->dsc_fd[dev_id], ret);
      return -1;
   }

   return 0;
}

static int ca_set_key(int dev_id, int index, int parity, unsigned int key_index)
{
   int ret = 0;
   int fd = 0;
   struct ca_sc2_descr_ex desc = {0};
   S_SC2_DSC_DEV_INFO *dsc = sc2_dsc_dev_info;

   DMX_DBG("ca_set_key dev:%d, index:%d, parity:%d, key_index:%d\n",
           dev_id, index, parity, key_index);

   desc.cmd = CA_KEY;
   desc.params.key_params.ca_index = index;
   desc.params.key_params.parity = parity;
   desc.params.key_params.key_index = key_index;

   if (dev_id >= g_max_dev_num)
   {
      return -1;
   }

   fd = dsc->dsc_fd[dev_id];
   ret = ioctl(fd, CA_SC2_SET_DESCR_EX, &desc);

   if (ret != 0)
   {
      DMX_DBG(" ca_set_key ioctl fail, dev_id %d fd %d ret:0x%0x\n", dev_id, dsc->dsc_fd[dev_id], ret);
      return -1;
   }

   DMX_DBG("ca_set_key, index:%d, parity:%d, key_index:%d\n", index, parity, key_index);
   return 0;
}

static void ca_dump_channel()
{
   int i;
   S_SC2_DSC_DEV_INFO *dsc = sc2_dsc_dev_info;
   struct s_sc2_dsc_channel *dsc_channel;

   for (i = 0; i < SC2_DSC_CH_NUM; i++)
   {
      dsc_channel = &sc2_dsc_dev_info->dsc_pid_channel[i];
      DMX_DBG("pid_info[%d] pid %d, chan_id %d, ref %d, dsc_type %d",
         i, dsc_channel->pid, dsc_channel->chan_id, dsc_channel->ref, dsc_channel->dsc_type);
   }
}

static void key_free (int key_fd, int key_id)
{
   DMX_DBG("dev_id %d key_id %d", key_fd, key_id);
   ioctl(key_fd, KEY_FREE, key_id);
}

/*---global function definitions---------------------------------------------*/

void STB_DMXDscSetSrc(int dev_id, int dmx_id)
{
   char dev_name[256];
   char dst_name[32];
   int  r;

   if (dmx_model_sc2)
      return;
   DMX_DBG("/sys/class/stb/dsc%d_source", dev_id);
   snprintf(dev_name, sizeof(dev_name), "/sys/class/stb/dsc%d_source", dev_id);
   snprintf(dst_name, sizeof(dst_name), "dmx%d", dmx_id);
   r = STB_File_Echo(dev_name, dst_name);

   if (r != 0)
      DMX_DBG("set %s source failed: %s", dev_name, strerror(errno));
#ifdef COMMON_INTERFACE
   DvbEnableCIPlus(TRUE);
#endif
}

int STB_DMXDscAlloc(int dev_id, int pid, E_STB_DMX_DESC_TYPE type, E_STB_DSC_CA_TYPE dsc_type)
{
   int chan_id = -1;
   int i, r, id;
   char name[256];

   DMX_DBG("dev %d pid %x dsc_type %d %s", dev_id, pid, type, name);

   if (dmx_model_sc2)
   {
      S_SC2_DSC_DEV_INFO *dsc = sc2_dsc_dev_info;
      struct ca_sc2_descr_ex desc = {0};
      enum ca_sc2_algo_type  algo;
      struct s_sc2_dsc_channel *dsc_channel = NULL;
      E_STB_TS_SOURCE ts_src;
      // Normall Check
      if (!dsc)
      {
         return -1;
      }

      /* if scb is not set, need protect pid 0 */
      if (pid == 0)
         return -1;

      STB_OSMutexLock(dsc->mutex);
      if (dsc->key_fd < 0)
         dsc->key_fd = key_open();

      if (dsc->dsc_ref[dev_id] == 0)
      {
         DMX_DBG("no reference, do init");

         if (dsc->dsc_fd[dev_id] <= 0)
         {
            /* only one dsc device actually */
            snprintf(name, sizeof(name), "/dev/dvb0.ca%d", dev_id);
            dsc->dsc_fd[dev_id] = open(name, O_RDWR);
            if (dsc->dsc_fd[dev_id] == -1)
            {
               DMX_DBG("open \"%s\" failed", name);
               STB_OSMutexUnlock(dsc->mutex);
               return -1;
            }
            DMX_DBG("dsc_fd %d, open success", dsc->dsc_fd[dev_id]);
         }
      }
      //Find if pid exists
      ts_src = STB_GetDmxTsSource(dev_id);
      dsc_channel = sc2_find_dsc_channel_by_pid(ts_src, pid, dsc_type, dev_id);

      if (dsc_channel)
      {
         DMX_DBG("found exist pid channel 0x%x type %d", pid, dsc_type);
         dsc_channel->ref++;
         dsc->dsc_ref[dev_id]++;
         STB_OSMutexUnlock(dsc->mutex);
         return dsc_channel->chan_id;
      }
      else
      {
         for (i = 0; i < SC2_DSC_CH_NUM;i++)
         {
            if (dsc->dsc_pid_channel[i].ref <= 0)
            {
               dsc_channel = &(dsc->dsc_pid_channel[i]);
               DMX_DBG("alloc new pid channel, [%d]", i);
               break;
            }
         }

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
            DMX_DBG("illegal descrambler type %d", type);
            STB_OSMutexUnlock(dsc->mutex);
            return -1;
         }

         desc.cmd = CA_ALLOC;
         desc.params.alloc_params.pid = pid;
         desc.params.alloc_params.algo = algo;
         desc.params.alloc_params.dsc_type = (enum ca_sc2_dsc_type)dsc_type;
         desc.params.alloc_params.ca_index = -1;

         DMX_DBG("type %d algo %d dsc_type %d", type, algo, dsc_type);
         r = ioctl(dsc->dsc_fd[dev_id], CA_SC2_SET_DESCR_EX, &desc);
         if (r < 0)
         {
            if (dsc_type == CA_DSC_TSD_TYPE)
            {
               DMX_DBG("CA_SC2_SET_DESCR_EX alloc channel failed, fd %d, try TSN", dsc->dsc_fd[dev_id]);
               desc.params.alloc_params.dsc_type = CA_DSC_COMMON_TYPE;
               r = ioctl(dsc->dsc_fd[dev_id], CA_SC2_SET_DESCR_EX, &desc);
               if (r < 0)
               {
                  DMX_DBG("CA_SC2_SET_DESCR_EX alloc channel failed, fd %d, byebye", dsc->dsc_fd[dev_id]);
                  STB_OSMutexUnlock(dsc->mutex);
                  return -1;
               }
               else
               {
                  DMX_DBG("alloc channel using TSN ok");
                  support_tsd = FALSE;
               }
            }
         }

         if (dsc_channel)
         {
            chan_id = desc.params.alloc_params.ca_index;
            dsc_channel->ref = 1;
            dsc_channel->chan_id = chan_id;
            dsc_channel->pid = pid;
            dsc_channel->src = ts_src;
            dsc_channel->dsc_type = dsc_type;
            dsc_channel->even_key_id = -1;
            dsc_channel->odd_key_id = -1;
            dsc_channel->iv_even_key_id = -1;
            dsc_channel->iv_odd_key_id = -1;
            dsc_channel->one_key_id = -1;
            dsc_channel->iv_one_key_id = -1;
            dsc_channel->dev_id = dev_id;
         dsc->dsc_ref[dev_id]++;
         }
      }
      STB_OSMutexUnlock(dsc->mutex);
      ca_dump_channel();
   }
   else
   {
      S_DSC_DEV_INFO *dsc = &dsc_dev_info[dev_id];
      DMX_DBG("dsc->fd  %d", dsc->fd);
      int id;
      if (dsc->fd == -1)
      {
         snprintf(name, sizeof(name), "/dev/dvb0.ca%d", dev_id);

         dsc->fd = open(name, O_RDWR);
         if (dsc->fd == -1)
         {
            DMX_DBG("open \"%s\" failed", name);
            return -1;
         }
         dsc->ref = 0;
      }

      for (id = 0; id < DSC_CHAN_NUM; id++)
      {
         DMX_DBG("dsc->pid id %d pid %d", id, dsc->pid[id]);
         if (dsc->pid[id] == -1)
         {
            struct ca_pid params;

            params.pid = pid;
            params.index = id;

            r = ioctl(dsc->fd, CA_SET_PID, &params);
            if (r < 0)
            {
               DMX_DBG("CA_SET_PID alloc channel failed");
               return -1;
            }
            else
               DMX_DBG("CA_SET_PID ok pid %d", pid);

            dsc->pid[id] = pid;
            dsc->ref++;
            chan_id = id;
            break;
         }
      }
   }

   return chan_id;
}

static void
dsc_set_aes_output(BOOLEAN enable)
{
   S_DSC_DEV_INFO *dsc;
   U8BIT r;
   int i;
   U32BIT flag = 0;
   U8BIT dev_name[256];
   U8BIT dst_name[32];
   U8BIT dmx_src[16];
   U8BIT target_source_str[8];
   U8BIT target_source;

   if (dmx_model_sc2)
      return;
   STB_GetCamSource(&target_source, NULL);
   snprintf(target_source_str, sizeof(target_source_str), "ts%d", target_source);
   if (enable)
   {
      for (i = 0; i < aml_hw_cfg.demux_num; i++)
      {
         snprintf(dev_name, sizeof(dev_name), "/sys/class/stb/demux%d_source", i);
         STB_File_Read(dev_name, dmx_src, sizeof(dmx_src));
         DMX_DBG("dmx.%d src %s target %s",i, dmx_src, target_source_str);
         if (strncmp(target_source_str, dmx_src, 3) == 0)
         {
            DMX_DBG("dmx source %d match ts1", i);
            flag |= 1 << i;
         }
      }
   }
   else
   {
      flag = 0;
   }
   DMX_DBG("ciplus flag %d", flag);
   snprintf(dev_name, sizeof(dev_name), "/sys/class/dmx/ciplus_output_ctrl");
   snprintf(dst_name, sizeof(dst_name), "%d", flag);
   r = STB_File_Echo(dev_name, dst_name);
   if (r != 0)
      DMX_DBG("set %s source failed", dev_name);
}

void STB_DMXDscFree(int dev_id, int chan_id)
{
   int i;
   int r;

   if (dmx_model_sc2)
   {
      S_SC2_DSC_DEV_INFO *dsc = sc2_dsc_dev_info;
      struct ca_sc2_descr_ex desc;
      struct s_sc2_dsc_channel* dsc_channel = NULL;
      E_STB_TS_SOURCE ts_src = STB_GetDmxTsSource(dev_id);

      STB_OSMutexLock(dsc->mutex);

      for (i = 0; i < SC2_DSC_CH_NUM; i++)
      {
         dsc_channel = &sc2_dsc_dev_info->dsc_pid_channel[i];
         if (dsc_channel->chan_id == chan_id &&
               dsc_channel->ref > 0 &&
               dsc_channel->src == ts_src)
         {
            dsc_channel->ref--;

            if (dsc_channel->ref > 0)
            {
               DMX_DBG("not freeing channel, ref now %d", dsc_channel->ref);
               STB_OSMutexUnlock(dsc->mutex);
               return;
            }
            else
            {
               DMX_DBG("freeing channel");
               desc.cmd = CA_FREE;
               desc.params.free_params.ca_index = chan_id;

               r = ioctl(dsc->dsc_fd[dev_id], CA_SC2_SET_DESCR_EX, &desc);
               if (r < 0)
                  DMX_DBG("CA_SC2_SET_DESCR_EX free channel failed");

               if (dsc_channel->even_key_id != -1)
                  key_free(dsc->key_fd, dsc_channel->even_key_id);
               if (dsc_channel->odd_key_id != -1)
                  key_free(dsc->key_fd, dsc_channel->odd_key_id);
               if (dsc_channel->iv_even_key_id != -1)
                  key_free(dsc->key_fd, dsc_channel->iv_even_key_id);
               if (dsc_channel->iv_odd_key_id != -1)
                  key_free(dsc->key_fd, dsc_channel->iv_odd_key_id);
               if (dsc_channel->one_key_id != -1)
                  key_free(dsc->key_fd, dsc_channel->one_key_id);
               if (dsc_channel->iv_one_key_id != -1)
                  key_free(dsc->key_fd, dsc_channel->iv_one_key_id);

               dsc_channel->even_key_id = -1;
               dsc_channel->odd_key_id = -1;
               dsc_channel->iv_even_key_id = -1;
               dsc_channel->iv_odd_key_id = -1;
               dsc_channel->one_key_id = -1;
               dsc_channel->iv_one_key_id = -1;

               dsc_channel->src = STB_TS_SOURCE_MAX;
               dsc_channel->dsc_type = -1;
               dsc_channel->pid = -1;
               dsc_channel->chan_id = -1;
               dsc_channel->ref = 0;
               dsc_channel->dev_id = -1;

               dsc->dsc_ref[dev_id]--;
            }
         }
      }
      if (dsc->dsc_ref[dev_id] == 0)
      {
         DMX_DBG("freeing dsc, fd %d", dsc->dsc_fd[dev_id]);
         if (dsc->dsc_fd[dev_id] > 0)
         {
            close(dsc->dsc_fd[dev_id]);
            dsc->dsc_fd[dev_id] = -1;
         }
      }
      STB_OSMutexUnlock(dsc->mutex);
      ca_dump_channel();
   }
   else
   {
      S_DSC_DEV_INFO *dsc = &dsc_dev_info[dev_id];
      struct ca_pid params;
      if ((dsc->fd == -1) || (chan_id == -1))
         return;

      params.pid = DEMUX_PID_NOT_USED;
      params.index = chan_id;

      r = ioctl(dsc->fd, CA_SET_PID, &params);
      if (r < 0)
         DMX_DBG("CA_SET_PID free channel failed");

      dsc->pid[chan_id] = -1;
      if (dsc->ref > 0)
      {
         dsc->ref--;
      }
      DMX_DBG("dsc->ref %d dev_id %d free_chan_id %d", dsc->ref, dev_id, chan_id);
      if ((dsc->ref == 0) && (dsc->fd != -1))
      {
         close(dsc->fd);
         dsc->fd = -1;
         dsc->dmx_src = -1;
      }
   }
}

int STB_DMXSetKey(int dev_id, int chan_id, E_STB_DMX_DESC_TYPE type, E_STB_DSC_CA_TYPE dsc_type, E_STB_DMX_DESC_KEY_PARITY parity, U8BIT *data)
{
   int r = 0;
   int i;
   char buffer[512] = {0};

   DMX_DBG("setkey: %x %x %x", data[0], data[1], data[2]);
   DMX_DBG("dev %d chan_id %d type %d parity %d dsc_type %d is_sc2 %d", dev_id, chan_id, type, parity, dsc_type, dmx_model_sc2);

   if (dev_id > g_max_dev_num || chan_id < 0)
   {
      DMX_DBG("param invalid, set key failed");
      return -1;
   }
   // memset(data, 1, 16);
   // memset(data+16, 2, 16);

   // for (i=0; i<32; i++)
      // data[i] = i;
   for (i = 0; i < 32; i++)
      sprintf(buffer + i * 3, "%02x ", data[i]);
   DMX_DBG("data: %s", buffer);

   if (dmx_model_sc2)
   {
      S_SC2_DSC_DEV_INFO *dsc = sc2_dsc_dev_info;
      E_STB_TS_SOURCE ts_src;
      E_KEY_ALGO_SC2 key_algo;
      struct s_sc2_dsc_channel *dsc_channel;
      int key_userid = 0;

      if (dsc_type == CA_DSC_TSD_TYPE)
      {
         if (support_tsd == FALSE)
         {
            dsc_type = DSC_COMMON_TYPE;
            DMX_DBG("Not support tsd, set ca key type change to common");
         }
      }

      STB_OSMutexLock(dsc->mutex);

      for (i = 0; i < DSC_CHAN_NUM; i++)
      {
         if (dsc->dsc_pid_channel[i].chan_id == chan_id)
            dsc_channel = &dsc->dsc_pid_channel[i];
      }
      ts_src = STB_GetDmxTsSource(dev_id);
      dsc_channel = sc2_find_dsc_channel_by_channel(ts_src, chan_id);

      if (!dsc_channel)
      {
         DMX_DBG("channel not found");
         STB_OSMutexUnlock(dsc->mutex);
         return 0;
      }

      if (dsc_channel->ref > 1)
      {
         DMX_DBG("channel already set, skip set_key.");
         STB_OSMutexUnlock(dsc->mutex);
         return 0;
      }

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
            key_algo = KEY_ALGO_AES;
            DMX_DBG("key type invalid");
            break;
      };
      E_CA_KEY_TYPE_SC2 key_type;
      E_CA_KEY_TYPE_SC2 iv_key_type;
      int *key_id;
      int *iv_key_id;

      switch (parity)
      {
      case KEY_PARITY_EVEN:
            key_type = CA_KEY_EVEN_TYPE;
            iv_key_type = CA_KEY_EVEN_IV_TYPE;
            key_id = &dsc_channel->even_key_id;
            iv_key_id = &dsc_channel->iv_even_key_id;
            break;
      case KEY_PARITY_ODD:
            key_type = CA_KEY_ODD_TYPE;
            iv_key_type = CA_KEY_ODD_IV_TYPE;
            key_id = &dsc_channel->odd_key_id;
            iv_key_id = &dsc_channel->iv_odd_key_id;
            break;
      case KEY_PARITY_NONE:
            key_type = CA_KEY_00_TYPE;
            iv_key_type = CA_KEY_00_IV_TYPE;
            key_id = &dsc_channel->one_key_id;
            iv_key_id = &dsc_channel->iv_one_key_id;
            break;
      }

      if (*key_id == -1)
      {
            *key_id = key_alloc(dsc->key_fd, FALSE);
            key_config(dsc->key_fd, *key_id, key_userid, key_algo, 0);
      }
      if (*iv_key_id == -1)
      {
            *iv_key_id = key_alloc(dsc->key_fd, TRUE);
            key_config(dsc->key_fd, *iv_key_id, key_userid, key_algo, 0);
      }
      /* set TSE scb */
      // if (dsc_type == CA_DSC_TSE_TYPE)
      // ca_set_scb(dev_id, chan_id, 2);
      /* set key */
      key_set(dsc->key_fd, *key_id, data, 16);
      ca_set_key(dev_id, chan_id, key_type, *key_id);
      /* set iv */
      key_set(dsc->key_fd, *iv_key_id, data + 16, 16);
      ca_set_key(dev_id, chan_id, iv_key_type, *iv_key_id);
      STB_OSMutexUnlock(dsc->mutex);
   }
   else
   {
      S_DSC_DEV_INFO *dsc = &dsc_dev_info[dev_id];
      struct ca_descr_ex desc;
      enum ca_cw_type cw_type;
      enum ca_cw_type cw_type_iv;
      enum ca_dsc_mode mode;

      if (dsc->fd == -1)
         return -1;

      switch (type)
      {
      case DESC_TYPE_DVB:
         cw_type = (parity == KEY_PARITY_EVEN) ? CA_CW_DVB_CSA_EVEN : CA_CW_DVB_CSA_ODD;
         mode = CA_DSC_ECB;
         break;
      case DESC_TYPE_AES:
         cw_type = (parity == KEY_PARITY_EVEN) ? CA_CW_AES_EVEN : CA_CW_AES_ODD;
         mode = CA_DSC_CBC;
         break;
      case DESC_TYPE_AES_SCTE_52:
         cw_type = (parity == KEY_PARITY_EVEN) ? CA_CW_AES_EVEN : CA_CW_AES_ODD;
         mode = CA_DSC_IDSA;
         break;
      case DESC_TYPE_DES:
         cw_type = (parity == KEY_PARITY_EVEN) ? CA_CW_DES_EVEN : CA_CW_DES_ODD;
         mode = CA_DSC_ECB;
         break;
      default:
         DMX_DBG("illegal descrambler type %d", type);
         return -1;
      }

      /*if (type == DESC_TYPE_DVB)
      {
         dsc_set_aes_output(FALSE);
      }
      else //aes & des need set this.
      {
         dsc_set_aes_output(TRUE);
      }*/

      if (mode == CA_DSC_CBC)
      {
         DMX_DBG("Set iv data");
         desc.index = chan_id;
         cw_type_iv = (parity == KEY_PARITY_EVEN) ? CA_CW_AES_EVEN_IV : CA_CW_AES_ODD_IV;
         desc.type = cw_type_iv;
         desc.mode = mode;
         desc.flags = 0;
         memcpy(desc.cw, data+16, 16);

         r = ioctl(dsc->fd, CA_SET_DESCR_EX, &desc);
         if (r < 0)
            DMX_DBG("CA_SET_DESCR_EX set iv key failed");
         else
            DMX_DBG("CA_SET_DESCR_EX set iv key success");
      }

      DMX_DBG("Set dsc data");
      desc.index = chan_id;
      desc.type = cw_type;
      desc.mode = mode;
      desc.flags = 0;
      memcpy(desc.cw, data, 16);

      r = ioctl(dsc->fd, CA_SET_DESCR_EX, &desc);
      if (r < 0)
         DMX_DBG("CA_SET_DESCR_EX set key failed");
      else
         DMX_DBG("CA_SET_DESCR_EX set key success");

   }
   /*if (type == DESC_TYPE_AES)
   {
      dsc_set_aes_output(TRUE);
   }
   else
   {
      dsc_set_aes_output(FALSE);
   }*/

   return r;
}

/**
 * @brief   Initialises the demux / programmable transport interface
 * @param   paths Number of demux paths to be initialised
 * @param   inc_pes_collection Not used
 */
void STB_DMXInitialise(U8BIT paths, BOOLEAN inc_pes_collection)
{
   BOOLEAN am_result = FALSE;
   S32BIT i;
   U16BIT j;

   char buf[128];
   char cmd[32];

   FUNCTION_START(STB_DMXInitialise);

   DMX_DBG("%u demuxes--, %s PES collection", paths, inc_pes_collection ? "with" : "no");
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
            am_result = DMX_Open(i);
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
                  demux_status[i].dvr_fd = -1;
                  demux_status[i].inj_ready = FALSE;
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

   /*Test is it the SC2 descrambler.*/
   {
      struct stat st;
      int         r;

      r = stat("/sys/class/stb/dsc_setting", &st);
      if (r == 0)
      {
         dmx_model_sc2 = TRUE;
         dsc_dev_num   = num_paths;
      }
      else
      {
         dmx_model_sc2 = FALSE;
         dsc_dev_num   = 2;
      }
      DMX_DBG("STB_DMXInitialise dsc_dev_num %d",dsc_dev_num);
      if (dmx_model_sc2)
      {
         g_max_dev_num = MAX_SC2_DSC_DEV;
         sc2_dsc_dev_info = (S_SC2_DSC_DEV_INFO *)STB_MEMGetSysRAM(sizeof(S_SC2_DSC_DEV_INFO));
         sc2_dsc_dev_info->key_fd = -1;
         sc2_dsc_dev_info->mutex = STB_OSCreateMutex();
         for (i = 0; i < MAX_SC2_DSC_DEV; i++)
         {
            sc2_dsc_dev_info->dsc_fd[i] = -1;
            sc2_dsc_dev_info->dsc_ref[i] = 0;
         }
         for (i = 0; i < SC2_DSC_CH_NUM; i++)
         {
            sc2_dsc_dev_info->dsc_pid_channel[i].src = STB_TS_SOURCE_MAX;
            sc2_dsc_dev_info->dsc_pid_channel[i].ref = 0;
            sc2_dsc_dev_info->dsc_pid_channel[i].pid = -1;
            sc2_dsc_dev_info->dsc_pid_channel[i].chan_id = -1;
            sc2_dsc_dev_info->dsc_pid_channel[i].dsc_type = -1;
            sc2_dsc_dev_info->dsc_pid_channel[i].dev_id = -1;
         }
      }
      else
      {
         dsc_dev_info = (S_DSC_DEV_INFO *)STB_MEMGetSysRAM(sizeof(S_DSC_DEV_INFO) * num_paths);
         g_max_dev_num = MAX_DSC_DEV;

         for (i = 0; i < dsc_dev_num; i++)
         {
            int c;

            for (c = 0; c < DSC_CHAN_NUM; c++)
            {
               dsc_dev_info[i].pid[c] = -1;
            }

            dsc_dev_info[i].path = i;
            dsc_dev_info[i].ref = 0;
            dsc_dev_info[i].fd = -1;
         }

         for (i = 0; i < dsc_dev_num; i++)
         {
            STB_DMXDscSetSrc(i, i);
            dsc_dev_info[i].dmx_src = i;
         }
      }
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
               ResetDscChannel(path, DESC_TRACK_AUDIO);
               ApplyKey(path, DESC_TRACK_AUDIO);
            }
            else
            {
               ClearKey(path, DESC_TRACK_AUDIO);
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
               ResetDscChannel(path, DESC_TRACK_VIDEO);
               ApplyKey(path, DESC_TRACK_VIDEO);
            }
            else
            {
               ClearKey(path, DESC_TRACK_VIDEO);
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
            if (!DMX_StopFilter(path, demux_status[path].text_fhandle))
            {
                DMX_ERR("Failed to stop text filter");
            }
            DMX_SetCallback(path, demux_status[path].text_fhandle, NULL, NULL);
            DMX_FreeFilter(path, demux_status[path].text_fhandle);
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
            ResetDscChannel(path, DESC_TRACK_TEXT);
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
   BOOLEAN am_result = TRUE;

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
            am_result = DMX_StopFilter(path, pid_filter->fhandle);
            if (!am_result)
            {
               DMX_ERR("%u: Failed to stop PID filter 0x%04x on PID %u, error %d", path,
                  pfilt_id, pid_filter->pid, am_result);
            }

            DMX_SetCallback(path, pid_filter->fhandle, NULL, NULL);
            DMX_FreeFilter(path, pid_filter->fhandle);
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
void STB_DMXSetDemuxSource(U8BIT path, E_STB_DMX_DEMUX_SOURCE source, U8BIT param, U16BIT demux_cap)
{
   int tuner_index;
   int ret;
   BOOLEAN am_result;
   DVB_DemuxSource_t dmx_src_cfg, dmx_src_cur;

   FUNCTION_START(STB_DMXSetDemuxSource);

   if (path >= num_paths)
   {
       DMX_ERR("path:%d error", path);
       return;
   }
   tuner_index = param >= aml_hw_cfg.tuner_num ? aml_hw_cfg.tuner_num-1 : param;

   dmx_src_cfg = GetDemuxSourceByCfg(aml_hw_cfg.tuners[tuner_index].ts_input_idx);

   if (dmx_model_sc2)
   {
      if (demux_cap == DMX_CAPS_USBCAM)
      {
         dmx_src_cfg = DVB_DEMUX_SOURCE_DMA0 + aml_hw_cfg.tuners[tuner_index].ts_input_idx;
         DMX_DBG("DMX_CAPS_USBCAM dmx_src_cfg=%d", dmx_src_cfg);
      }
      else if (demux_cap == DMX_CAPS_LIVE && source == DMX_TUNER)
      {
         dmx_src_cfg = DVB_DEMUX_SOURCE_TS0_1 + aml_hw_cfg.tuners[tuner_index].ts_input_idx;
         DMX_DBG("DMX_CAPS_LIVE dmx_src_cfg=%d", dmx_src_cfg);
      }
      else if (demux_cap == DMX_CAPS_RECORDING && source == DMX_TUNER)
      {
         dmx_src_cfg = DVB_DEMUX_SOURCE_TS0 + aml_hw_cfg.tuners[tuner_index].ts_input_idx;
         DMX_DBG("DMX_CAPS_Recording dmx_src_cfg=%d", dmx_src_cfg);
      }
   }

   DvbGetDemuxSource(path, &dmx_src_cur);
   DMX_DBG("path %d Demux source [config:cur_node] = [%d:%d]", path, dmx_src_cfg, dmx_src_cur);
   if ((source != demux_status[path].source) || (param != demux_status[path].source_param) || (dmx_src_cfg != dmx_src_cur))
   {
      DMX_DBG("%u: new=%u, %u; old=%u, %u", path, source, param, demux_status[path].source, demux_status[path].source_param);
      demux_status[path].source = source;
      demux_status[path].source_param = param;

      if (source == DMX_TUNER)
      {
         AV_StopInjection(path);
         ret = DvbSetDemuxSource(path, dmx_src_cfg);
         if (ret == -1)
         {
            DMX_ERR("Failed to set demux %u source to %u, error %d", path, param, ret);
         }
      }
      else if(source == DMX_MEMORY)
      {
         DMX_DBG("setting source to MEMORY");
         AV_StartInjection(path);
         if (dmx_src_cur != _GetDmxDMASourceById(path))
         {
            ret = DvbSetDemuxSource(path, _GetDmxDMASourceById(path));
            if (ret == -1)
            {
                DMX_ERR("Failed to set demux %u source to %u ", path, param);
            }
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
   int param = 0;

   if (aml_hw_cfg.cam[slot].is_set_tssource == 0)
   {
      //not set source at cfg file,so we return now,
      return;
   }

#ifdef COMMON_INTERFACE
   DvbEnableCIPlus(plug);
#endif

   for (i = 0; i < aml_hw_cfg.tuner_num; i++) {
      if (plug == 0)
      {
         // cam card is unplug.used ori_tsinput_idx to
         // set ts_input_idx for dmx source
         aml_hw_cfg.tuners[i].ts_input_idx = aml_hw_cfg.tuners[i].ori_tsinput_idx;
         DMX_DBG("index[%d]unplug[%d]", i, aml_hw_cfg.tuners[i].ori_tsinput_idx);
      }
      else if (plug == 1)
      {
         if (STB_CIUsbModuleInserted())
         {
            aml_hw_cfg.tuners[i].ts_input_idx = STB_CIUsbGetDmxSource(TRUE);
            DMX_DBG("index[%d]plug[%d]", i, STB_CIUsbGetDmxSource(TRUE));
         }
         else
         {
            // cam card is plug.used camPlug_tssource to
            // set ts_input_idx for dmx source
            aml_hw_cfg.tuners[i].ts_input_idx = aml_hw_cfg.cam[slot].camPlug_tssource;
            DMX_DBG("index[%d]plug[%d]", i, aml_hw_cfg.cam[slot].camPlug_tssource);
         }
      }
   }
   DMX_DBG("demux reset now");
   STB_File_Echo("/sys/class/stb/demux_reset", "1");
   for (i = 0; i < num_paths; i++)
   {
      // change ts_input_idx
      E_STB_DMX_DEMUX_SOURCE source;
      U8BIT param;
      STB_DMXGetDemuxSource(i, &source, &param);

      if (source == DMX_TUNER)
      {
         if (dmx_model_sc2)
         {
            if ((plug == 1) && (STB_CIUsbModuleInserted()))
               param = DMX_CAPS_USBCAM;

            STB_DMXSetDemuxSource(i, DMX_TUNER, tuner_index, param);
         }
         else
         {
            DMX_DBG("demux reset now");
            STB_File_Echo("/sys/class/stb/demux_reset", "1");
            STB_DMXSetDemuxSource(i, DMX_TUNER, tuner_index, param);
         }
      }
   }
   FUNCTION_FINISH(STB_DMXChangeAllDemuxSource);
}

/**
 * @brief   Reset the source of the demux
 * @param   path the demux path to configure
 */
void STB_DMXResetDemuxSource(U8BIT path)
{
    STB_DMXSetDemuxSource(path, DMX_TUNER, 0, 0);
}

#define TUNER_PATH 0
#define PAT_TIMEOUT (10000)
#define POLL_TIMEOUT (100)

static pthread_t ci_signal_thread;
static int       ci_signal_thread_run = 0;
static int       event_fd   = -1;
static int       demod_mode = 0;

#define DEMOD_NODE_NAME "/sys/class/dtvdemod/attr"
#define DEMOD_NODE_CMD0 "ci_mode 0"
#define DEMOD_NODE_CMD1 "ci_mode 1"

static void set_demod_mode (int mode)
{
   demod_mode = mode;
   DMX_DBG("ci monitor set demod mode to %d", mode);

   if (mode == 0) {
      DMX_DBG("echo %s > %s", DEMOD_NODE_CMD0, DEMOD_NODE_NAME);
      STB_File_Echo(DEMOD_NODE_NAME, DEMOD_NODE_CMD0);
   } else {
      DMX_DBG("echo %s > %s", DEMOD_NODE_CMD1, DEMOD_NODE_NAME);
      STB_File_Echo(DEMOD_NODE_NAME, DEMOD_NODE_CMD1);
   }
}

static void* ci_signal_entry (void *arg)
{
   struct dmx_sct_filter_params filter;
   struct pollfd fds[2];
   char buf[64];
   int  i, r;
   int  fd;
   int  has_signal = 0;
   int  timeout = 0;
   DMX_DBG("ci monitor wait lock");

   while (ci_signal_thread_run) {
      if (STB_TuneGetLockStatus(TUNER_PATH) == TUNER_STATE_LOCKED)
         break;

      fds[0].fd     = event_fd;
      fds[0].events = POLLIN|POLLERR;

      if (poll(fds, 1, 50) < 0)
      {
         DMX_DBG("poll failure: %s", strerror(errno));
         break;
      }
   }

   if (!ci_signal_thread_run)
      return NULL;

   DMX_DBG("ci monitor locked");

   if (STB_TuneGetActualSignalType(TUNER_PATH) != TUNE_SIGNAL_QAM) {
      return NULL;
   }

   for (i = 0; i < num_paths; i++) {
      E_STB_DMX_DEMUX_SOURCE source;
      U8BIT param;

      STB_DMXGetDemuxSource(i, &source, &param);
      if (source == DMX_TUNER)
         break;
   }

   DMX_DBG("ci monitor open demux %d", i);

   snprintf(buf, sizeof(buf), "/dev/dvb0.demux%d", i);

   fd = open(buf, O_RDWR);
   if (fd == -1) {
      DMX_DBG("cannot open demux %d", i);
      return NULL;
   }

   memset(&filter, 0, sizeof(filter));

   filter.pid = 0;
   filter.filter.filter[0] = 0;
   filter.filter.mask[0]   = 0xff;
   filter.flags |= DMX_CHECK_CRC;

   if (ioctl(fd, DMX_SET_FILTER, &filter) < 0)
   {
        DMX_DBG("set filter fail error:%s", strerror(errno));
        close(fd);
        return NULL;
   }

   if (ioctl(fd, DMX_START) < 0)
   {
       DMX_DBG("set START fail error:%s", strerror(errno));
       close(fd);
       return NULL;
   }

   fds[0].fd     = event_fd;
   fds[0].events = POLLIN|POLLERR;
   fds[1].fd     = fd;
   fds[1].events = POLLIN|POLLERR;

   while (ci_signal_thread_run) {
      //one time is 200ms
      r = poll(fds, 2, POLL_TIMEOUT);
      if (r > 1) {
         if (fds[1].revents & POLLIN) {
            has_signal = 1;
            DMX_DBG("ci monitor PAT got");
            break;
         }
      }
      timeout = timeout + POLL_TIMEOUT;
      if (timeout >= PAT_TIMEOUT) {
         break;
      }
   }

   close(fd);

   if (!ci_signal_thread_run)
      return NULL;

   if (!has_signal) {
      DMX_DBG("ci monitor PAT timeout");
      set_demod_mode(1);
   }

   return NULL;
}


/**
 * @brief set demod mode api.
 */
void STB_DMXCI_Set_Demod_Mode(int mode)
{
   DMX_DBG("ci set demod mode set[%d]old[%d]", mode, demod_mode);
   if (mode != demod_mode) {
      set_demod_mode(mode);
   }
}

/**
 * @brief Start the CI signal monitor.
 */
void STB_DMXCISignalMonitorStart()
{
   FUNCTION_START(STB_DMXCISignalMonitorStart);
   DMX_DBG("ci monitor start");
   if (!ci_signal_thread_run) {
      ci_signal_thread_run = 1;
      event_fd = eventfd(0, 0);
      pthread_create(&ci_signal_thread, NULL, ci_signal_entry, NULL);
   }

   FUNCTION_FINISH(STB_DMXCISignalMonitorStart);
}

/**
 * @brief Stop the CI signal monitor.
 */
void STB_DMXCISignalMonitorStop()
{
   FUNCTION_START(STB_DMXCISignalMonitorStop);
   DMX_DBG("ci monitor stop");
   if (ci_signal_thread_run) {
      int v = 0;

      ci_signal_thread_run = 0;
      write(event_fd, &v, sizeof(v));
      pthread_join(ci_signal_thread, NULL);
      close(event_fd);
      if (demod_mode)
         set_demod_mode(0);
   }

   FUNCTION_FINISH(STB_DMXCISignalMonitorStop);
}

E_STB_TS_SOURCE STB_GetDmxTsSource(int dmx_id)
{
   // Now we only support one ts source.
   return 0;
}

/**
 * @brief   set the tsout source when ts route is "tsin->tsout->tsin"
 * get ts out source from cfg
 */
void STB_SetTsoutSource(BOOLEAN is_cam_plugin)
{
   FUNCTION_START(STB_SetTsoutSource);
   if (aml_hw_cfg.cam[0].is_set_tsout)
   {
      char buf[32];
      char *cmd;
      int src = aml_hw_cfg.cam[0].tsout_source;
      sprintf(buf, STB_TSO_SOURCE);
      if (is_cam_plugin)
      {
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
      }
      else
         cmd = "close";
      DMX_DBG("set tsout: %s", cmd);
      STB_File_Echo(buf, cmd);
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
   int ret;
   U32BIT left = size;
   char name[32];

   FUNCTION_START(STB_DMXWriteDemux);
   if (demux_status[path].inj_ready == FALSE)
   {
      snprintf(name, sizeof(name), "/dev/dvb0.dvr%d", path);
      demux_status[path].dvr_fd = open(name, O_WRONLY);
      DMX_DBG("SETUP DVR DEV for MEMORY => %d!", demux_status[path].dvr_fd);
      // TODO: do we need this?
      //ret = ioctl(demux_status[path].dvr_fd, DMX_SET_INPUT, INPUT_LOCAL);
      //ret = ioctl(demux_status[path].dvr_fd, DMX_SET_BUFFER_SIZE, 5*1024*1024);
      DMX_DBG("set tsn_source to LOCAL");
      STB_File_Echo("/sys/class/stb/tsn_source", "local");
      demux_status[path].inj_ready = TRUE;
   }
   else if (data == NULL && size == 0)
   {
      DMX_DBG("DMX [%d] -> EOS -> cleanup!", path);
      if (demux_status[path].dvr_fd >= 0)
      {
         close(demux_status[path].dvr_fd);
         demux_status[path].dvr_fd = -1;
      }
      DMX_DBG("set tsn_source to DEMOD");
      STB_File_Echo("/sys/class/stb/tsn_source", "demod");
      demux_status[path].inj_ready = FALSE;
      return;
   }
   if (demux_status[path].dvr_fd == -1)
   {
      DMX_ERR("Cannot write to DMX [%d]", path);
      return;
   }
   if (data)
   {
      ret = write(demux_status[path].dvr_fd, data, left);
      left -= ret;
   }

   if (left || (size % 188))
   {
      DMX_ERR("Write to DMX [%d] %u -> %u", path, size, left);
   }

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
   int     dsc_dev;
   S_DSC_DEV_INFO *dsc;

   FUNCTION_START(STB_DMXGetDescramblerKey);

   if ((path < num_paths) && (track < DESC_NUM_TRACKS))
   {
      dsc_dev = dmx_model_sc2 ? path : DSC_DEV_NO;

      dsc = &dsc_dev_info[dsc_dev];

      ClearKey(path, track);

      STB_DMXDscSetSrc(dsc_dev, path);

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
   int     dsc_dev;
   S_DSC_DEV_INFO *dsc;

   FUNCTION_START(STB_DMXFreeDescramblerKey);

   DMX_DBG("path %u track %u", path, track);

   dsc_dev = dmx_model_sc2 ? path : DSC_DEV_NO;
   dsc     = &dsc_dev_info[dsc_dev];

   if ((path < num_paths) && (track < DESC_NUM_TRACKS))
   {
      pdmx = demux_status + path;
      ptrk = pdmx->tracks + track;
      ClearKey(path, track);
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
      ApplyKey(path, track);
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
   int dsc_dev;

   dsc_dev = dmx_model_sc2 ? path : DSC_DEV_NO;

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
            DMX_DBG("dsc alloc failed");
            return;
         }
      }
      if (ptrk->iseven)
         STB_DMXSetKey(dsc_dev, ptrk->chanid, ptrk->type, DSC_COMMON_TYPE, KEY_PARITY_EVEN, ptrk->even);
      if (ptrk->isodd)
         STB_DMXSetKey(dsc_dev, ptrk->chanid, ptrk->type, DSC_COMMON_TYPE, KEY_PARITY_ODD, ptrk->odd);
   }
}
static void ResetDscChannel(U8BIT path, E_STB_DMX_DESC_TRACK track)
{
   S_DMX_STATUS *pdmx;
   S_DES_TRACK_INFO *ptrk;
   int dsc_dev;

   pdmx = demux_status + path;
   ptrk = pdmx->tracks + track;

   dsc_dev = dmx_model_sc2 ? path : DSC_DEV_NO;

   if (ptrk->chanid != -1)
   {
      STB_DMXDscFree(dsc_dev, ptrk->chanid);
      ptrk->chanid = -1;
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
   int desc_chan;
   int dsc_dev;

   DMX_DBG("Clear key");

   dsc_dev = dmx_model_sc2 ? path : DSC_DEV_NO;

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
   BOOLEAN am_result;
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

static DVB_DemuxSource_t GetDemuxSourceByCfg(U8BIT ts_input_idx)
{
   DVB_DemuxSource_t demux_source = DVB_DEMUX_SOURCE_TS0;
   switch (ts_input_idx)
   {
   case 0:
      demux_source = DVB_DEMUX_SOURCE_TS0; /**< Hardware TS input port 0.*/
      break;
   case 1:
      demux_source = DVB_DEMUX_SOURCE_TS1; /**< Hardware TS input port 1.*/
      break;
   case 2:
      demux_source = DVB_DEMUX_SOURCE_TS2; /**< Hardware TS input port 2.*/
      break;
   case 3:
      demux_source = DVB_DEMUX_SOURCE_TS3; /**< Hardware TS input port 3.*/
      break;
   case 4:
      demux_source = DVB_DEMUX_SOURCE_TS4; /**< Hardware TS input port 4.*/
      break;
   case 5:
      demux_source = DVB_DEMUX_SOURCE_TS5; /**< Hardware TS input port 5.*/
      break;
   case 6:
      demux_source = DVB_DEMUX_SOURCE_TS6; /**< Hardware TS input port 6.*/
      break;
   case 7:
      demux_source = DVB_DEMUX_SOURCE_TS7; /**< Hardware TS input port 7.*/
      break;
   case 8:
      demux_source = DVB_DEMUX_SOURCE_DMA0; /**< DMA input port 0.*/
      break;
   case 9:
      demux_source = DVB_DEMUX_SOURCE_DMA1; /**< DMA input port 1.*/
      break;
   case 10:
      demux_source = DVB_DEMUX_SOURCE_DMA2; /**< DMA input port 2.*/
      break;
   case 11:
      demux_source = DVB_DEMUX_SOURCE_DMA3; /**< DMA input port 3.*/
      break;
   case 12:
      demux_source = DVB_DEMUX_SOURCE_DMA4; /**< DMA input port 4.*/
      break;
   case 13:
      demux_source = DVB_DEMUX_SOURCE_DMA5; /**< DMA input port 5.*/
      break;
   case 14:
      demux_source = DVB_DEMUX_SOURCE_DMA6; /**< DMA input port 6.*/
      break;
   case 15:
      demux_source = DVB_DEMUX_SOURCE_DMA7; /**< DMA input port 6.*/
      break;

   default:
      DMX_DBG("do not support demux source:ts%d", ts_input_idx);
      break;
   }
   return demux_source;
}

static int DvbSetDemuxSource(int dmx_idx, DVB_DemuxSource_t src)
{
    char node[32] = {0};
    char node2[20] = {0};
    int r = 0;

    snprintf(node, sizeof(node), "/sys/class/stb/demux%d_source", dmx_idx);
    snprintf(node2, sizeof(node2), "/dev/dvb0.demux%d", dmx_idx);

    int fd = open(node, O_RDONLY);
    if (fd == -1)
    {
        int source = 0;
        int input = 0;
        int fd2 = open(node2, O_WRONLY);
        if (fd2 != -1)
        {
            if (src <= DVB_DEMUX_SOURCE_TS7) {
                source = FRONTEND_TS0 + src - DVB_DEMUX_SOURCE_TS0;
                input = INPUT_DEMOD;
            } else if (src >= DVB_DEMUX_SOURCE_DMA0 &&
                src <= DVB_DEMUX_SOURCE_DMA7) {
                source = DMA_0 + src - DVB_DEMUX_SOURCE_DMA0;
                input = INPUT_LOCAL;
            } else if (src >= DVB_DEMUX_SECSOURCE_DMA0 &&
                src <= DVB_DEMUX_SECSOURCE_DMA7) {
                source = DMA_0 + src - DVB_DEMUX_SECSOURCE_DMA0;
                input = INPUT_LOCAL_SEC;
            } else if (src >= DVB_DEMUX_SOURCE_DMA0_1 &&
                src <= DVB_DEMUX_SOURCE_DMA7_1) {
                source = DMA_0_1 + src - DVB_DEMUX_SOURCE_DMA0_1;
                input = INPUT_LOCAL;
            } else if (src >= DVB_DEMUX_SECSOURCE_DMA0_1 &&
                src <= DVB_DEMUX_SECSOURCE_DMA7_1) {
                source = DMA_0_1 + src - DVB_DEMUX_SECSOURCE_DMA0_1;
                input = INPUT_LOCAL_SEC;
            } else if (src >= DVB_DEMUX_SOURCE_TS0_1 &&
                src <= DVB_DEMUX_SOURCE_TS7_1) {
                source = FRONTEND_TS0_1 + src - DVB_DEMUX_SOURCE_TS0_1;
                input = INPUT_DEMOD;
            } else {
               DMX_ERR("DvbSetDemuxSource:%d invalid source:%d", __LINE__, src);
               close(fd2);
               return -1;
            }

            if (ioctl(fd2, DMX_SET_INPUT, input) < 0)
            {
                 DMX_DBG("DvbSetDemuxSource ioctl DMX_SET_INPUT:%d error:%d", input, errno);
                 r = -1;
            }
            else
            {
                 DMX_DBG("DvbSetDemuxSource ioctl succeeded src:%d DMX_SET_INPUT:%d dmx_idx:%d", src, input, dmx_idx);
                 r = 0;
            }
            if (ioctl(fd2, DMX_SET_HW_SOURCE, source) < 0)
            {
                DMX_DBG("DvbSetDemuxSource ioctl DMX_SET_HW_SOURCE:%d error:%d", source, errno);
                r = -1;
            }
            else
            {
                DMX_DBG("DvbSetDemuxSource ioctl succeeded src:%d DMX_SET_HW_SOURCE:%d dmx_idx:%d", src, source, dmx_idx);
                r = 0;
            }
            close(fd2);
        }
        else
        {
            DMX_ERR("DvbSetDemuxSource open \"%s\" failed, error:%d", node, errno);
        }
    }
    else
    {
        char *val = NULL;

        close(fd);

        if (ciplus_enable)
        {
            char buf[32];
            int i, out;

            out = 0;

            for (i = 0; i < 3; i ++)
            {
                DVB_DemuxSource_t dmx_src = DVB_DEMUX_SOURCE_TS0;

                if (i == dmx_idx)
                    dmx_src = src;
                else
                    DvbGetDemuxSource(i, &dmx_src);
                if (dmx_src != DVB_DEMUX_SOURCE_DMA0)
                    out |= 1 << i;
            }

            snprintf(buf, sizeof(buf), "%d", out);
            STB_File_Echo("/sys/class/dmx/ciplus_output_ctrl", buf);
        }

        switch (src)
        {
        case DVB_DEMUX_SOURCE_TS0:
        case DVB_DEMUX_SOURCE_TS0_1:
            val = "ts0";
            break;
        case DVB_DEMUX_SOURCE_TS1:
        case DVB_DEMUX_SOURCE_TS1_1:
            val = "ts1";
            break;
        case DVB_DEMUX_SOURCE_TS2:
        case DVB_DEMUX_SOURCE_TS2_1:
            val = "ts2";
            break;
        case DVB_DEMUX_SOURCE_DMA0:
        case DVB_DEMUX_SOURCE_DMA1:
        case DVB_DEMUX_SOURCE_DMA2:
        case DVB_DEMUX_SOURCE_DMA3:
        case DVB_DEMUX_SOURCE_DMA4:
        case DVB_DEMUX_SOURCE_DMA5:
        case DVB_DEMUX_SOURCE_DMA6:
        case DVB_DEMUX_SOURCE_DMA7:
            val = "hiu";
            break;
        default:
            DMX_ERR("DvbSetDemuxSource:%d invalid source:%d", __LINE__, src);
            return -1;
        }

        r = STB_File_Echo(node, val);
    }
    return r;
}

static int DvbGetDemuxSource(int dmx_idx, DVB_DemuxSource_t *src)
{
    char node[32] = {0};
    char node2[20] = {0};
    char buf[32] = {0};
    int r = 0;
    int source_no = 0;

    snprintf(node, sizeof(node), "/sys/class/stb/demux%d_source", dmx_idx);
    snprintf(node2, sizeof(node2), "/dev/dvb0.demux%d", dmx_idx);

    int fd = open(node, O_RDONLY);
    if (fd == -1)
    {
        int source;
        int fd2 = open(node2, O_RDONLY);
        if (fd2 != -1)
        {
            if (ioctl(fd2, DMX_GET_HW_SOURCE, &source) >= 0)
            {
                switch (source)
                {
                case FRONTEND_TS0:
                    *src = DVB_DEMUX_SOURCE_TS0;
                    break;
                case FRONTEND_TS1:
                    *src = DVB_DEMUX_SOURCE_TS1;
                    break;
                case FRONTEND_TS2:
                    *src = DVB_DEMUX_SOURCE_TS2;
                    break;
                case FRONTEND_TS3:
                    *src = DVB_DEMUX_SOURCE_TS3;
                    break;
                case FRONTEND_TS4:
                    *src = DVB_DEMUX_SOURCE_TS4;
                    break;
                case FRONTEND_TS5:
                    *src = DVB_DEMUX_SOURCE_TS5;
                    break;
                case FRONTEND_TS6:
                    *src = DVB_DEMUX_SOURCE_TS6;
                    break;
                case FRONTEND_TS7:
                    *src = DVB_DEMUX_SOURCE_TS7;
                    break;
                case DMA_0:
                    *src = DVB_DEMUX_SOURCE_DMA0;
                    break;
                case DMA_1:
                    *src = DVB_DEMUX_SOURCE_DMA1;
                    break;
                case DMA_2:
                    *src = DVB_DEMUX_SOURCE_DMA2;
                    break;
                case DMA_3:
                    *src = DVB_DEMUX_SOURCE_DMA3;
                    break;
                case DMA_4:
                    *src = DVB_DEMUX_SOURCE_DMA4;
                    break;
                case DMA_5:
                    *src = DVB_DEMUX_SOURCE_DMA5;
                    break;
                case DMA_6:
                    *src = DVB_DEMUX_SOURCE_DMA6;
                    break;
                case DMA_7:
                    *src = DVB_DEMUX_SOURCE_DMA7;
                    break;
                case FRONTEND_TS0_1:
                    *src = DVB_DEMUX_SOURCE_TS0_1;
                    break;
                case FRONTEND_TS1_1:
                    *src = DVB_DEMUX_SOURCE_TS1_1;
                    break;
                case FRONTEND_TS2_1:
                    *src = DVB_DEMUX_SOURCE_TS2_1;
                    break;
                case FRONTEND_TS3_1:
                    *src = DVB_DEMUX_SOURCE_TS3_1;
                    break;
                case FRONTEND_TS4_1:
                    *src = DVB_DEMUX_SOURCE_TS4_1;
                    break;
                case FRONTEND_TS5_1:
                    *src = DVB_DEMUX_SOURCE_TS5_1;
                    break;
                case FRONTEND_TS6_1:
                    *src = DVB_DEMUX_SOURCE_TS6_1;
                    break;
                case FRONTEND_TS7_1:
                    *src = DVB_DEMUX_SOURCE_TS7_1;
                    break;
                case DMA_0_1:
                    *src = DVB_DEMUX_SOURCE_DMA0_1;
                    break;
                case DMA_1_1:
                    *src = DVB_DEMUX_SOURCE_DMA1_1;
                    break;
                case DMA_2_1:
                    *src = DVB_DEMUX_SOURCE_DMA2_1;
                    break;
                case DMA_3_1:
                    *src = DVB_DEMUX_SOURCE_DMA3_1;
                    break;
                case DMA_4_1:
                    *src = DVB_DEMUX_SOURCE_DMA4_1;
                    break;
                case DMA_5_1:
                    *src = DVB_DEMUX_SOURCE_DMA5_1;
                    break;
                case DMA_6_1:
                    *src = DVB_DEMUX_SOURCE_DMA6_1;
                    break;
                case DMA_7_1:
                    *src = DVB_DEMUX_SOURCE_DMA7_1;
                    break;
                default:
                    DMX_ERR("DvbGetDemuxSource invalid source:%d", source);
                    r = -1;
                }
            }
            else
            {
                DMX_ERR("ioctl DMX_GET_HW_SOURCE:%d error:%d", source, errno);
            }
            close(fd2);
        }
        else
        {
            DMX_ERR("opening \"%s\" failed with errno:%d", node2, errno);
        }
    }
    else
    {
        close(fd);
        r = STB_File_Read(node, buf, sizeof(buf)-1);
        if (r != -1)
        {
            buf[r] = '\0';
            if (strncmp(buf, "ts", 2) == 0 && strlen(buf) == 3)
            {
                if (sscanf(buf, "ts%d", &source_no) == 1)
                {
                    switch (source_no)
                    {
                    case 0:
                        *src = DVB_DEMUX_SOURCE_TS0;
                        break;
                    case 1:
                        *src = DVB_DEMUX_SOURCE_TS1;
                        break;
                    case 2:
                        *src = DVB_DEMUX_SOURCE_TS2;
                        break;
                    default:
                        DMX_DBG("do not support demux source:%s", buf);
                        r = -1;
                        break;
                    }
                }
                else
                {
                    r = -1;
                }
            }
            else if (strncmp(buf, "hiu", 3) == 0)
            {
                *src = DVB_DEMUX_SOURCE_DMA0;
            }
            else
            {
                r = -1;
            }
            DMX_DBG("DvbGetDemuxSource \"%s\" :%s", node, buf);
        }
    }
    return r;
}

static int DvbEnableCIPlus(int enable)
{
    int out;
    char buf[32];

    ciplus_enable = enable;

    if (STB_IsNewHW())
        return 0;

    if (enable)
    {
        int i;

        out = 0;

        for (i = 0; i < 3; i ++)
        {
            DVB_DemuxSource_t src = DVB_DEMUX_SOURCE_TS0;

            DvbGetDemuxSource(i, &src);
            if (src != DVB_DEMUX_SOURCE_DMA0)
                out |= 1 << i;
        }
    }
    else
    {
        out = 8;
    }

    snprintf(buf, sizeof(buf), "%d", out);
    STB_File_Echo("/sys/class/dmx/ciplus_output_ctrl", buf);

    return 0;
}

BOOLEAN STB_DMXSetSource(U8BIT dmx_idx, U8BIT src)
{
    return DvbSetDemuxSource(dmx_idx, (DVB_DemuxSource_t)src);
}

BOOLEAN STB_DMXGetDevNo(U8BIT path , U8BIT *dev_no)
{
    BOOLEAN retval = FALSE;
    FUNCTION_START(STB_DMXGetDevNo);
    if (path < num_paths)
    {
        DMX_DBG("STB_DMXGetDevNo [%d]!", demux_status[path].path);
       *dev_no = demux_status[path].path;
        retval = TRUE ;
    }
    FUNCTION_FINISH(STB_DMXGetDevNo);
    return retval;
}
