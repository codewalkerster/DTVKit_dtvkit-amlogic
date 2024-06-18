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
 * If you or your organization is not a member of DTVKit then you have access
 * to this source code outside of the terms of the licence agreement
 * and you are expected to delete this and any associated files immediately.
 * Further information on DTVKit, membership and terms can be found at www.dtvkit.org
 *******************************************************************************/
/**
 * @brief   Header file - Platform Specific Hardware Definitions
 * @file    stbca.h
 * @date    October 2018
 */

#include <Aml_MP/Dvr.h>


//---Constant and macro definitions for public use-----------------------------


/*!**************************************************************************
 * @brief   This function can get from other module, to judge under M2M
 *          or not
 * @return  true under M2M, false not M2M
 ****************************************************************************/
BOOLEAN STB_CAIsM2M();

/*!**************************************************************************
 * @brief   This function is called when in timeshift state
 * @param   handle - CA descrambler handle
 * @param   On - TRUE in timeshfit, FALSE normal record or replay
 ****************************************************************************/
void STB_CASetTimeShiftOn(UINTPTR handle, BOOLEAN On);

int STB_CAPVRGetPlaySection(AML_MP_CASSESSION *sec);

int STB_CAPVRGetDvrSection(UINTPTR handle, AML_MP_CASSESSION *sec);

void STB_CAPVRRecodingEncrypt(void *handle, void *param);

void STB_CAPVRPlayDecrypt(void *handle, void *param);

//---Enumerations for public use-----------------------------------------------

//---Global type defs for public use-------------------------------------------

//---Global Function prototypes for public use---------------------------------

