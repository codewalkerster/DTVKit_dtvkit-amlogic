/*******************************************************************************
 * Copyright © 2014 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * Copyright © 2004 Ocean Blue Software Ltd
 *
 * This file is part of a DTVKit Software Component
 * You are permitted to copy, modify or distribute this file subject to the terms
 * of the DTVKit 1.0 Licence which can be found in licence.txt or at www.dtvkit.org
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHout WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * If you or your organisation is not a member of DTVKit then you have access
 * to this source code outside of the terms of the licence agreement
 * and you are expected to delete this and any associated files immediately.
 * Further information on DTVKit, membership and terms can be found at www.dtvkit.org
 *******************************************************************************/
/**
 * @brief   Header file - Function prototypes for Demux control
 * @file    stbhwdmx.h
 * @date    06/02/2001
 */

// pre-processor mechanism so multiple inclusions don't cause compilation error

#ifndef _STBHWDMX_H

#define _STBHWDMX_H

#include "techtype.h"
#include "stbhwav.h"

//---Constant and macro definitions for public use-----------------------------
#define MAX_HW_SECT_FILT_LEN 8

#define STB_DMX_PID_FILTER_INVALID       0xffff
#define STB_DMX_SECT_FILTER_INVALID      0xffff

#define STB_TPID_CBUFF_PRIORITY          10

//---Enumerations for public use-----------------------------------------------

// definitions for the descrambler
typedef enum
{
    DESC_TRACK_AUDIO,
    DESC_TRACK_VIDEO,
    DESC_TRACK_TEXT,
    DESC_NUM_TRACKS
} E_STB_DMX_DESC_TRACK;

typedef enum
{
    KEY_PARITY_EVEN,
    KEY_PARITY_ODD,
    KEY_PARITY_NONE
} E_STB_DMX_DESC_KEY_PARITY;

typedef enum
{
    DESC_TYPE_DVB,
    DESC_TYPE_AES,
    DESC_TYPE_AES_SCTE_52,
    DESC_TYPE_DES,
    DESC_TYPE_TDES
} E_STB_DMX_DESC_TYPE;

typedef enum
{
    KEY_USAGE_PES,
    KEY_USAGE_TRANSPORT,
    KEY_USAGE_ALL
} E_STB_DMX_KEY_USAGE;

typedef enum
{
    DMX_TUNER,
    DMX_1394,
    DMX_MEMORY
} E_STB_DMX_DEMUX_SOURCE;

typedef enum
{
    DMX_CAPS_LIVE = 0x0001,          /* Demux can be used to watch live TV */
    DMX_CAPS_PIP = 0x0002,           /* Demux can be used for picture-in-picture */
    DMX_CAPS_RECORDING = 0x0004,     /* Demux can be used for PVR recording */
    DMX_CAPS_PLAYBACK = 0x0008,      /* Demux can be used for PVR playback */
    DMX_CAPS_MONITOR_SI = 0x0010,    /* Demux can be used to monitor SI data from a tuner */
    DMX_CAPS_NO_DSC_NEEDED = 0x0020, /* Demux can be assign to whom need no dsc */
    DMX_CAPS_USBCAM = 0x0040         /* Demux can be assign to usbcam */
} E_STB_DMX_CAPS;


typedef enum
{
    STB_TS_SOURCE0,
    STB_TS_SOURCE1,
    STB_TS_SOURCE2,
    STB_TS_SOURCE3,
    STB_TS_SOURCE_MAX
} E_STB_TS_SOURCE;

typedef enum
{
    STB_DMX_MODEL_905X2 = 0,
    STB_DMX_MODEL_SC2
} E_STB_BOARD_TYPE;

typedef enum
{
    DSC_COMMON_TYPE,
    DSC_TSD_TYPE,
    DSC_TSE_TYPE
} E_STB_DSC_CA_TYPE;

//---Global type defs for public use-------------------------------------------

typedef void (*FILTER_CALLBACK)( U8BIT path, U16BIT bytes, U16BIT pfilt_id );
typedef void (*TEMI_FILTER_CALLBACK)( void *context, U64BIT pts, U8BIT descr_tag, U8BIT descr_data_len, U8BIT *descr_data );

