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
 * @file    stbhwmem.h
 * @brief   Header file - Function prototypes for NVM and Heap
 * @date    26/04/2002
 */

// pre-processor mechanism so multiple inclusions don't cause compilation error

#ifndef _STBHWMEM_H

#define _STBHWMEM_H

//#define  HEAP_DEBUG

#include "techtype.h"

//---Constant and macro definitions for public use-----------------------------
// IMPORTANT NOTE: The following definition musts comply with that defined in el_nvram.c of the loader
// source code. The number of NVRAM bytes that are reserved for the loader are defined in the first two
// bytes of NVRAM. The area immediately following those 2 bytes are the Euroloader NVM area itself
#define ELOAD_RESERVED_NVM_OFFSET      0  // offset of NVM reserved for Euroloader
#define ELOAD_RESERVED_NVM_SIZE_WIDTH  2  // width in bytes of variable denoting no.
// of Euroloader NVM bytes

#define ELOAD_NVM_DATA_OFFSET ELOAD_RESERVED_NVM_SIZE_WIDTH // E'loader data starts immed. after size variable

//---Enumerations for public use-----------------------------------------------

/* Key values for data items that can be stored in secure NVM */
enum
{
    SECURE_NVM_DEFAULT_ENCRYPTION_KEY,           /*   0 */
    SECURE_NVM_DEFAULT_ENC_INIT_VECTOR,          /*   1 */
    SECURE_NVM_CI_PLUS_ROOT_CERT,                /*   2 */
    SECURE_NVM_CI_PLUS_BRAND_CERT,               /*   3 */
    SECURE_NVM_CI_PLUS_DEVICE_CERT,              /*   4 */
    SECURE_NVM_CI_PLUS_DEVICE_KEY,               /*   5 */
    SECURE_NVM_CI_PLUS_DH_P,                     /*   6 */
    SECURE_NVM_CI_PLUS_DH_G,                     /*   7 */
    SECURE_NVM_CI_PLUS_DH_Q,                     /*   8 */
    SECURE_NVM_CI_PLUS_PRNG_KEY_K,               /*   9 */
    SECURE_NVM_CI_PLUS_PRNG_SEED,                /*  10 */
    SECURE_NVM_CI_PLUS_SIV,                      /*  11 */
    SECURE_NVM_CI_PLUS_SLK,                      /*  12 */
    SECURE_NVM_CI_PLUS_CLK,                      /*  13 */
    SECURE_NVM_CI_PLUS_CONTEXTS,                 /*  14 */
    SECURE_NVM_MHEG5_PRIVATE_KEY,                /*  15 */
    SECURE_NVM_MHEG5_PRIVATE_KEY_VERSION,        /*  16 */
    SECURE_NVM_SOFTWARE_UPGRADE_PUBLIC_KEY,      /*  17 */
    SECURE_NVM_SOFTWARE_UPGRADE_SHARED_KEY       /*  18 */
};

//---Global type defs for public use-------------------------------------------

//---Global Function prototypes for public use---------------------------------

#include "stbhwdsk.h" /* for E_STB_DSK_FILE_MODE */

/**
 * @brief   Initialises the heap
 */
void STB_MEMInitialiseRAM(void);

/**
 * @brief   Initialises Non-Volatile memory access. For a new NVM device, this function formats it
 *          ready for use, otherwise it prepares existing data for access.
 */
void STB_MEMInitialiseNVM(void);

/**
 * @brief   Read data from the NVM
 * @param   addr The NVM start address
 * @param   bytes The number of bytes to read
 * @param   dst_addr Caller's buffer for the data
 * @return  TRUE if successful FALSE otherwise
 */
BOOLEAN STB_MEMReadNVM(U32BIT addr, U32BIT bytes, U8BIT *dst_addr);

/**
 * @brief   Writes data to the NVM
 * @param   addr The NVM start address to write to
 * @param   bytes The number of bytes to be written
 * @param   src_addr Pointer to the data to be written
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_MEMWriteNVM(U32BIT addr, U32BIT bytes, U8BIT *src_addr);

/**
 * @brief   Read variable defined by the given key from secure NVM. The data must be released using
 *          STB_MEMReleaseSecureVariable. This function is used for CI+
 * @param   key data item to be read
 * @param   len returned length of data read
 * @return  Pointer to data read, or NULL on failure.
 */
