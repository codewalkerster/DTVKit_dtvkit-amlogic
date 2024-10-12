/*******************************************************************************
 * Copyright ?2014 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * Copyright ?2013 Ocean Blue Software Ltd
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
 * @brief   Glue layer between DVB and conditional access systems
 * @file    ca_glue.h
 * @date    10/09/2013
 * @author  Steve Ford
 */

/* pre-processor mechanism so multiple inclusions don't cause compilation error*/
#ifndef __CA_GLUE_H
#define __CA_GLUE_H

#ifndef DTVKIT_WITH_AML_MP_SDK
#define DTVKIT_WITH_AML_MP_SDK
#endif

#include "techtype.h"

#define CAS_MSG_LEN 8192
#define EVENT_TYPE_PVR_METADATA  0x111

/*---Constant and macro definitions for public use-----------------------------*/

/*---Enumerations for public use-----------------------------------------------*/
typedef enum
{
    CA_DECODE_STATUS_STARTING,
    CA_DECODE_STATUS_STARTED,
    CA_DECODE_STATUS_STOPPED
} E_CA_DECODE_STATUS;

typedef enum
{
    CAS_TYPE_NONE,
    CAS_TYPE_IRDETO,
    CAS_TYPE_VMX,
    CAS_TYPE_NAGRA,
    CAS_TYPE_WV
} E_CAS_TYPE;


typedef struct
{
    U8BIT path;
    U8BIT pathType;
    UINTPTR session;
    char data_str[CAS_MSG_LEN];
    int event;
    int args;
    U32BIT len;
} CAS_EVENT_DATA_t;

typedef struct
{
    U8BIT path;
    U8BIT pathType;
    UINTPTR session;
    char *long_data_str;
} CAS_EVENT_LONG_DATA_t;

/*---Global type defs for public use-------------------------------------------*/

/*---Global Function prototypes for public use---------------------------------*/

/*!**************************************************************************
 * @brief   Called once on system startup to allow initialisation of the CA systems
 * @return  TRUE if initialisation is successful, FALSE otherwise
 ****************************************************************************/
BOOLEAN STB_CAInitialise(void);


/*!**************************************************************************
 * @brief   This function is used by the resource manager to acquire a CA descrambler
 *          that's able to descramble a service that uses one of the CA systems
 *          defined by the array of CA system IDs (ca_ids). If a descrambler is
 *          available then a handle should be returned in 'handle' which will be
 *          used in all future calls related to this descrambler.
 *          If the CA software needs to set the demux descrambling keys, or create
 *          any filters to monitor SI data, the given demux handle should be used.
 * @param   path - the ID of the decode path to use if a descrambler is acquired
 * @param   serv_id - ID of the service the descrambler is being acquired for
 * @param   ca_ids - array of CA system IDs for the service
 * @param   num_ca_ids - number of CA system IDs in the array
 * @param   handle - pointer to return a handle to identify the acquired CA descrambler
 * @return  TRUE if a descrambler is acquired, FALSE otherwise
 ****************************************************************************/
BOOLEAN STB_CAAcquireDescrambler(U8BIT path, U16BIT serv_id, U16BIT *ca_ids, U16BIT num_ca_ids,
                                 UINTPTR *handle);

/*!**************************************************************************
 * @brief   Will be called when a CA descrambler is no longer required.
 * @param   handle - CA descrambler handle being released
 * @return  TRUE if the descrambler is released, FALSE otherwise
 ****************************************************************************/
BOOLEAN STB_CAReleaseDescrambler(UINTPTR handle);

/*!**************************************************************************
 * @brief   This function will be called when decoding of a service is about to start
 *          and there's an associated descrambler.
 * @param   handle - CA descrambler handle
 ****************************************************************************/
void STB_CADescrambleServiceStart(UINTPTR handle);

/*!**************************************************************************
 * @brief   This function will be called when decoding of a service is stopped
 * @param   handle - CA descrambler handle
 ****************************************************************************/
void STB_CADescrambleServiceStop(UINTPTR handle);
/*!**************************************************************************
 * @brief   This function will be called when set CA descramble ioctl
 * @param   handle - CA descrambler handle
 ****************************************************************************/
void STB_CADescrambleIoctl(UINTPTR handle, U32BIT session, const char* inJson, char* outJson, U32BIT outLen);

/*!**************************************************************************
 * @brief   When there's an update to the PMT for a service, the updated PMT
 *          will be reported to the CA system using this function.
 * @param   handle - CA descrambler handle
 * @param   pmt_data - raw PMT section data
 * @param   data_len - number of bytes in the PMT
 ****************************************************************************/
void STB_CAReportPMT(UINTPTR handle, U8BIT *pmt_data, U16BIT data_len);

/*!**************************************************************************
 * @brief   When there's an update to the CAT for a service, the updated CAT
 *          will be reported to the CA system using this function. The data is
 *          provided a section at a time, rather than as a complete table.
 * @param   path - the decoder path
 * @param   cat_data - raw CAT section data
 * @param   data_len - number of bytes in the CAT section
 ****************************************************************************/
void STB_CAReportCAT(U8BIT path, U8BIT *cat_data, U16BIT data_len);

/*!**************************************************************************
 * @brief   When there's an update to the BAT, the updated BAT will be reported
 *          to the CA system using this function. The data is provided a section
 *          at a time, rather than as a complete table.
 * @param   handle - CA descrambler handle
 * @param   bat_data - raw BAT section data
 * @param   data_len - number of bytes in the BAT section
 ****************************************************************************/
