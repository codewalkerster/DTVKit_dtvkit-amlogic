/*******************************************************************************
 * Copyright © 2014 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * Copyright © 2004 Ocean Blue Software Ltd
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
 * @file    stbhwc.h
 * @brief   Function prototypes for HW control
 * @date    26/09/2000
 */

// pre-processor mechanism so multiple inclusions don't cause compilation error

#ifndef _STBHWC_H
#define _STBHWC_H

#include "techtype.h"

//---Constant and macro definitions for public use-----------------------------

//---Enumerations for public use-----------------------------------------------

typedef enum
{
    HANDSHAKING_NONE,
    HANDSHAKING_XONXOFF,
    HANDSHAKING_RTSCTS
} E_STB_SP_HANDSHAKING;

typedef enum e_hw_status
{
    HW_OK = 0,
    HW_GEN_ERROR,
    HW_BAD_PARAM,
    HW_NO_MEMORY
} E_HW_STATUS;

//---Global type defs for public use-------------------------------------------

//---Global Function prototypes for public use---------------------------------

// Serial port functions

void STB_SPWrite(U8BIT *data, U32BIT len, U32BIT timeout, U32BIT *written);
void STB_SPRead(U8BIT *data, U32BIT len, U32BIT timeout, U32BIT *read);

/**
 * @brief   Write debug string to serial/debug port. <CR><LF> characters will be automatically
 *          added to the end of the string
 * @param   format string & format
 */
void STB_SPDebugWrite(const char *format, ... );

/**
 * @brief   Writes debug string to the serial port without <CR><LF>
 * @param   format string & format
 */
void STB_SPDebugNoCnWrite(const char *format, ... );

/**
 * @brief   Report Assertion failure
 * @param   file name of source file
 * @param   line line number of source file
 * @param   eval_str evaluation string that failed
 */
void STB_SPDebugAssertFail(const char *file, int line, const char *eval_str);

/**
 * @brief     Print hexadecimal dump of data buffer
 * @param     data Pointer to data to hex dump
 * @param     size Size of data in bytes
 */
void STB_SPDebugHexDump(U8BIT *data, U32BIT size);

/**
 * @brief   Returns the number of front end (Tuner) paths on the platform
 * @return  The number of tuner paths
 */
U8BIT STB_HWGetTunerNums(void);

/**
 * @brief   Returns the number of recordings that can take place at the same time
 * @return  The number of recordings
 */
U8BIT STB_HWGetNumRecorders(void);

/**
 * @brief   Returns the number of audio decoding paths on the platform
 * @return  The number of audio decode paths
 */
U8BIT STB_HWGetAudioDecodePaths(void);

/**
 * @brief   Returns the number of video decoding paths on the platform
 * @return  The number of video decode paths
 */
U8BIT STB_HWGetVideoDecodePaths(void);

/**
 * @brief   Queries the number of demux paths available
 * @return  The number of demux paths
 */
U8BIT STB_HWGetDemuxPaths(void);

/**
 * @brief   Returns the serial number of the Set Top Box
 * @return  The serial number
 */
U8BIT STB_HWGetNumCISlots(void);

/**
 * @brief   Returns the number of CI slots on the platform
 * @return  The number of CI slots
 */
U8BIT STB_HWGetNumSCSlots(void);

/**
 * @brief   Returns the number of smart card slots on the platform
 * @return  The number of smart card slots
 */
U8BIT* STB_HWGetOUI(void);

/**
 * @brief   Returns the platform hardware identifier code
 * @return  The platform id
 */
U16BIT STB_HWGetHwId(void);

/**
 * @brief   Returns the platform customer identifier code
 * @return  The platform id
 */
U16BIT STB_HWGetCustomerId(void);

/**
 * @brief   Returns the serial number of the Set Top Box
 * @return  The serial number
 */
U32BIT STB_HWGetBoxSerialNumber(void);

/**
 * @brief   Gives the name (optionally including path) of the SQL database file
 * @param   pathname - array into which the full path will be written
 * @param   max_pathname_len - size of the pathname array
 * @return  TRUE if the pathname is returned successfully.
 */
BOOLEAN STB_GetSqlFileName(U8BIT *pathname, U16BIT max_pathname_len);

// VBI interface
void STB_HWInitialiseVBI(void);
void STB_HWVBIInsert(U8BIT *pes_data_field, U32BIT num_bytes);

void STB_TimeConsumeDebug(const char *format, ... );

#endif //  _STBHWC_H


