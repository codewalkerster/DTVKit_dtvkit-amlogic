/*******************************************************************************
 * Copyright  © 2018 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
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
 * @brief   Set Top Box - Hardware Layer, CI functions.
 * @file    stbhwci.c
 * @date    October 2018
 */

//#define CI_DEBUG
#define CI_ERROR
//#define TRUST_CENTER
//#define SMARDTV
//#define EFINS

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* third party header files */

/* STB Header Files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwmem.h"
#include "stbhwdef.h"
//#include "stbhwci.h"
//#include "stbhwnvm.h"
//#include "stbcios.h" /*for STB_CIDebugPrintf()*/
#include "stbhwdmx.h"
#include "dtv_log.h"
#include "cam_manager.h"
#define TAG  "STBHWCI"

#ifdef INCLUDE_TEST_KEYS
#include "ciptestkeys.h"
#endif

/*---macro definitions for this file-----------------------------------------*/

#ifdef CI_DEBUG
#define CI_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x "\n",__FUNCTION__,__LINE__, ##__VA_ARGS__);
#else
#define CI_DBG(x,...)
#endif

#ifdef CI_ERROR
#define CI_ERR(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d ERROR " x "\n",__FUNCTION__,__LINE__, ##__VA_ARGS__);
#else
#define CI_ERR(x,...)
#endif

#define LINE_LEN  (16 * 3)

/*---constant definitions for this file--------------------------------------*/

/* Number of host keys is (STB_CI_KEY_CLK + 1) - i.e. 12 */
#define HOST_KEYS 12

/* ToDo: Following locations may need to be modified */
#define SECURE_NVM_PATH "/system/certs/"
#define SECURE_NVM_FNAME "/system/secure.dat"

/*---local typedef structs for this file-------------------------------------*/

typedef struct
{
   const char *filename;
   U8BIT *data;
   U32BIT size;
} S_SECURE_VARIABLE;

/*---local (static) variable declarations for this file----------------------*/

static const char *secure_nvm_fname = SECURE_NVM_FNAME;
static const char hexdigits[] = "0123456789abcdef";