void STB_CAReportBAT(UINTPTR handle, U8BIT *bat_data, U16BIT data_len);

/*!**************************************************************************
 * @brief   When there's an update to the NIT, the updated NIT will be reported
 *          to the CA system using this function. The data is provided a section
 *          at a time, rather than as a complete table.
  * @param   path - the decoder path
 * @param   nit_data - raw NIT section data
 * @param   data_len - number of bytes in the NIT section
 ****************************************************************************/
void STB_CAReportNIT(U8BIT path, U8BIT *nit_data, U16BIT data_len);

/*!**************************************************************************
 * @brief   Notifies the CA system of a change in the video decoding state
 * @param   handle - CA descrambler handle
 * @param   decode_status - decoding status
 ****************************************************************************/
void STB_CADecodeVideoStatus(U32BIT handle, E_CA_DECODE_STATUS decode_status);

/*!**************************************************************************
 * @brief   Notifies the CA system of a change in the audio decoding state
 * @param   handle - CA descrambler handle
 * @param   decode_status - decoding status
 ****************************************************************************/
void STB_CADecodeAudioStatus(U32BIT handle, E_CA_DECODE_STATUS decode_status);

/*!**************************************************************************
 * @brief   Notifies the CA system of a change in the AD decoding state
 * @param   handle - CA descrambler handle
 * @param   decode_status - decoding status
 ****************************************************************************/
void STB_CADecodeADStatus(U32BIT handle, E_CA_DECODE_STATUS decode_status);

/*!**************************************************************************
 * @brief   This function will be called when there's a change to the running status
 *          of a service being descrambled as indicated by the running_status field
 *          in the SDT.
 * @param   handle - CA descrambler handle
 * @param   status - running status as defined in the SDT
 ****************************************************************************/
void STB_CANotifyRunningStatus(UINTPTR handle, U8BIT status);

/*!**************************************************************************
 * @brief   This function works out whether a CA descrambler is required to playback
 *          a recording with one of the given CA system IDs.
 * @param   ca_ids - array of CA system IDs
 * @param   num_ca_ids - number of CA system IDs in the array
 * @return  TRUE if a CA descrambler is required, FALSE otherwise
 ****************************************************************************/
BOOLEAN STB_CADescramblerRequiredForPlayback(U16BIT *ca_ids, U16BIT num_ca_ids);

/*!**************************************************************************
 * @brief   This function works out whether a CA descrambler is required to record
 *          a service with one of the given CA system IDs.
 * @param   ca_ids - array of CA system IDs
 * @param   num_ca_ids - number of CA system IDs in the array
 * @return  TRUE if a CA descrambler is required, FALSE otherwise
 ****************************************************************************/
BOOLEAN STB_CADescramblerRequiredForRecording(U16BIT *ca_ids, U16BIT num_ca_ids);

/*!**************************************************************************
 * @brief   This function is called to get an array of PIDs that need to be recorded
 *          for the CA system required for the given PMT. The array must be allocated
 *          by this function, which also returns the number of items in the array.
 * @param   pmt_data - raw PMT section data
 * @param   pid_array - pointer to an array allocated by this function on return,
 *                      containing the PIDs to be recorded
 * @return  the number of PIDs in the returned array
 ****************************************************************************/
U16BIT STB_CAGetRecordingPids(U8BIT *pmt_data, U16BIT **pid_array);

/*!**************************************************************************
 * @brief   Called to free the array of PIDs allocated by STB_CAGetRecordingPids.
 * @param   pid_array - array of PIDs to be freed
 * @param   num_pids - number of PIDs in the array
 ****************************************************************************/
void STB_CAReleaseRecordingPids(U16BIT *pid_array, U16BIT num_pids);

/*!**************************************************************************
 * @brief   This function is called when a recording starts and when it stops
 * @param   handle - CA descrambler handle
 * @param   status - TRUE when a recording starts, FALSE when it stops
 ****************************************************************************/
void STB_CANotifyRecordingStatus(UINTPTR handle, BOOLEAN status);


/*!**************************************************************************
 * @brief   This function is called when need identify multi-session callback
 * @param   handle - CA descrambler handle
  * @param  handle - CA descrambler private session
 * @param   status - TRUE when when equal
 ****************************************************************************/
BOOLEAN STB_CACheckSessionStatus(UINTPTR handle, UINTPTR session);
/****************************************************************************/
/****************************************************************************/
void STB_CANotifyMetaKEY(UINTPTR handle, U8BIT path, uint8_t* data, int dataLen);
/****************************************************************************/
/****************************************************************************/
int STB_CAPVRRecordStart(UINTPTR handle);
/****************************************************************************/
/****************************************************************************/
void STB_CAPVRRecordStop(UINTPTR handle);
/****************************************************************************/
/****************************************************************************/
void STB_CAPVRPlayStart(UINTPTR handle,void *param, BOOLEAN isTimeShift);
/****************************************************************************/
/****************************************************************************/
void STB_CAPVRPlayStop(UINTPTR handle);
/****************************************************************************/
/****************************************************************************/

/***********************NEED CLEAN UP these API*******************************/
E_CAS_TYPE STB_CAGetCASType();
/******************************************************/
BOOLEAN STB_CATMSFtaBit();

#endif /* __CA_GLUE_H */

/******************************************************************************
** End of file
******************************************************************************/