// B: For ATSC1.0
typedef enum
{
   DMX_PID_TYPE_VIDEO,
   DMX_PID_TYPE_AUDIO,
   DMX_PID_TYPE_PCR,
   DMX_PID_TYPE_SUBTITLES,
   DMX_PID_TYPE_SECTION,
} E_DMX_PID_TYPE;

typedef struct
{
   E_DMX_PID_TYPE type;
   U16BIT pid;
   union
   {
      E_STB_AV_VIDEO_CODEC video_codec;
      E_STB_AV_AUDIO_CODEC audio_codec;
   } u;
} S_DMX_PID_INFO;
// E: For ATSC1.0

//---Global Function prototypes for public use---------------------------------

void STB_DMXDscFree(int dev_id, int chan_id);

void STB_DMXDscSetSrc(int dev_id, int dmx_id);

BOOLEAN STB_DMXPrepareKey(int dev_id, E_STB_DSC_CA_TYPE ca_type, E_STB_DMX_DESC_TYPE dsc_type, E_STB_DMX_DESC_KEY_PARITY parity);
int STB_DMXSetKey(int dev_id, int chan_id, E_STB_DMX_DESC_TYPE type, E_STB_DSC_CA_TYPE ca_type, E_STB_DMX_DESC_KEY_PARITY parity, U8BIT *data);

E_STB_TS_SOURCE STB_GetDmxTsSource(int dmx_id);

/**
 * @brief   alloc dsc channel
 * @param   dev_id
 * @param   pid
 * @param   type
 * @return  channel id
 */
int STB_DMXDscAlloc(int dev_id, int pid, E_STB_DMX_DESC_TYPE type, E_STB_DSC_CA_TYPE dsc_type);

/**
 * @brief   Initialises the demux / programmable transport interface
 * @param   paths Number of demux paths to be initialised
 * @param   inc_pes_collection Not used
 */
void STB_DMXInitialise(U8BIT paths, BOOLEAN inc_pes_collection);

/**
 * @brief   Returns the capability flags of the given demux
 * @param   path - demux
 * @return  Capability flags
 */
U16BIT STB_DMXGetCapabilities(U8BIT path);

/**
 * @brief   Returns the maximum number of section filters available on this hw
 * @return  The number of filters
 */
U8BIT STB_DMXGetMaxSectionFilters(void);

/**
 * @brief   Configures the source of the demux
 * @param   path the demux path to configure
 * @param   source the source to use
 * @param   param source specific parameters (e.g. tuner number)
 */
void STB_DMXSetDemuxSource(U8BIT path, E_STB_DMX_DEMUX_SOURCE source, U8BIT param, U16BIT demux_cap);

/**
 * @brief   Gets the current source of a given demux
 * @param   path the demux path to query
 * @param   source the source of the demux
 * @param   param the source specific parameter (e.g. tuner number)
 */
void STB_DMXGetDemuxSource(U8BIT path, E_STB_DMX_DEMUX_SOURCE *source, U8BIT *param);

/**
 * @brief   Reads Teletext PES data from the demux
 * @param   path the demux path to read
 * @param   buffer pointer to PES data
 * @param   num_bytes the number of bytes of data
 */
void STB_DMXReadTextPES(U8BIT path, U8BIT **buffer, U32BIT *num_bytes);

/**
 * @brief   Changes the packet IDs for the PCR Video, Audio, Text and Data
 * @param   path The demux path to be configured
 * @param   pcr_pid The PID to use for the Program Clock Reference
 * @param   video_pid The PID to use for the Video PES
 * @param   audio_pid The PID to use for the Audio PES
 * @param   text_pid The PID to use for the Teletext data
 * @param   data_pid The PID to use for the data
 * @param   ad_pid The PID to use for the data
 * @param   preselection_id The PID to use for the data
 */
void STB_DMXChangeDecodePIDs(U8BIT path, U16BIT pcr_pid, U16BIT video_pid, U16BIT audio_pid,
                             U16BIT text_pid, U16BIT data_pid, U16BIT ad_pid, U8BIT preselection_id);

/**
 * @brief   Changes just the teletext PID
 * @param   path The demux path to configure
 * @param   text_pid The PID to use for the teletext data
 */
void STB_DMXChangeTextPID(U8BIT path, U16BIT text_pid);

