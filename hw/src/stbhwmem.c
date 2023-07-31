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
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "dtv_log.h"
#define TAG  "STBHWMEM"


/* third party header files */

/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"

#include "stbhwdef.h"
#include "stbhwc.h"
#include "stbhwmem.h"

/*---macro definitions for this file-----------------------------------------*/
#ifdef  HEAP_DEBUG
#define  HEAP_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  HEAP_DBG(x,...)
#endif

#ifdef  NVM_DEBUG
#define  NVM_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  NVM_DBG(x,...)
#endif

#ifdef  SECURE_DEBUG
#define  SEC_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  SEC_DBG(x,...)
#endif

#ifdef DTVKIT_IN_VENDOR_PARTITION
#define NVM_PATH "/data/vendor/dtvkit/NVM/"
#else
#define NVM_PATH "/data/data/com.droidlogic.dtvkit.inputsource/NVM/"
#endif

/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/

/*---local function prototypes for this file---------------------------------*/

/*---local function definitions----------------------------------------------*/
static U8BIT*  ConvertPathToNVM(U8BIT* path);
static BOOLEAN CreateDirectories(U8BIT* path);

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
static inline void* _STB_MEMGetSysRAM(U32BIT bytes)
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
static inline void* _STB_MEMResizeSysRAM(void *ptr, U32BIT new_num_bytes)
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
static inline void _STB_MEMFreeSysRAM(void *block_ptr)
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
static inline U8BIT _STB_MEMSysRAMUsed(void)
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
static inline void* _STB_MEMGetAppRAM(U32BIT bytes)
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
static inline void* _STB_MEMResizeAppRAM(void *ptr, U32BIT new_num_bytes)
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
static inline void _STB_MEMFreeAppRAM(void *block_ptr)
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
static inline U8BIT _STB_MEMAppRAMUsed(void)
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
   char buf[4096+1];
   char *p1, *p2;
   uint64_t key_v = 0;
   static uint8_t des_key[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
   FUNCTION_START(STB_MEMReadSecureConstant);
   /*If the private global encrypt procedure for pvr data is on,
     return a fake key to make the encrypt procedure correct in DVBCore.*/

   /*But for now, the secure things are not ready in dtvkit,
     we temporarily make the encrypt request all pass,
     even the private global encrypt procedure is off,
     fix me later.*/
   int force_fake = 1;

   if (key == SECURE_NVM_DEFAULT_ENCRYPTION_KEY
               || key == SECURE_NVM_DEFAULT_ENC_INIT_VECTOR) {
      int readlen;
      int fd = open("/proc/cpuinfo", O_RDONLY);
      *len = sizeof(des_key);
      if (fd == -1) {
	return &des_key[0];
      }

      readlen = read(fd, buf, sizeof(buf)-1);
      buf[sizeof(buf)-1]= '\0';
      close(fd);
      if ((readlen != 0) && (p1 = strstr(buf, "Serial"))) {
         if ((p2 = strstr(p1, ": "))) {
            char *pc = p2 + 2;
	    while (1) {
               int r;
	       uint8_t n;
	       r = sscanf(pc, "%02hhx", &n);
	       if (r != 1)
		  break;
	       key_v = ((key_v << 5) | n);
	       pc += 2;
	    }
	    *(uint64_t *)des_key = key_v;
	    return &des_key[0];
	 }
      } else {
	 return &des_key[0];
      }

   } else if (force_fake) {
      *len = 64;
      return "0123456789abcdef";
   }

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
   U8BIT *path;
   FILE *file_p = NULL;
   BOOLEAN success = FALSE;

   FUNCTION_START(STB_NVMFileSize);

   path = ConvertPathToNVM(filename);
   if (path != NULL)
   {
      file_p = fopen(path, "r");
      if (file_p != NULL)
      {
         fseek(file_p, 0, SEEK_END);
         *filesize = ftell(file_p);
         fclose(file_p);
         success = TRUE;
      }
      STB_MEMFreeSysRAM(path);
   }
   FUNCTION_FINISH(STB_NVMFileSize);
   return success;
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
   FILE *file_p = NULL;
   U8BIT *path;
   FUNCTION_START(STB_NVMOpenFile);

   path = ConvertPathToNVM(name);
   if (path != NULL)
   {
      switch (mode)
      {
      case FILE_MODE_OVERWRITE:
         if (CreateDirectories(path) == TRUE)
         {
            file_p = fopen(path, "w");
         }
         break;
      case FILE_MODE_READ:
         file_p = fopen(path, "r");
         break;
      case FILE_MODE_WRITE:
         file_p = fopen(path, "r+");
         break;
      }

      STB_MEMFreeSysRAM(path);
   }

   FUNCTION_FINISH(STB_NVMOpenFile);
   return file_p;
}

