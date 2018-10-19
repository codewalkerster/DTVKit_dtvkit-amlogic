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
 * @brief   Set Top Box - Hardware Layer, Memory Management
 * @file    stbhwmem.c
 * @date    October 2018
 */

/*#define  HEAP_DEBUG*/
/*#define  NVM_DEBUG*/
/*#define  SECURE_DEBUG*/

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */
#include <malloc.h>

/* third party header files */

/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"

#include "stbhwc.h"
#include "stbhwmem.h"

/*---macro definitions for this file-----------------------------------------*/
#ifdef  HEAP_DEBUG
#define  HEAP_DBG(x,...)      STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  HEAP_DBG(x,...)
#endif

#ifdef  NVM_DEBUG
#define  NVM_DBG(x,...)       STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  NVM_DBG(x,...)
#endif

#ifdef  SECURE_DEBUG
#define  SEC_DBG(x,...)       STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  SEC_DBG(x,...)
#endif

/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/

/*---local function prototypes for this file---------------------------------*/

/*---local function definitions----------------------------------------------*/

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialises the heap
 */
void  STB_MEMInitialiseRAM(void)
{
   FUNCTION_START(STB_MEMInitialiseRAM);
   FUNCTION_FINISH(STB_MEMInitialiseRAM);
}

/**
 * @brief   Allocates a new block of memory for system use
 * @param   bytes Size of required block in bytes
 * @return  A pointer to the allocated block of memory, or NULL on failure
 */
void* STB_MEMGetSysRAM(U32BIT bytes)
{
   void *retval;

   FUNCTION_START(STB_MEMGetSysRAM);

   retval = NULL;

   if (bytes > 0)
   {
      retval = malloc((size_t)bytes);
      if (retval == NULL)
      {
         HEAP_DBG("*** OUT OF MEMORY (%lu bytes)", bytes);
      }
   }

   FUNCTION_FINISH(STB_MEMGetSysRAM);

   return(retval);
}

/**
 * @brief   Changes the size of the given block of memory
 * @param   ptr pointer to memory already allocated from the system heap
 * @param   new_num_bytes size of the memory block to be returned
 * @return  Address of new block of memory
 */
void* STB_MEMResizeSysRAM(void *ptr, U32BIT new_num_bytes)
{
   void *new_ptr;

   FUNCTION_START(STB_MEMResizeSysRAM);

   new_ptr = NULL;

   if ((ptr != NULL) && (new_num_bytes > 0))
   {
      new_ptr = realloc(ptr, new_num_bytes);
      if (new_ptr == NULL)
      {
         HEAP_DBG("Failed to reallocate %u bytes for %p", new_num_bytes, ptr);
      }
   }

   FUNCTION_FINISH(STB_MEMResizeSysRAM);

   return(new_ptr);
}

/**
 * @brief   Releases a previously allocated block of system memory
 * @param   block_ptr address of block to be released
 */
void STB_MEMFreeSysRAM(void *block_ptr)
{
   FUNCTION_START(STB_MEMFreeSysRAM);

   if (block_ptr != NULL)
   {
      free(block_ptr);
   }

   FUNCTION_FINISH(STB_MEMFreeSysRAM);
}

/**
 * @brief   Returns the amount of available system memory consumed
 * @return  Memory used as a percentage of available
 */
U8BIT STB_MEMSysRAMUsed(void)
{
   U8BIT used_result = 0;

   FUNCTION_START(STB_MEMSysRAMUsed);

   FUNCTION_FINISH(STB_MEMSysRAMUsed);

   return used_result;
}

/**
 * @brief   Allocates a new block of memory for application use
 * @param   bytes Size of required block in bytes
 * @return  A pointer to the allocated block of memory
 * @return  NULL allocation failed
 */
