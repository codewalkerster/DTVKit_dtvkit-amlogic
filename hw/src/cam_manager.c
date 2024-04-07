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


#include <stdint.h>
#include "techtype.h"
#include "cam_manager.h"
#include "stbhwdmx.h"
#include "stbhwcfg.h"
#include "stb_utils.h"
// #include "stbdpc.h"
#include "dbgfuncs.h"
#include "stbhwtun.h"
#include "stbhwc.h"
#include "stbhwos.h"
#include "stbhwmem.h"
#include "stbhwdemux_usb.h"

#ifdef ATF_USBCAM
#include "wrapper_dmx.h"
#endif

#define STB_TSO_SOURCE "/sys/class/stb/tso_source"

#define TAG  "STB_CAM_MANGER"

#define CAM_MANGER_DEBUG
#ifdef CAM_MANGER_DEBUG
#define CAM_MANGER_DBG(x, ...) DTV_LOG(ANDROID_LOG_INFO, TAG, "CAM_MANGER %s:%d " x, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define CAM_MANGER_DBG(x, ...)
#endif

CAM_Manager_t CAM_Manager;
static CAM_Manager_State cur_manager_state;
CAM_Card_State cur_card_state;

static BOOLEAN (*CAM_DecodingPath_CallBack)(U8BIT i) = NULL;
static BOOLEAN (*CAM_RecordingPath_CallBack)(U8BIT i) = NULL;

static void CAM_SetPcmcia_DmxSource(int num, int slot_id)
{
   int i = 0;
   int tuner_index = 0;
   int param = 0;

   CAM_Manager_State manager_state = CAM_MAN_STATE_PCMCIA;
   aml_hw_cfg.tuners[num].ts_input_idx = aml_hw_cfg.cam[slot_id].camPlug_tssource;

   STB_File_Echo("/sys/class/stb/demux_reset", "1");

   for (i = 0; i < aml_hw_cfg.demux_num; i++)
   {
      // change ts_input_idx
      E_STB_DMX_DEMUX_SOURCE source;
      U8BIT param;
      STB_DMXGetDemuxSource(i, &source, &param);
      if (source == DMX_TUNER)
      {
         if (STB_DMXGetModel())
         {
             if (CAM_DecodingPath_CallBack(i))
                param = DMX_CAPS_LIVE;
             if (CAM_RecordingPath_CallBack(i))
                param = DMX_CAPS_RECORDING;
            STB_DMXSetDemuxSource(i, DMX_TUNER, tuner_index, param);
         }
         else
         {
            STB_File_Echo("/sys/class/stb/demux_reset", "1");
            STB_DMXSetDemuxSource(i, DMX_TUNER, tuner_index, param);
         }
      }
   }
}

void CAM_IsUsbcam(BOOLEAN enable)
{
   DVB_DemuxSource_t usb_source;

   if (enable)
      usb_source = STB_CIUsbGetDmxSource(TRUE);
   else
      usb_source = DVB_DEMUX_SOURCE_MAX;

   STB_DMXSetSourceUsb(usb_source);
}

static void CAM_SetUsbcam_DmxSource(int num)
{
   int i = 0;
   int tuner_index = 0;
   int param = 0;
   CAM_Manager_State manager_state = CAM_MAN_STATE_USB;

   aml_hw_cfg.tuners[num].ts_input_idx = STB_CIUsbGetDmxSource(TRUE);
   STB_File_Echo("/sys/class/stb/demux_reset", "1");
   for (i = 0; i < aml_hw_cfg.demux_num; i++)
   {
      // change ts_input_idx
      E_STB_DMX_DEMUX_SOURCE source;
      U8BIT param;
      STB_DMXGetDemuxSource(i, &source, &param);
      if (source == DMX_TUNER)
      {
         if (STB_DMXGetModel())
         {
             if (CAM_DecodingPath_CallBack(i))
                param = DMX_CAPS_LIVE;
             if (CAM_RecordingPath_CallBack(i))
                param = DMX_CAPS_RECORDING;
            STB_DMXSetDemuxSource(i, DMX_TUNER, tuner_index, param);
         }
         else
         {
            STB_File_Echo("/sys/class/stb/demux_reset", "1");
            STB_DMXSetDemuxSource(i, DMX_TUNER, tuner_index, param);
         }
      }
   }
}