/**
 * @brief   Start specified TEMI filter collecting data.
 * @param   path Required decode path number.
 * @param   pid Required PID to demux.
 * @param   func Function to report any TEMI related AF descriptor
 * @param   context Parameter to be used as context in callbacks from this filter
 * @return  New TEMI filter identifier or invalid id.
 */
U16BIT STB_DMXStartTemiFilter(U8BIT path, U16BIT pid, TEMI_FILTER_CALLBACK func, void *context);

/**
 * @brief   Stop specified TEMI filter.
 * @param   path Required Decode Path Number.
 * @param   filter_id Required TEMI filter identifier.
 */
void STB_DMXStopTemiFilter(U8BIT path, U16BIT filt_id);

/**
 * @brief   Get a New PID Filter & Setup Associated Buffer and Callback
 *          Function Address.
 * @param   path Required Decode Path Number.
 * @param   pid Required PID to Demux.
 * @param   func_ptr User's Interrupt Procedure Function Address.
 * @return  New PID filter identifier or invalid id.
 */
U16BIT STB_DMXGrabPIDFilter(U8BIT path, U16BIT pid, FILTER_CALLBACK func);

/**
 * @brief   Copies a filtered section to caller's buffer
 * @param   path the demux path to use
 * @param   buffer the caller's buffer
 * @param   size the size of the caller's buffer
 * @param   pfilt_id the handle of the PID filter to read from
 * @return  TRUE copied ok
 * @return  FALSE no data to copy
 */
BOOLEAN STB_DMXCopyPIDFilterSect(U8BIT path, U8BIT *buffer, U16BIT size, U16BIT pfilt_id);

/**
 * @brief   Skips (discards) a section in the PID filter buffer
 * @param   path the demux path of the filter
 * @param   pfilt_id the PID filter handle
 */
void STB_DMXSkipPIDFilterSect(U8BIT path, U16BIT pfilt_id);

/**
 * @brief   Allocated a new section filter on the specified PID filter
 * @param   path the demux path to use
 * @param   pfilt_id the PID filter to assign the section filter to
 * @return  The section filter handle
 */
U16BIT STB_DMXGrabSectFilter(U8BIT path, U16BIT pfilt_id);

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
                            U8BIT not_equal_byte_index, BOOLEAN crc);

/**
 * @brief   Start Specified PID Filter Collecting Data.
 * @param   path Required Decode Path Number.
 * @param   pfilter_id Required PID Filter Identifier.
 */
void STB_DMXStartPIDFilter(U8BIT path, U16BIT pfilt_id);

/**
 * @brief   Stop Specified PID Filter Collecting Data.
 * @param   path Required Decode Path Number.
 * @param   pfilter_id Required PID Filter Identifier.
 */
void STB_DMXStopPIDFilter(U8BIT path, U16BIT pfilt_id);

/**
 * @brief   Releases a previously allocated PID filter
 * @param   path the demux path of the filter
 * @param   pfilt_id the handle of the filter
 */
void STB_DMXReleasePIDFilter(U8BIT path, U16BIT pfilt_id);

/**
 * @brief   Releases a previously allocated section filter
 * @param   path the demux path of the filter
 * @param   sfilt_id the handle of the section filter
 */
void STB_DMXReleaseSectFilter(U8BIT path, U16BIT sfilt_id);