void* STB_MEMReadSecureVariable(U8BIT key, U32BIT *len);

/**
 * @brief   Releas the data returned by STB_MEMReadSecureVariable. This function is used for CI+
 * @param   value pointer to data to be released
 */
void STB_MEMReleaseSecureVariable(void *value);

/**
 * @brief   Writes data defined by the given key to secure NVM. This function is used for CI+
 * @param   key data item to be written
 * @param   value pointer to data to be written
 * @param   len data size
 * @return  TRUE if the data is written successfully, FALSE otherwise
 */
BOOLEAN STB_MEMWriteSecureVariable(U8BIT key, void *value, U32BIT len);

/**
 * @brief   Read constant defined by the given key from secure NVM. The data must not be released
 *          (it is managed by the platform layer). This function is used for CI+
 * @param   key data item to be read
 * @param   len returned length of data read
 * @return  Pointer to data read, or NULL on failure.
 */
const void* STB_MEMReadSecureConstant(U8BIT key, U32BIT *len);

/**
 * @brief   Returns the size (capacity) of the NVM
 * @return  Size of the NVM in bytes
 */
U32BIT STB_MEMGetNVMSize(void);

/**
 * @brief   Returns any offset required in NVM to avoid private data
 * @return  The offset in bytes
 */
U32BIT STB_MEMGetNVMOffset(void);

/**
 * @brief   Returns the word alignment size of the NVM device
 * @return  Alignment in bytes
 */
U8BIT STB_MEMGetNVMAlign(void);

#ifndef HEAP_DEBUG
/**
 * @brief   Allocates a new block of memory for system use
 * @param   bytes Size of required block in bytes
 * @return  Pointer to the allocated block of memory or NULL if allocation failed
 */
void* STB_MEMGetSysRAM(U32BIT bytes);

/**
 * @brief   Changes the size of the given block of memory ensuring data contained within the
 *          original memory block is maintained.
 * @param   ptr pointer to memory already allocated from the system heap
 * @param   new_num_bytes size of the memory block to be returned
 * @return  Address of new block of memory
 */
void* STB_MEMResizeSysRAM(void *ptr, U32BIT new_num_bytes);

/**
 * @brief   Releases a previously allocated block of system memory
 * @param   block_ptr address of block to be released
 */
void STB_MEMFreeSysRAM(void *block);

/**
 * @brief   Returns the amount of available system memory consumed
 * @return  Memory used as a percentage of available
 */
U8BIT STB_MEMSysRAMUsed(void);

/**
 * @brief   Allocates a new block of memory for application use
 * @param   bytes Size of required block in bytes
 * @return  A pointer to the allocated block of memory
 * @return  NULL allocation failed
 */
void* STB_MEMGetAppRAM(U32BIT bytes);

/**
 * @brief   Changes the size of the given block of memory ensuring data contained within the
 *          original memory block is maintained.
 * @param   ptr pointer to memory already allocated from the app heap
 * @param   new_num_bytes size of the memory block to be returned
 * @return  Address of new block of memory
 */
void* STB_MEMResizeAppRAM(void *ptr, U32BIT new_num_bytes);

/**
 * @brief   Releases a previously allocated block of system memory
 * @param   block_ptr address of block to be released
 */
void STB_MEMFreeAppRAM(void *block);

/**
 * @brief   Returns the amount of available application memory consumed
 * @return  Memory used as a percentage of available
 */
U8BIT STB_MEMAppRAMUsed(void);
#endif

BOOLEAN STB_MEMConfigMloaderForUpgrade(void *loader_data, U32BIT data_size);
BOOLEAN STB_MEMReadMloaderData(void *buffer, U32BIT size);
BOOLEAN STB_MEMWriteMloaderData(void *buffer, U32BIT size);

/**
 * @brief   Returns the size in KB of the given file
 * @param   filename Name of the file in the nvm
 * @param   filesize Returned value giving the file size in KB
 * @return  TRUE if the file exists, FALSE otherwise
 */
BOOLEAN STB_NVMFileSize(U8BIT *filename, U32BIT *filesize);