void* STB_MEMGetAppRAM(U32BIT bytes)
{
   void *retval;

   FUNCTION_START(STB_MEMGetAppRAM);

   retval = NULL;

   if (bytes > 0)
   {
      retval = malloc((size_t)bytes);
      if (retval == NULL)
      {
         HEAP_DBG("*** OUT OF MEMORY (%lu bytes)", bytes);
      }
   }

   FUNCTION_FINISH(STB_MEMGetAppRAM);

   return(retval);
}

/**
 * @brief   Changes the size of the given block of memory
 * @param   ptr pointer to memory already allocated from the app heap
 * @param   new_num_bytes size of the memory block to be returned
 * @return  Address of new block of memory
 */
void* STB_MEMResizeAppRAM(void *ptr, U32BIT new_num_bytes)
{
   void *new_ptr;

   FUNCTION_START(STB_MEMResizeAppRAM);

   new_ptr = NULL;

   if ((ptr != NULL) && (new_num_bytes > 0))
   {
      new_ptr = realloc(ptr, new_num_bytes);
      if (new_ptr == NULL)
      {
         HEAP_DBG("Failed to reallocate %u bytes for %p", new_num_bytes, ptr);
      }
   }

   FUNCTION_FINISH(STB_MEMResizeAppRAM);

   return(new_ptr);
}

/**
 * @brief   Releases a previously allocated block of system memory
 * @param   block_ptr address of block to be released
 */
void STB_MEMFreeAppRAM(void *block_ptr)
{
   FUNCTION_START(STB_MEMFreeAppRAM);

   if (block_ptr != NULL)
   {
      free(block_ptr);
   }

   FUNCTION_FINISH(STB_MEMFreeAppRAM);
}

/**
 * @brief   Returns the amount of available application memory consumed
 * @return  Memory used as a percentage of available
 * @todo     Add STB810 code
 */
U8BIT STB_MEMAppRAMUsed(void)
{
   U8BIT used_result;

   FUNCTION_START(STB_MEMFreeAppRAM);

   used_result = 0;

   FUNCTION_FINISH(STB_MEMFreeAppRAM);

   return used_result;
}

/**
 * @brief   Initialises Non-Volatile memory access
 * For a new NVM device, this function formats it ready for use, otherwise
 * it prepares existing data for access
 */
void STB_MEMInitialiseNVM(void)
{
   FUNCTION_START(STB_MEMInitialiseNVM);
   FUNCTION_FINISH(STB_MEMInitialiseNVM);
}

/**
 * @brief   Read data from the NVM
 * @param   addr The NVM start address
 * @param   bytes The number of bytes to read
 * @param   dst_addr Callers buffer for the data
 * @return  TRUE if successful
 * @return  FALSE if there was a problem
 */
BOOLEAN STB_MEMReadNVM(U32BIT addr, U32BIT bytes, U8BIT *dst_addr)
{
   FUNCTION_START(STB_MEMReadNVM);
   USE_UNWANTED_PARAM(addr);
   USE_UNWANTED_PARAM(bytes);
   USE_UNWANTED_PARAM(dst_addr);
   FUNCTION_FINISH(STB_MEMReadNVM);

   return FALSE;
}

/**
 * @brief   Writes data to the NVM
 * @param   addr The NVM start address to write to
 * @param   bytes The number of bytes to be written
 * @param   src_addr Pointer to the data to be written
 * @return  TRUE if successful
 * @return  FALSE if there was a problem
 */
BOOLEAN STB_MEMWriteNVM(U32BIT addr, U32BIT bytes, U8BIT *src_addr)
{
   FUNCTION_START(STB_MEMWriteNVM);
   USE_UNWANTED_PARAM(addr);
   USE_UNWANTED_PARAM(bytes);
   USE_UNWANTED_PARAM(src_addr);
   FUNCTION_FINISH(STB_MEMWriteNVM);

   return FALSE;
}

/**
 * @brief   Returns the size (capacity) of the NVM
 * @return  Size of the NVM in bytes
 */
U32BIT STB_MEMGetNVMSize(void)
{
   FUNCTION_START(STB_MEMGetNVMSize);
   FUNCTION_FINISH(STB_MEMGetNVMSize);
   return(0);
}