static S_SECURE_VARIABLE host_keys[HOST_KEYS * 2] = {
#if defined(TRUST_CENTER)
   {"tc/ciplus_test_root_ca_R01.der", NULL, 0}, /* STB_CI_KEY_ROOT_CERT */
   {"tc/ciplus_test_brand_ca_B51.der", NULL, 0}, /* STB_CI_KEY_BRAND_CERT */
   {"tc/H30-cert.der", NULL, 0}, /* STB_CI_KEY_DEVICE_CERT */
   {"tc/prng_seed", NULL, 0}, /* STB_CI_KEY_PRNG_SEED */
   {"tc/prng_key_k", NULL, 0}, /* STB_CI_KEY_PRNG_KEY_K */
   {"tc/dh_p", NULL, 0}, /* STB_CI_KEY_DH_P */
   {"tc/dh_g", NULL, 0}, /* STB_CI_KEY_DH_G */
   {"tc/dh_q", NULL, 0}, /* STB_CI_KEY_DH_Q */
   {"tc/H30-key.der", NULL, 0}, /* STB_CI_KEY_HDQ */
   {"tc/siv", NULL, 0}, /* STB_CI_KEY_SIV */
   {"tc/slk", NULL, 0}, /* STB_CI_KEY_SLK */
   {"tc/clk", NULL, 0}, /* STB_CI_KEY_CLK */
#elif defined(SMARDTV)
   {"smardtv/ciplusRootCert.der", NULL, 0}, /* STB_CI_KEY_ROOT_CERT */
   {"smardtv/ciplusBrandACert.der", NULL, 0}, /* STB_CI_KEY_BRAND_CERT */
   {"smardtv/ciplusDevice1Cert.der", NULL, 0}, /* STB_CI_KEY_DEVICE_CERT */
   {"smardtv/prng_seed", NULL, 0}, /* STB_CI_KEY_PRNG_SEED */
   {"smardtv/prng_key_k", NULL, 0}, /* STB_CI_KEY_PRNG_KEY_K */
   {"smardtv/dh_p", NULL, 0}, /* STB_CI_KEY_DH_P */
   {"smardtv/dh_g", NULL, 0}, /* STB_CI_KEY_DH_G */
   {"smardtv/dh_q", NULL, 0}, /* STB_CI_KEY_DH_Q */
   {"smardtv/ciplusDevice1Key.der", NULL, 0}, /* STB_CI_KEY_HDQ */
   {"smardtv/siv", NULL, 0}, /* STB_CI_KEY_SIV */
   {"smardtv/slk", NULL, 0}, /* STB_CI_KEY_SLK */
   {"smardtv/clk", NULL, 0}, /* STB_CI_KEY_CLK */
#elif defined(EFINS)
   {"efins/ciplus_test_root_cert.pem", NULL, 0}, /* STB_CI_KEY_ROOT_CERT */
   {"efins/ciplus_test_b51_cert.pem", NULL, 0}, /* STB_CI_KEY_BRAND_CERT */
   {"efins/B0M0DR0-cer.pem", NULL, 0}, /* STB_CI_KEY_DEVICE_CERT */
   {"efins/prng_seed", NULL, 0}, /* STB_CI_KEY_PRNG_SEED */
   {"efins/prng_key_k", NULL, 0}, /* STB_CI_KEY_PRNG_KEY_K */
   {"efins/dh_p", NULL, 0}, /* STB_CI_KEY_DH_P */
   {"efins/dh_g", NULL, 0}, /* STB_CI_KEY_DH_G */
   {"efins/dh_q", NULL, 0}, /* STB_CI_KEY_DH_Q */
   {"efins/B0M0DR0-key.der", NULL, 0}, /* STB_CI_KEY_HDQ */
   {"efins/siv", NULL, 0}, /* STB_CI_KEY_SIV */
   {"efins/slk", NULL, 0}, /* STB_CI_KEY_SLK */
   {"efins/clk", NULL, 0}, /* STB_CI_KEY_CLK */
#else /*softlinks*/
   {"any/root.crt", NULL, 0}, /* STB_CI_KEY_ROOT_CERT */
   {"any/brand.crt", NULL, 0}, /* STB_CI_KEY_BRAND_CERT */
   {"any/device.crt", NULL, 0}, /* STB_CI_KEY_DEVICE_CERT */
   {"any/prng_seed", NULL, 0}, /* STB_CI_KEY_PRNG_SEED */
   {"any/prng_key_k", NULL, 0}, /* STB_CI_KEY_PRNG_KEY_K */
   {"any/dh_p", NULL, 0}, /* STB_CI_KEY_DH_P */
   {"any/dh_g", NULL, 0}, /* STB_CI_KEY_DH_G */
   {"any/dh_q", NULL, 0}, /* STB_CI_KEY_DH_Q */
   {"any/device.key", NULL, 0}, /* STB_CI_KEY_HDQ */
   {"any/siv", NULL, 0}, /* STB_CI_KEY_SIV */
   {"any/slk", NULL, 0}, /* STB_CI_KEY_SLK */
   {"any/clk", NULL, 0}, /* STB_CI_KEY_CLK */
#endif
#ifdef EFINS
   {"efins/ROT2_R01.der", NULL, 0}, /* STB_CI_KEY_ROOT_CERT */
   {"efins/ROT2_B51.der", NULL, 0}, /* STB_CI_KEY_BRAND_CERT */
   {"efins/ECP_B0M0DU0-cer.der", NULL, 0}, /* STB_CI_KEY_DEVICE_CERT */
   {"efins/prng_seed", NULL, 0}, /* STB_CI_KEY_PRNG_SEED */
   {"efins/prng_key_k", NULL, 0}, /* STB_CI_KEY_PRNG_KEY_K */
   {"efins/dh_p", NULL, 0}, /* STB_CI_KEY_DH_P */
   {"efins/dh_g", NULL, 0}, /* STB_CI_KEY_DH_G */
   {"efins/dh_q", NULL, 0}, /* STB_CI_KEY_DH_Q */
   {"efins/ECP_B0M0DU0-key.der", NULL, 0}, /* STB_CI_KEY_HDQ */
   {"efins/siv", NULL, 0}, /* STB_CI_KEY_SIV */
   {"efins/slk", NULL, 0}, /* STB_CI_KEY_SLK */
   {"efins/clk", NULL, 0} /* STB_CI_KEY_CLK */
#else
   {"any2/root2.crt", NULL, 0}, /* STB_CI_KEY_ROOT_CERT */
   {"any2/brand2.crt", NULL, 0}, /* STB_CI_KEY_BRAND_CERT */
   {"any2/device2.crt", NULL, 0}, /* STB_CI_KEY_DEVICE_CERT */
   {"any2/prng_seed", NULL, 0}, /* STB_CI_KEY_PRNG_SEED */
   {"any2/prng_key_k", NULL, 0}, /* STB_CI_KEY_PRNG_KEY_K */
   {"any2/dh_p", NULL, 0}, /* STB_CI_KEY_DH_P */
   {"any2/dh_g", NULL, 0}, /* STB_CI_KEY_DH_G */
   {"any2/dh_q", NULL, 0}, /* STB_CI_KEY_DH_Q */
   {"any2/device2.key", NULL, 0}, /* STB_CI_KEY_HDQ */
   {"any2/siv", NULL, 0}, /* STB_CI_KEY_SIV */
   {"any2/slk", NULL, 0}, /* STB_CI_KEY_SLK */
   {"any2/clk", NULL, 0}, /* STB_CI_KEY_CLK */
#endif
};