static void CAM_SetTuner_DmxSource(int num)
{
   int i = 0;
   int tuner_index = 0;
   int param = 0;
   CAM_Manager_State manager_state = CAM_MAN_STATE_BYPASS;
   aml_hw_cfg.tuners[num].ts_input_idx = aml_hw_cfg.tuners[num].ori_tsinput_idx;

   STB_File_Echo("/sys/class/stb/demux_reset", "1");

   for (i = 0; i < aml_hw_cfg.demux_num; i++)
   {
      // change ts_input_idx
      E_STB_DMX_DEMUX_SOURCE source;
      U8BIT param;
      STB_DMXGetDemuxSource(i, &source, &param);
      if (source == DMX_TUNER)
      {
         if (STB_DMXGetModel())
         {
             if (CAM_DecodingPath_CallBack(i))
                param = DMX_CAPS_LIVE;
             if (CAM_RecordingPath_CallBack(i))
                param = DMX_CAPS_RECORDING;
            STB_DMXSetDemuxSource(i, DMX_TUNER, tuner_index, param);
         }
         else
         {
            STB_File_Echo("/sys/class/stb/demux_reset", "1");
            STB_DMXSetDemuxSource(i, DMX_TUNER, tuner_index, param);
         }
      }
   }
}

void CAM_Register_CallBack(BOOLEAN (*decodingpath_callback)(U8BIT i),BOOLEAN (*recordingpath_callback)(U8BIT i))
{
   if (decodingpath_callback)
        CAM_DecodingPath_CallBack = decodingpath_callback;
   if (recordingpath_callback)
        CAM_RecordingPath_CallBack = recordingpath_callback;
}

int usb_slotid = -1;
int pcmcia_slotid = -1;

void CAM_UpdateTsStatus(int slot_id, CAM_TsStatus_t status)
{
   int i = 0;
   int tuner_index = 0;
   U8BIT slot = 0;
   int param = 0;

   CAM_Manager_State manager_state;

   if (cur_card_state == CAM_CARD_NONE)
   {
      for (i = 0; i < aml_hw_cfg.ci_slot_num; i++)
      {
         if (CAM_Manager.CAM[i].insert == 1)
         {
            if (CAM_Manager.CAM[i].device_type == CAM_DEVICE_USB) {
               usb_slotid = i;
            }
            else if (CAM_Manager.CAM[i].device_type == CAM_DEVICE_PCMCIA) {
               pcmcia_slotid = i;
            }
         }
      }
   }

   if (usb_slotid >= 15) {
      cur_card_state = CAM_CARD_USB;
      slot = usb_slotid;
      manager_state = CAM_MAN_STATE_USB;
   }

   if (pcmcia_slotid >= 0) {
      cur_card_state = CAM_CARD_PCMCIA;
      slot = pcmcia_slotid;
      manager_state = CAM_MAN_STATE_PCMCIA;
   }

   if (usb_slotid < 0 && pcmcia_slotid < 0) {
      cur_card_state = CAM_CARD_NONE;
      manager_state = CAM_MAN_STATE_BYPASS;
   }

   if (cur_manager_state != manager_state)
   {
#ifdef ATF_USBCAM
      DMX_Route_TS(slot_id, status);
#else
      for (i = 0; i < aml_hw_cfg.tuner_num; i++)
      {
         if (manager_state == CAM_MAN_STATE_BYPASS)
         {
            CAM_IsUsbcam(FALSE);
            CAM_SetTuner_DmxSource(i);
         }
         else if (manager_state == CAM_MAN_STATE_USB)
         {
            CAM_IsUsbcam(TRUE);
            CAM_SetUsbcam_DmxSource(i);
         }
         else if (manager_state == CAM_MAN_STATE_PCMCIA)
         {
            CAM_IsUsbcam(FALSE);
            CAM_SetPcmcia_DmxSource(i, slot);
         }
      }
#endif
      cur_manager_state = manager_state;
   }
   CAM_MANGER_DBG("CAM_UpdateTsStatus cur_manager_state %u manager_state %u slot %u",cur_manager_state ,manager_state,slot);
}

CAM_Card_State CAM_GetCamCardState()
{
   return cur_card_state;
}

int CAM_Insert(int slot_id, CAM_DeviceType_t type)
{

        CAM_Manager.CAM[slot_id].device_type = type;
        CAM_Manager.CAM[slot_id].slot_id = slot_id;
        CAM_Manager.CAM[slot_id].insert = 1;
        return 0;
}

int CAM_SetTsStatus(int slot_id, CAM_TsStatus_t status)
{
        CAM_Manager.CAM[slot_id].through = status;

        CAM_UpdateTsStatus(slot_id, status);

        return 0;
}

int CAM_Remove(int slot_id)
{
        CAM_Manager.CAM[slot_id].insert = 0;
        CAM_Manager.CAM[slot_id].through = CAM_TS_BYPASS;
         if (CAM_Manager.CAM[slot_id].device_type == CAM_DEVICE_USB) {
            usb_slotid = -1;
         }
         else if (CAM_Manager.CAM[slot_id].device_type == CAM_DEVICE_PCMCIA) {
            pcmcia_slotid = -1;
         }
      //   CAM_UpdateTsStatus();
        return 0;
}