/**
 * @brief   Returns any offset required in NVM to avoid private data
 * @return  The offset in bytes
 */
U32BIT STB_MEMGetNVMOffset(void)
{
   FUNCTION_START(STB_MEMGetNVMOffset);
   FUNCTION_FINISH(STB_MEMGetNVMOffset);
   return(0);
}

/**
 * @brief   Returns the word alignment size of the NVM device
 * @return  Alignment in bytes
 */
U8BIT STB_MEMGetNVMAlign(void)
{
   FUNCTION_START(STB_MEMGetNVMAlign);
   FUNCTION_FINISH(STB_MEMGetNVMAlign);
   return(0);
}

/**
 * @brief   Read variable defined by the given key from secure NVM
 * @param   key data item to be read
 * @param   len returned length of data read
 * @return  Pointer to data read, or NULL on failure.
 * @note    The data must be released using STB_MEMReleaseSecureVariable
 */
void* STB_MEMReadSecureVariable(U8BIT key, U32BIT *len)
{
   FUNCTION_START(STB_MEMReadSecureVariable);
   USE_UNWANTED_PARAM(key);
   USE_UNWANTED_PARAM(len);
   FUNCTION_FINISH(STB_MEMReadSecureVariable);

   return(NULL);
}

/**
 * @brief   Releas the data returned by STB_MEMReadSecureVariable
 * @param   value pointer to data to be released
 */
void STB_MEMReleaseSecureVariable(void *value)
{
   FUNCTION_START(STB_MEMReleaseSecureVariable);
   USE_UNWANTED_PARAM(value);
   FUNCTION_FINISH(STB_MEMReleaseSecureVariable);
}

/**
 * @brief   Writes data defined by the given key to secure NVM
 * @param   key data item to be written
 * @param   value pointer to data to be written
 * @param   len data size
 * @return  TRUE if the data is written successfully, FALSE otherwise
 */
BOOLEAN STB_MEMWriteSecureVariable(U8BIT key, void *value, U32BIT len)
{
   FUNCTION_START(STB_MEMWriteSecureVariable);
   USE_UNWANTED_PARAM(key);
   USE_UNWANTED_PARAM(value);
   USE_UNWANTED_PARAM(len);
   FUNCTION_FINISH(STB_MEMWriteSecureVariable);

   return FALSE;
}

/**
 * @brief   Read constant defined by the given key from secure NVM
 * @param   key data item to be read
 * @param   len returned length of data read
 * @return  Pointer to data read, or NULL on failure.
 * @note    The data must not be released (it is managed by the platform layer)
 */
const void* STB_MEMReadSecureConstant(U8BIT key, U32BIT *len)
{
   FUNCTION_START(STB_MEMReadSecureConstant);
   USE_UNWANTED_PARAM(key);
   USE_UNWANTED_PARAM(len);
   FUNCTION_FINISH(STB_MEMReadSecureConstant);

   return(NULL);
}

/**
 * @brief   Returns the size in KB of the given file
 * @param   filename - name of the file in the nvm
 * @param   filesize - returned value giving the file size in KB
 * @return  TRUE if the file exists, FALSE otherwise
 */
BOOLEAN STB_NVMFileSize(U8BIT *filename, U32BIT *filesize)
{
   FUNCTION_START(STB_NVMFileSize);
   USE_UNWANTED_PARAM(filename);
   USE_UNWANTED_PARAM(filesize);
   FUNCTION_FINISH(STB_NVMFileSize);
   return(FALSE);
}

/**
 * @brief   Opens an existing file or creates a new one.
 * @param   name - The filename (including path). When the mode is
 *          FILE_MODE_OVERWRITE, the directories in the path will be created
 *          when they don't exist.
 * @param   mode - The access mode
 * @return  The file handle
 */