/*---local function prototypes for this file---------------------------------*/

static BOOLEAN ReadSecureFile(S_SECURE_VARIABLE *var);
#if 0
static void DebugPrintBuffer(U8BIT *buff, U32BIT len);
#endif

/*---global function definitions----------------------------------------------*/

/**
 * @brief   Return number of CI slots
 * @note    When supporting USB CAMs, this function returns
 *          the same value as STB_CIUsbCamTotal().
 * @return  Number of CI slots on the receiver
 */
U8BIT STB_CIGetSlotCount(void)
{
   return NUM_CI_SLOTS;
}

/**
 * @brief   Puts CI control into standby mode (power off)
 */
void STB_CIStandbyOn(void)
{
   FUNCTION_START(STB_CIStandbyOn);
   FUNCTION_FINISH(STB_CIStandbyOn);
}

/**
 * @brief   Brings CI out of standby mode (power on)
 */
void STB_CIStandbyOff(void)
{
   FUNCTION_START(STB_CIStandbyOff);
   FUNCTION_FINISH(STB_CIStandbyOff);
}

/**
 * @brief   Sets up routing between a tuner and slot
 * @param   tuner - tuner to be routed
 * @param   slot_id - slot to apply routing to
 * @param   pass_through - TRUE if the TS should be routed through the slot, FALSE otherwise
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_CIRouteTS(U8BIT tuner, U8BIT slot_id, BOOLEAN pass_through)
{
    USE_UNWANTED_PARAM(tuner);

    FUNCTION_START(STB_CIRouteTS);

    STB_SPDebugWrite("STB_CIRouteTS(%u, %u, %u)", tuner, slot_id, pass_through);

    if(TRUE == pass_through)
    {
        CAM_SetTsStatus(slot_id, 1);
      //   STB_DMXChangeAllDemuxSource(slot_id, 1);
        STB_SPDebugWrite("======>TS change to passthough");
    }
    else
    {
        CAM_SetTsStatus(slot_id, 0);
      //   STB_DMXChangeAllDemuxSource(slot_id, 0);
        STB_SPDebugWrite("======>TS change to bypass");
    }
    FUNCTION_FINISH(STB_CIRouteTS);

    return(TRUE);
}
#if 0
/**
 * @brief   Return CI+ host key
 * @param   type type of host key
 * @param   key pointer to the key data
 * @param   length number of bytes in key data
 * @note    The pointer must remain valid while the CI+ stack is running
 * @param   slot_id Zero-based CI slot identifier (0, 1, ...)
 */
