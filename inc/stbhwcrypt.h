/*******************************************************************************
 * Copyright © 2014 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * Copyright © 2011 Ocean Blue Software Ltd
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
 * @brief   Encryption / decryption
 * @file    stbhwencryption.h
 * @date    27/01/2011
 * @author  Steve Ford
 */

/* pre-processor mechanism so multiple inclusions don't cause compilation error*/
#ifndef _STBHWCRYPT_H
#define _STBHWCRYPT_H

#include "techtype.h"

/*---Constant and macro definitions for public use-----------------------------*/
#define AES128_KEY_SIZE    16

/*---Enumerations for public use-----------------------------------------------*/

/*---Global type defs for public use-------------------------------------------*/

/*---Global Function prototypes for public use---------------------------------*/

/**
 * @brief   Initialises encryption/decryption
 * @return  TRUE if initialisation is successful, FALSE otherwise
 */
BOOLEAN STB_CRYPTInitialise(void);

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
void STB_CRYPTAesEncrypt(U8BIT *src, U8BIT *dest, U32BIT length, U8BIT *key);

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
void STB_CRYPTAesDecrypt(U8BIT *src, U8BIT *dest, U32BIT length, U8BIT *key);

/**
 * @brief   Apply AES-128-CBC encryption on a buffer. The result of the encryption is stored in the
 *          input buffer.
 * @param   src buffer to be encrypted
 * @param   dest buffer to store encrypted data
 * @param   length length of buffer in bytes
 * @param   key AES key (128 bits = 16 bytes)
 * @param   iv AES initialisation vector (128 bits = 16 bytes)
 */
void STB_CRYPTAesCbcEncrypt(U8BIT *src, U8BIT *dest, U32BIT length, U8BIT *key, U8BIT *iv);

/**
 * @brief   Apply AES-128 decryption on a buffer. The result of the decryption is stored in the
 *          input buffer.
 * @param   src buffer to be decrypted
 * @param   dest buffer to store decrypted data
 * @param   length length of buffer in bytes
 * @param   key AES key (128 bits = 16 bytes)
 * @param   iv AES initialisation vector (128 bits = 16 bytes)
 */
void STB_CRYPTAesCbcDecrypt(U8BIT *src, U8BIT *dest, U32BIT length, U8BIT *key, U8BIT *iv);

#endif /* _STBHWCRYPT_H */