/**
 * @brief   Flushes and closes an open file
 * @param   file - The file handle
 */
void STB_NVMCloseFile(void *file)
{
   FUNCTION_START(STB_NVMCloseFile);

   fclose(file);

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
   U32BIT read;
   FUNCTION_START(STB_NVMReadFile);

   read = fread(data, sizeof(U8BIT), size, file);

   FUNCTION_FINISH(STB_NVMReadFile);
   return read;
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
   U32BIT written;
   FUNCTION_START(STB_NVMWriteFile);

   written = fwrite(data, sizeof(U8BIT), size, file);

   FUNCTION_FINISH(STB_NVMWriteFile);
   return written;
}

/**
 * @brief   Deletes the file.
 * @param   filename - pathname of the file to be deleted
 * @retval  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_NVMDeleteFile(U8BIT *filename)
{
   U8BIT *path;
   BOOLEAN success = FALSE;

   FUNCTION_START(STB_NVMDeleteFile);

   path = ConvertPathToNVM(filename);
   if (path != NULL)
   {
      if (unlink(path) == 0)
      {
         success = TRUE;
      }
      STB_MEMFreeSysRAM(path);
   }

   FUNCTION_FINISH(STB_NVMDeleteFile);
   return success;
}

/**
 * @brief   Opens a directory in order to read the entries
 * @param   dir_name - name of the directory to open
 * @return  Handle to be used in all other operations, NULL if the open fails
 */
void* STB_NVMOpenDirectory(U8BIT *dir_name)
{
   DIR* handle = NULL;
   U8BIT *path;
   FUNCTION_START(STB_NVMOpenDirectory);

   path = ConvertPathToNVM(dir_name);
   if (path != NULL)
   {
      handle = opendir(path);

      STB_MEMFreeSysRAM(path);
   }

   FUNCTION_FINISH(STB_NVMOpenDirectory);
   return handle;
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
   struct dirent* entry;
   BOOLEAN success = FALSE;
   FUNCTION_START(STB_NVMReadDirectory);

   entry = readdir(dir);
   if (entry != NULL)
   {
      switch (entry->d_type)
      {
      case DT_REG:
         *entry_type = DIR_ENTRY_FILE;
         success = TRUE;
         break;
      case DT_DIR:
         *entry_type = DIR_ENTRY_DIRECTORY;
         success = TRUE;
         break;
      default:
         break;
      }
      if (success)
      {
         strncpy(filename,entry->d_name,filename_len);
      }
   }

   FUNCTION_FINISH(STB_NVMReadDirectory);
   return success;
}

/**
 * @brief   Closes the directory for reading
 * @param   dir - directory handle
 */
void STB_NVMCloseDirectory(void *dir)
{
   FUNCTION_START(STB_NVMCloseDirectory);

   closedir(dir);

   FUNCTION_FINISH(STB_NVMCloseDirectory);
}

/*---local function definitions----------------------------------------------*/

static U8BIT* ConvertPathToNVM(U8BIT* path)
{
  U8BIT* new_path = NULL;
  U32BIT length ;

  length = strlen(path);
  if (length > 0)
  {
     new_path = STB_MEMGetSysRAM(length+strlen(NVM_PATH)+1);
     if (new_path != NULL)
     {
        sprintf(new_path,"%s%s",NVM_PATH,path);
     }
  }
  return new_path;
}


static BOOLEAN CreateDirectories(U8BIT* path)
{
   BOOLEAN success = FALSE;
   char *dir_path;
   int retval = 0;
   U32BIT cursor;

   /*Make a temporary buffer where we can manipulate the path*/
   dir_path = STB_MEMGetSysRAM(strlen(path)+1);
   if (dir_path != NULL)
   {
      /*start from the preset NVM_PATH*/
      cursor = strlen(NVM_PATH)-1;
      while (path[cursor] != '\0')
      {
         if (path[cursor] == '/')
         {
            strncpy(dir_path,path,cursor);
            dir_path[cursor] = '\0';
            retval = mkdir(dir_path, (S_IFDIR | S_IRWXU | S_IRWXG));
            if (retval == 0 || errno == EEXIST) /*The only acceptable error is that the directory exists*/
            {
               success = TRUE;
            }
            else
            {
               success = FALSE;
               break;
            }
         }
         cursor++;
      }

      STB_MEMFreeSysRAM(dir_path);
   }
   return success;

}


#ifdef HEAP_DEBUG
extern void LB_HEAP_DBG_PRINT(const char *format, ... );