void STB_CIGetHostKey(E_STB_CI_KEY_TYPE type, U8BIT **key, U16BIT *length)
{
   S_SECURE_VARIABLE *var;

   FUNCTION_START(STB_CIGetHostKey);

   CI_DBG("type=%u pKey=%p pLen=%p", type, key, length)
   if (type < HOST_KEYS)
   {
	  STB_SPDebugWrite("get host key, type < HOST_KEYS, %d", type);
      var = &(host_keys[type]);
      if (ReadSecureFile(var))
      {
	  	STB_SPDebugWrite("Using ReadSecureFile keys.");
         *key = var->data;
         *length = var->size;
      }
      else
      {
      #ifdef INCLUDE_TEST_KEYS
	  STB_SPDebugWrite("Using test keys.");
         *key = g_citest_keys[type].data;
         *length = g_citest_keys[type].size;
      #else
         *key = NULL;
         *length = 0;
      #endif
      }
      CI_DBG("key=%p len=%u", *key, *length)
      //DebugPrintBuffer(*key, *length);
   }
   else if (type >= STB_CI_ECP_KEY_ROOT_CERT && type < (STB_CI_ECP_KEY_ROOT_CERT+HOST_KEYS))
   {
   	STB_SPDebugWrite("get host key, type > HOST_KEYS, %d", type);
      var = &(host_keys[type+HOST_KEYS-STB_CI_ECP_KEY_ROOT_CERT]);
      if (ReadSecureFile(var))
      {
         *key = var->data;
         *length = var->size;
      }
      else
      {
         *key = NULL;
         *length = 0;
      }
      CI_DBG("key=%p len=%u", *key, *length)
   }
   else
   {
   	STB_SPDebugWrite("get host key, type unknown");
      CI_ERR("unknown type %u", type)
      *key = NULL;
      *length = 0;
   }

   FUNCTION_FINISH(STB_CIGetHostKey);
}
#endif
/**
 * @brief   Read data from secure non-volatile area
 * @param   buffer pointer to data buffer to read into
 * @param   len number of bytes to read
 * @return  TRUE if read operation was successful, FALSE otherwise
 */
BOOLEAN STB_CIReadSecureNVM(U8BIT *buffer, U32BIT len)
{
   BOOLEAN retval;
   FILE *f;
   U32BIT flen;

   FUNCTION_START(STB_CIReadSecureNVM);

   f = fopen(secure_nvm_fname, "r");
   if (f != NULL)
   {
      fseek(f, 0, SEEK_END);
      flen = ftell(f);
      fseek(f, 0, SEEK_SET);
      if (flen != len)
      {
         CI_ERR("length (%u) does not match file: %s", len, secure_nvm_fname)
         retval = FALSE;
      }
      else
      {
         flen = fread(buffer, 1, len, f);
         if (flen != len)
         {
            CI_ERR("Read %u bytes but len required %u", flen, len)
            retval = FALSE;
         }
         else
         {
            retval = TRUE;
         }
      }
      fclose(f);
   }
   else
   {
      retval = FALSE;
   }

   FUNCTION_FINISH(STB_CIReadSecureNVM);

   return retval;
}

/**
 * @brief   Write data into secure non-volatile area
 * @param   buffer pointer to data buffer to write
 * @param   len number of bytes to write
 * @return  TRUE if read operation was successful, FALSE otherwise
 */
BOOLEAN STB_CIWriteSecureNVM(U8BIT *buffer, U32BIT len)
{
   BOOLEAN retval = FALSE;
   FILE *f;

   FUNCTION_START(STB_CIWriteSecureNVM);

   f = fopen(secure_nvm_fname, "r+");
   if (f == NULL)
   {
      f = fopen(secure_nvm_fname, "w");
   }

   if (f != NULL)
   {
      if (fwrite(buffer, 1, len, f) == len)
      {
         CI_DBG("Wrote %u bytes from '%s'", len, secure_nvm_fname)
         retval = TRUE;
      }
      else
      {
         CI_ERR("failed to write %u byhtes to '%s'", len, secure_nvm_fname)
      }
      fclose(f);
   }
   else
   {
      CI_ERR("file not opened '%s'", secure_nvm_fname)
   }

   FUNCTION_FINISH(STB_CIWriteSecureNVM);

   return retval;
}