/**
 * @brief   Acquires a descrambler for the specified track on this path
 * @param   path the demux path for which the descrambler is acquired
 * @param   track enum representing audio, video or subtitles PES
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXGetDescramblerKey(U8BIT path, E_STB_DMX_DESC_TRACK track);

/**
 * @brief   Frees the descrambler for the specified track on this path
 * @param   path the demux path for which the descrambler is freed
 * @param   track enum representing audio, video or subtitles PES
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXFreeDescramblerKey(U8BIT path, E_STB_DMX_DESC_TRACK track);

/**
 * @brief   Set the descrambler key data for the specified track on this path
 * @param   path the demux path for which the descrambler key data is set
 * @param   track enum representing audio, video or subtitles PES
 * @param   parity even or odd
 * @param   data pointer to the key data, its length depends on the descrambler
 *          type (see STB_DMXSetDescramblerType)
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXSetDescramblerKeyData(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_DESC_KEY_PARITY parity, U8BIT *data);

/**
 * @brief   Get the descrambler key usage for the specified track on this path as set by
 *          STB_DMXSetKeyUsage
 * @param   path the demux path that the descrambler key usage refers to
 * @param   track enum representing audio, video or subtitles PES
 * @param   key_usage whether the descrambler has been set to operate at PES level, transport
 *          level or all.
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXGetKeyUsage(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_KEY_USAGE *key_usage);

/**
 * @brief   Set the descrambler key usage for the specified track on this path
 * @param   path the demux path that the descrambler key usage refers to
 * @param   track enum representing audio, video or subtitles PES
 * @param   key_usage whether the descrambler operates at PES level, transport
 *          level or all.
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXSetKeyUsage(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_KEY_USAGE key_usage);

/**
 * @brief   Get the descrambler type for the specified track on this path, as set by
 *          STB_DMXSetDescramblerType
 * @param   path the demux path that the descrambler type refers to
 * @param   type descrambler type (DES, AES, etc...)
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXGetDescramblerType(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_DESC_TYPE *type);

/**
 * @brief   Set the descrambler type for the specified track on this path
 * @param   path the demux path that the descrambler type refers to
 * @param   type descrambler type (DES, AES, etc...)
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXSetDescramblerType(U8BIT path, E_STB_DMX_DESC_TRACK track, E_STB_DMX_DESC_TYPE type);

/**
 * @brief   Get Board type, different board has different demux structure.
 * @return  Board type E_STB_BOARD_TYPE.
 */
E_STB_BOARD_TYPE STB_DMXGetModel();

/**
 * @brief   Writes data to the demux from memory
 * @param   path the demux path to be written
 * @param   data the data to be written
 * @param   size the number of bytes to be written
 */
void STB_DMXWriteDemux(U8BIT path, U8BIT *data, U32BIT size);
/**
 * @brief   change the source of the demux when cam card plug or unplug
 *          we need check "is_set_tssource" is 0 or not,if it value is 0,
 *          we do nothing now.
 * @param   slot  cam card slot
 * @param   plug 0:cam card unplug, 1：camc card plug
 */
void STB_DMXChangeAllDemuxSource(U8BIT slot, U8BIT plug);

/**
 * @brief   set the tsout source when ts route is "tsin->tsout->tsin"
 * get ts out source from cfg
 */
void STB_SetTsoutSource(BOOLEAN is_cam_plugin);

/**
 * @brief Start the CI signal monitor.
 */
void STB_DMXCISignalMonitorStart();

/**
 * @brief Stop the CI signal monitor.
 */
void STB_DMXCISignalMonitorStop();

/**
 * @brief set demod mode api.
 */
void STB_DMXCI_Set_Demod_Mode(int mode);

/**
 * @brief   Reset the source of the demux
 * @param   path the demux path to configure
 */
void STB_DMXResetDemuxSource(U8BIT path);

/**
 * @brief   set demux hw source
 */
BOOLEAN STB_DMXSetSource(U8BIT dmx_idx, U8BIT src);

/**
 * @brief   Get the demux specified on this path of demux driver /dev/dvb0.demux%d
 * @param   path the demux path that the demux type refers to
 * @return  TRUE on success, FALSE otherwise
 */
BOOLEAN STB_DMXGetDevNo(U8BIT path , U8BIT *dev_no);

// B: For ATSC1.0
/**
 * @brief   Sets the array of PIDs that make up a service and would be included in a
 *          single program transport stream (SPTS) when streaming.
 *          This function will be called with an updated list if the PIDs change,
 *          and with num_pids=0 when streaming is to be stopped.
 * @param   path demux path
 * @param   num_pids number of PIDs in the array
 * @param   pid_array array of PIDs to be included
 */
void STB_DMXSetServicePids(U8BIT path, U16BIT num_pids, S_DMX_PID_INFO *pid_array);
// E: For ATSC1.0
BOOLEAN DMXGetDecodePIDs(U8BIT path, U16BIT *pcr_pid, U16BIT *video_pid, U16BIT *audio_pid, U16BIT *ad_pid, U8BIT *preselection_id);


#endif //  _STBHWDMX_H