/**
 * @brief   Opens an existing file or creates a new one.
 * @param   name - The filename (including path). When the mode is
 *          FILE_MODE_OVERWRITE, the directories in the path will be created
 *          when they don't exist.
 * @param   mode The access mode
 * @return  The file handle
 */
void* STB_NVMOpenFile(U8BIT *name, E_STB_DSK_FILE_MODE mode);

/**
 * @brief    Flushes and closes an open file
 * @param    file The file handle
 */
void STB_NVMCloseFile(void *file);

/**
 * @brief    Reads data from an open file
 * @param    file The file handle
 * @param    data The caller's buffer
 * @param    size Number of bytes to be read
 * @return   Number of bytes successfully read
 */
U32BIT STB_NVMReadFile(void *file, U8BIT *data, U32BIT size);

/**
 * @brief    Writes data to an open file
 * @param    file The file handle
 * @param    data Pointer to the data to be written
 * @param    size Number of bytes to  write
 * @return   Number of bytes successfully written
 */
U32BIT STB_NVMWriteFile(void *file, U8BIT *data, U32BIT size);

/**
 * @brief   Deletes the file.
 * @param   filename Path name of the file to be deleted
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_NVMDeleteFile(U8BIT *filename);

/**
 * @brief   Move file or directory (and it's children).
 * @param   oldname Path name of the file to be moved
 * @param   newname Path name of new file location
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_NVMMoveFile(U8BIT* oldname, U8BIT* newname);

/**
 * @brief   Opens a directory in order to read the entries
 * @param   dir_name Name of the directory to open
 * @return  Handle to be used in all other operations, NULL if the open fails
 */
void* STB_NVMOpenDirectory(U8BIT *dir_name);

/**
 * @brief   Reads the next entry from the directory, returning the name of the
 *          entry and the type of the entry
 * @param   dir Handle returned when the directory was opened
 * @param   filename Array in which the name is returned
 * @param   filename_len Size of the filename array
 * @param   entry_type Type of entry
 * @return  TRUE if and entry is read, FALSE otherwise which could indicate end
 *          of the directory
 */
BOOLEAN STB_NVMReadDirectory(void *dir, U8BIT *filename, U16BIT filename_len,
                             E_STB_DIR_ENTRY_TYPE *entry_type);

/**
 * @brief   Closes the directory for reading
 * @param   dir directory handle
 */
void STB_NVMCloseDirectory(void *dir);

#ifdef  HEAP_DEBUG
void* STB_MEMGetSysRAM_DBG(U32BIT bytes, const char *filename, int linenum);
void* STB_MEMResizeSysRAM_DBG(void *ptr, U32BIT new_num_bytes, const char *filename, int linenum);
void STB_MEMFreeSysRAM_DBG(void *block, const char *filename, int linenum);
U8BIT STB_MEMSysRAMUsed_DBG(const char *filename, int linenum);
void* STB_MEMGetAppRAM_DBG(U32BIT bytes, const char *filename, int linenum);
void* STB_MEMResizeAppRAM_DBG(void *ptr, U32BIT new_num_bytes, const char *filename, int linenum);
void STB_MEMFreeAppRAM_DBG(void *block, const char *filename, int linenum);
U8BIT STB_MEMAppRAMUsed_DBG(const char *filename, int linenum);


#define STB_MEMGetSysRAM(a) STB_MEMGetSysRAM_DBG(a, __FILE__, __LINE__)
#define STB_MEMResizeSysRAM(a, b) STB_MEMResizeSysRAM_DBG(a, b, __FILE__, __LINE__)
#define STB_MEMFreeSysRAM(a) STB_MEMFreeSysRAM_DBG(a, __FILE__, __LINE__)
#define STB_MEMSysRAMUsed() STB_MEMSysRAMUsed_DBG(__FILE__, __LINE__)


#define STB_MEMGetAppRAM(a) STB_MEMGetAppRAM_DBG(a, __FILE__, __LINE__)
#define STB_MEMResizeAppRAM(a, b) STB_MEMResizeAppRAM_DBG(a, b, __FILE__, __LINE__)
#define STB_MEMFreeAppRAM(a) STB_MEMFreeAppRAM_DBG(a, __FILE__, __LINE__)
#define STB_MEMAppRAMUsed() STB_MEMAppRAMUsed_DBG(__FILE__, __LINE__)


#endif


#endif //  _STBHWMEM_H