void* STB_NVMOpenFile(U8BIT *name, E_STB_DSK_FILE_MODE mode)
{
   FUNCTION_START(STB_NVMOpenFile);
   USE_UNWANTED_PARAM(name);
   USE_UNWANTED_PARAM(mode);
   FUNCTION_FINISH(STB_NVMOpenFile);
   return(NULL);
}

/**
 * @brief   Flushes and closes an open file
 * @param   file - The file handle
 */
void STB_NVMCloseFile(void *file)
{
   FUNCTION_START(STB_NVMCloseFile);
   USE_UNWANTED_PARAM(file);
   FUNCTION_FINISH(STB_NVMCloseFile);
}

/**
 * @brief   Reads data from an open file
 * @param   file - The file handle
 * @param   data - The caller's buffer
 * @param   size - Number of bytes to be read
 * @return  Number of bytes successfully read
 */
U32BIT STB_NVMReadFile(void *file, U8BIT *data, U32BIT size)
{
   FUNCTION_START(STB_NVMReadFile);
   USE_UNWANTED_PARAM(file);
   USE_UNWANTED_PARAM(data);
   USE_UNWANTED_PARAM(size);
   FUNCTION_FINISH(STB_NVMReadFile);
   return(0);
}

/**
 * @brief   Writes data to an open file
 * @param   file - The file handle
 * @param   data - Pointer to the data to be written
 * @param   size - Number of bytes to  write
 * @return  Number of bytes successfully written
 */
U32BIT STB_NVMWriteFile(void *file, U8BIT *data, U32BIT size)
{
   FUNCTION_START(STB_NVMWriteFile);
   USE_UNWANTED_PARAM(file);
   USE_UNWANTED_PARAM(data);
   USE_UNWANTED_PARAM(size);
   FUNCTION_FINISH(STB_NVMWriteFile);
   return(0);
}

/**
 * @brief   Deletes the file.
 * @param   filename - pathname of the file to be deleted
 * @retval  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_NVMDeleteFile(U8BIT *filename)
{
   FUNCTION_START(STB_NVMDeleteFile);
   USE_UNWANTED_PARAM(filename);
   FUNCTION_FINISH(STB_NVMDeleteFile);
   return(FALSE);
}

/**
 * @brief   Opens a directory in order to read the entries
 * @param   dir_name - name of the directory to open
 * @return  Handle to be used in all other operations, NULL if the open fails
 */
void* STB_NVMOpenDirectory(U8BIT *dir_name)
{
   FUNCTION_START(STB_NVMOpenDirectory);
   USE_UNWANTED_PARAM(dir_name);
   FUNCTION_FINISH(STB_NVMOpenDirectory);
   return(NULL);
}

/**
 * @brief   Reads the next entry from the directory, returning the name of the
 *          entry and the type of the entry
 * @param   dir - handle returned when the directory was opened
 * @param   filename - array in which the name is returned
 * @param   filename_len - size of the filename array
 * @param   entry_type - type of entry
 * @return  TRUE if and entry is read, FALSE otherwise which could indicate end
 *          of the directory
 */
BOOLEAN STB_NVMReadDirectory(void *dir, U8BIT *filename, U16BIT filename_len,
   E_STB_DIR_ENTRY_TYPE *entry_type)
{
   FUNCTION_START(STB_NVMReadDirectory);
   USE_UNWANTED_PARAM(dir);
   USE_UNWANTED_PARAM(filename);
   USE_UNWANTED_PARAM(filename_len);
   USE_UNWANTED_PARAM(entry_type);
   FUNCTION_FINISH(STB_NVMReadDirectory);
   return(FALSE);
}

/**
 * @brief   Closes the directory for reading
 * @param   dir - directory handle
 */
void STB_NVMCloseDirectory(void *dir)
{
   FUNCTION_START(STB_NVMCloseDirectory);
   USE_UNWANTED_PARAM(dir);
   FUNCTION_FINISH(STB_NVMCloseDirectory);
}

/*---local function definitions----------------------------------------------*/
