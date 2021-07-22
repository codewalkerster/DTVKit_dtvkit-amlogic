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
 * @brief   Header file - Internal functions and definitions
 * @file    internal.h
 * @date    October 2018
 */

#ifndef _INTERNAL_H
#define _INTERNAL_H

//---Constant and macro definitions for public use-----------------------------

//---Enumerations for public use-----------------------------------------------

//---Global type defs for public use-------------------------------------------

//---Global Function prototypes for public use---------------------------------

/**
 * @brief   Internal function that returns the decode PIDs for the given demux
 * @param   path demux path
 * @param   pcr_pid pointer for returned PCR PID value
 * @param   video_pid pointer for returned video PID value
 * @param   audio_pid pointer for returned audio PID value
 * @param   ad_pid pointer for returned AD PID value
 * @param   preselection_id pointer for returned preselection PID value
 * @return  TRUE if demux is valid and PIDs are returned, FALSE otherwise
 */
BOOLEAN DMXGetDecodePIDs(U8BIT path, U16BIT *pcr_pid, U16BIT *video_pid, U16BIT *audio_pid,
   U16BIT *ad_pid, U8BIT *preselection_id);

/**
 * @brief   Internal function that sets the AV path into TS injection mode
 *          for AV streaming
 * @param   path AV path
 * @return  TRUE if AV was configured for injection, FALSE otherwise
 */
BOOLEAN AV_StartInjection(U8BIT path);

/**
 * @brief   Internal function that disables TS injection for the specified
 *          AV path
 * @param   path AV path
 * @return  TRUE if injection was disabled, FALSE otherwise
 */
BOOLEAN AV_StopInjection(U8BIT path);

/**
 * @brief   Internal function to pass data to the AV decoder for injection
 * @param   path AV path
 * @param   data TS data to be injected
 * @param   size Size of data in bytes
 * @return  void
 */
void AV_InjectData(U8BIT path, U8BIT *data, U32BIT size);

#endif