/**
 * @brief   Write debug string to output
 * @param   format string & format
 */
void STB_CIDebugPrintf(const char *format, ... )
{
   static char debug_msg_buff[512];
   va_list vparams;

   FUNCTION_START(STB_SPDebugNoCnWrite);

   ASSERT(format != NULL);

   va_start(vparams, format);
   vsnprintf(debug_msg_buff, sizeof(debug_msg_buff), format, vparams);
   va_end(vparams);

   //STB_SPDebugWrite("%s", debug_msg_buff);
   DTV_LOG(ANDROID_LOG_INFO, "CI-Plus", debug_msg_buff);
   //fflush(stdout);

   FUNCTION_FINISH(STB_SPDebugNoCnWrite);
}

/*---local function definitions----------------------------------------------*/

static BOOLEAN IsSymlink(const char *filename)
{
   struct stat st;
   /* if lstat errors, assume it is a symlink */
   return (lstat(filename, &st) == 0 && !S_ISLNK(st.st_mode))? FALSE : TRUE;
}

/* Local PROTOTYPE Declarations */
/**
 * @brief   Decode base64 encoded key and store the result.
 *          The data is modified to point to the position after the key.
 * @param   data Buffer data to decode
 * @param   key Decoded key
 * @param   len Key length in bytes
 * @return  TRUE if the key was decoded, FALSE otherwise
 */
static void DecodeKey(U8BIT *data, U8BIT **key, U32BIT *len)
{
   static char *base64 =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/=";
   char *p = (char *)data;
   int symbols, bytes, i, j;
   int b0, b1, b2, b3;

   symbols = strspn(p, base64);
   if (symbols > 0 && symbols % 4 == 0)
   {
      bytes = symbols * 3 / 4;
      if (p[symbols - 1] == '=')
      {
         --bytes;
      }
      if (p[symbols - 2] == '=')
      {
         --bytes;
      }

      *key = STB_MEMGetSysRAM(bytes);
      if ((*key) != NULL)
      {
         *len = bytes;
         for (i = 0, j = 0; i < symbols; i += 4)
         {
            b0 = strchr(base64, *p++) - base64;
            b1 = strchr(base64, *p++) - base64;
            b2 = strchr(base64, *p++) - base64;
            b3 = strchr(base64, *p++) - base64;

            (*key)[j++] = b0 << 2 | b1 >> 4;
            if (b2 < 64)
            {
               (*key)[j++] = b1 << 4 | b2 >> 2;
               if (b3 < 64)
               {
                  (*key)[j++] = b2 << 6 | b3;
               }
            }
         }
      }
      else
      {
         *len = 0;
      }
   }
   else
   {
      *key = NULL;
      *len = 0;
   }
}

static void ReadDataFile(FILE *f, S_SECURE_VARIABLE *var)
{
   U8BIT *data = var->data;
   char *line = (char *)data;
   size_t len = var->size;
   size_t read, prev_read;
   ssize_t sread;

   FUNCTION_START(ReadDataFile);

   read = fread(data, 1, 8, f);
   if (read < 8)
   {
      CI_ERR("read %u sz=%u", read, var->size)
      var->size = read;
   }
   else if (memcmp(data, "-----BEG", 8) == 0)
   {/* Base64 encoded certificate */
      /* Skip first line */
      getline(&line, &len, f);
      while ((sread = getline(&line, &len, f)) != -1)
      {
         line += sread - 1;
         len -= sread - 1;
         prev_read = sread;
      }

      /* Ignore last line */
      len = (U8BIT*)line - data - prev_read + 1;
      data[len] = '\0';

      /* Base64 string into data. Now decode it. */
      DecodeKey(data, &var->data, &var->size);

      CI_DBG("free data %p (var %p)", data, var) // this print does not appear
      /* Free tmp memory */
      STB_MEMFreeSysRAM(data);
   }
   else
   {/* Raw certificate with more than 8 bytes */
      //fseek(f, 0, SEEK_SET);
      len -= 8;
      read = fread(data + 8, 1, len, f);
      if (read != len)
      {
         CI_ERR("read %u sz=%u", read, len)
         var->size = read + 8;
      }
      else
      {
         CI_DBG("read data %p (var %p)", data, var)
      }
   }

   FUNCTION_FINISH(ReadDataFile);
}

