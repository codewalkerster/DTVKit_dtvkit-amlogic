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
 * @brief   Set Top Box - Hardware Layer, Encryption / decryption functions
 * @file    stbhwcrypt.c
 * @date    October 2018
 */

/*#define CRYPT_DEBUG*/

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */

/* third party header files */

/* DVBCore header files */
#include <techtype.h>
#include <dbgfuncs.h>
#include "dtv_log.h"
#define TAG  "STBHWCRYPT"

/*---constant definitions for this file--------------------------------------*/
#undef DBG
#ifdef CRYPT_DEBUG
   #define DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define DBG(x,...)
#endif

/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/

/*---local function prototypes for this file---------------------------------*/

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialises encryption/decryption
 * @return  TRUE if initialisation is successful, FALSE otherwise
 */
BOOLEAN STB_CRYPTInitialise(void)
{
   FUNCTION_START(STB_CRYPTInitialise);
   FUNCTION_FINISH(STB_CRYPTInitialise);
   return FALSE;
}

/**
 * @brief   Apply AES-128 encryption on the buffer using the given key.
 *          If the length of the buffer is not a multiple of 128 bits the last
 *          incomplete block will not be encrypted.
 * @param   src buffer to be encrypted
 * @param   dest buffer to store encrypted data
 * @param   length length of buffer in bytes
 * @param   key AES key (16 bytes - 128 bits)
 * @note    src and dest may point to the same buffer
 */
void STB_CRYPTAesEncrypt(U8BIT *src, U8BIT *dest, U32BIT length, U8BIT *key)
{
   FUNCTION_START(STB_CRYPTAesEncrypt);
   USE_UNWANTED_PARAM(src);
   USE_UNWANTED_PARAM(dest);
   USE_UNWANTED_PARAM(length);
   USE_UNWANTED_PARAM(key);
   FUNCTION_FINISH(STB_CRYPTAesEncrypt);
}

/**
 * @brief   Apply AES-128 decryption on the buffer using the given key.
 *          If the length of the buffer is not a multiple of 128 bits the last
 *          incomplete block will not be decrypted.
 * @param   src buffer to be decrypted
 * @param   dest buffer to store decrypted data
 * @param   length length of buffer in bytes
 * @param   key AES key (16 bytes - 128 bits)
 * @note    src and dest may point to the same buffer
 */
void STB_CRYPTAesDecrypt(U8BIT *src, U8BIT *dest, U32BIT length, U8BIT *key)
{
   FUNCTION_START(STB_CRYPTAesDecrypt);
   USE_UNWANTED_PARAM(src);
   USE_UNWANTED_PARAM(dest);
   USE_UNWANTED_PARAM(length);
   USE_UNWANTED_PARAM(key);
   FUNCTION_FINISH(STB_CRYPTAesDecrypt);
}

/**
 * @brief   Apply AES-128-CBC encryption on a buffer. The result of the encryption is stored in the
 *          input buffer.
 * @param   src buffer to be encrypted
 * @param   dest buffer to store encrypted data
 * @param   length length of buffer in bytes
 * @param   key AES key (128 bits = 16 bytes)
 * @param   iv AES initialisation vector (128 bits = 16 bytes)
 */
void STB_CRYPTAesCbcEncrypt(U8BIT *src, U8BIT *dest, U32BIT length, U8BIT *key, U8BIT *iv)
{
   FUNCTION_START(STB_CRYPTAesCbcEncrypt);
   USE_UNWANTED_PARAM(src);
   USE_UNWANTED_PARAM(dest);
   USE_UNWANTED_PARAM(length);
   USE_UNWANTED_PARAM(key);
   USE_UNWANTED_PARAM(iv);
   FUNCTION_FINISH(STB_CRYPTAesCbcEncrypt);
}

/**
 * @brief   Apply AES-128 decryption on a buffer. The result of the decryption is stored in the
 *          input buffer.
 * @param   src buffer to be decrypted
 * @param   dest buffer to store decrypted data
 * @param   length length of buffer in bytes
 * @param   key AES key (128 bits = 16 bytes)
 * @param   iv AES initialisation vector (128 bits = 16 bytes)
 */
void STB_CRYPTAesCbcDecrypt(U8BIT *src, U8BIT *dest, U32BIT length, U8BIT *key, U8BIT *iv)
{
   FUNCTION_START(STB_CRYPTAesCbcDecrypt);
   USE_UNWANTED_PARAM(src);
   USE_UNWANTED_PARAM(dest);
   USE_UNWANTED_PARAM(length);
   USE_UNWANTED_PARAM(key);
   USE_UNWANTED_PARAM(iv);
   FUNCTION_FINISH(STB_CRYPTAesCbcDecrypt);
}

/*---local function definitions----------------------------------------------*/