void* STB_MEMGetSysRAM_DBG(U32BIT bytes, const char *filename, int linenum)
{
    void *buffer = _STB_MEMGetSysRAM(bytes);

    if (NULL != buffer)
    {
        LB_HEAP_DBG_PRINT("[PGW] allocate (%d) bytes at [%s:%d] [%p, %p] \n", bytes,
                        filename, linenum, buffer, buffer);
    }
    else
    {
        LB_HEAP_DBG_PRINT("[PGW] Cannot allocate %d bytes at %s:%d\n", bytes,
                        filename, linenum);
    }

    return buffer;
}

void* STB_MEMResizeSysRAM_DBG(void *ptr, U32BIT new_num_bytes, const char *filename, int linenum)
{
    void *buffer = NULL;

    LB_HEAP_DBG_PRINT("[PGW] [Resize] free (%d) bytes at [%s:%d] [%p, %p] \n", 0,
                        filename, linenum, ptr, ptr);

    buffer = _STB_MEMResizeSysRAM(ptr, new_num_bytes);

    LB_HEAP_DBG_PRINT("[PGW] [Resize] allocate (%d) bytes at [%s:%d] [%p, %p] \n", new_num_bytes,
                        filename, linenum, buffer, buffer);

    return buffer;
}

void STB_MEMFreeSysRAM_DBG(void *block, const char *filename, int linenum)
{
    LB_HEAP_DBG_PRINT("[PGW] free (%d) bytes at [%s:%d] [%p, %p] \n", 0,
                    filename, linenum, block, block);

    _STB_MEMFreeSysRAM(block);
}

U8BIT STB_MEMSysRAMUsed_DBG(const char *filename, int linenum)
{
    return _STB_MEMSysRAMUsed();
}



void* STB_MEMGetAppRAM_DBG(U32BIT bytes, const char *filename, int linenum)
{
    void *buffer = _STB_MEMGetAppRAM(bytes);

    if (NULL != buffer)
    {
        LB_HEAP_DBG_PRINT("[PGW] allocate (%d) bytes at [%s:%d] [%p, %p] \n", bytes,
                        filename, linenum, buffer, buffer);
    }
    else
    {
        LB_HEAP_DBG_PRINT("[PGW] Cannot allocate %d bytes at %s:%d\n", bytes,
                        filename, linenum);
    }

    return buffer;
}

void* STB_MEMResizeAppRAM_DBG(void *ptr, U32BIT new_num_bytes, const char *filename, int linenum)
{
    void *buffer = NULL;

    LB_HEAP_DBG_PRINT("[PGW] [Resize] free (%d) bytes at [%s:%d] [%p, %p] \n", 0,
                        filename, linenum, ptr, ptr);

    buffer = _STB_MEMResizeAppRAM(ptr, new_num_bytes);

    LB_HEAP_DBG_PRINT("[PGW] [Resize] allocate (%d) bytes at [%s:%d] [%p, %p] \n", new_num_bytes,
                        filename, linenum, buffer, buffer);

    return buffer;
}


void STB_MEMFreeAppRAM_DBG(void *block, const char *filename, int linenum)
{
    LB_HEAP_DBG_PRINT("[PGW] free (%d) bytes at [%s:%d] [%p, %p] \n", 0,
                    filename, linenum, block, block);

    _STB_MEMFreeAppRAM(block);
}


U8BIT STB_MEMAppRAMUsed_DBG(const char *filename, int linenum)
{
    return _STB_MEMAppRAMUsed();
}


#else

void* STB_MEMGetSysRAM(U32BIT bytes)
{
    return _STB_MEMGetSysRAM(bytes);
}

void* STB_MEMResizeSysRAM(void *ptr, U32BIT new_num_bytes)
{
    return _STB_MEMResizeSysRAM(ptr, new_num_bytes);
}

void STB_MEMFreeSysRAM(void *block)
{
    _STB_MEMFreeSysRAM(block);
}

U8BIT STB_MEMSysRAMUsed()
{
    return _STB_MEMSysRAMUsed();
}



void* STB_MEMGetAppRAM(U32BIT bytes)
{
    return _STB_MEMGetAppRAM(bytes);
}

void* STB_MEMResizeAppRAM(void *ptr, U32BIT new_num_bytes)
{
    void *buffer = NULL;

    return _STB_MEMResizeAppRAM(ptr, new_num_bytes);

}

void STB_MEMFreeAppRAM(void *block)
{
    _STB_MEMFreeAppRAM(block);
}

U8BIT STB_MEMAppRAMUsed()
{
    return _STB_MEMAppRAMUsed();
}
#endif