static BOOLEAN ReadSecureFile(S_SECURE_VARIABLE *var)
{
   char fname[96];
   FILE *f;
   S32BIT len;

   FUNCTION_START(ReadSecureFile);

   strcpy(fname, SECURE_NVM_PATH);

   if((strlen(SECURE_NVM_PATH) + strlen(var->filename)) >= sizeof(fname))
   {
      CI_DBG("file name too long")
      return FALSE;
   }

   strcat(fname, var->filename);
   if (var->size != 0 && !IsSymlink(fname))
   {
      ASSERT(var->data != NULL)
      CI_DBG("already have data for %s sz=%u", fname, var->size)
      len = var->size;
   }
   else
   {
      var->size = 0;
      if (var->data != NULL)
      {
         /* data came from symlinked  file */
         CI_DBG("free data %p (var %p)", var->data, var) // this print does not appear
         STB_MEMFreeSysRAM(var->data);
         var->data = NULL;
      }
      f = fopen(fname, "r");
      if (f == NULL)
      {
         CI_DBG("failed to open '%s': %s", fname, strerror(errno))
         len = 0;
      }
      else
      {
         fseek(f, 0, SEEK_END);
         len = ftell(f);
         if(len < 0)
         {
            CI_DBG("file %s ftell length error", fname)
            fclose(f);
            return FALSE;
         }

         fseek(f, 0, SEEK_SET);
         if (len == 0)
         {
            CI_DBG("file %s length zero", fname)
         }
         else
         {
            var->data = STB_MEMGetSysRAM(len);
            if (var->data != NULL)
            {
               CI_DBG("file %s length %u data %p (var %p)", fname, len, var->data, var)
               var->size = len;
               ReadDataFile(f, var);
            }
            else
            {
               CI_ERR("mem failed")
               len = 0;
            }
         }
         fclose(f);
      }
   }

   FUNCTION_FINISH(ReadSecureFile);
   return (len != 0)? TRUE : FALSE;
}

void TST_CIGetDateTime(U16BIT *mjd, U8BIT *hour, U8BIT *minute, U8BIT *second, S16BIT *offset)
{
   FUNCTION_START(TST_CIGetDateTime);
#if defined(TRUST_CENTER)
   // Set to 19 Dec 2009, 6am
   *mjd = 55184;
   *hour = 6;
   *minute = 0;
   *second = 0;
   *offset = 0;
#elif defined(SMARDTV)
   // Set to 19 Dec 2008, 6am
   *mjd = 54819;
   *hour = 6;
   *minute = 0;
   *second = 0;
   *offset = 0;
#elif defined(EFINS)
   // Set to 19 Dec 2009, 6am
   *mjd = 55184;
   *hour = 6;
   *minute = 0;
   *second = 0;
   *offset = 0;
#else
   // Set to 10 Dec 2017, 10am
   *mjd = 58097;
   *hour = 6;
   *minute = 0;
   *second = 0;
   *offset = 0;
#endif
   CI_DBG(": mjd %u, hour %u, min %u\n", *mjd, *hour, *minute);
   FUNCTION_FINISH(TST_CIGetDateTime);
}

#if 0
static void DebugPrintBuffer(U8BIT *buff, U32BIT len)
{
   char printline[LINE_LEN + 2];
   U32BIT ii, jj;
   printline[LINE_LEN] = '\n';
   printline[LINE_LEN + 1] = '\0';
   for (ii = 0, jj = 0; jj != len; ++jj)
   {
      printline[ii++] = ' ';
      printline[ii++] = hexdigits[(buff[jj] >> 4) & 0xF];
      printline[ii++] = hexdigits[buff[jj] & 0xF];
      if (ii == LINE_LEN)
      {
         STB_CIDebugPrintf("%s", printline);
         ii = 0;
      }
   }
   if (ii != LINE_LEN)
   {
      printline[ii++] = '\n';
      printline[ii] = '\0';
      STB_CIDebugPrintf("%s", printline);
   }
}
#endif //0
