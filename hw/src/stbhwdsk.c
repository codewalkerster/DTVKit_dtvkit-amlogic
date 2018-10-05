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
 * @brief   Set Top Box - Hardware Layer, Disk Functions
 * @file    stbhwdsk.c
 * @date    October 2018
 */

/*#define  DISK_DEBUG*/

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */

/* third party header files */
/* STB Header Files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwdsk.h"

/*---macro definitions for this file-----------------------------------------*/
#ifdef  DISK_DEBUG
   #define  DISK_DBG(x,...)      STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define  DISK_DBG(x,...)
#endif

/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/

/*---local function prototypes for this file---------------------------------*/

/*---global function definitions----------------------------------------------*/

/**
 * @brief   Initialise the hard disk component
 */
void STB_DSKInitialise(void)
{
   FUNCTION_START(STB_DSKInitialise);
   FUNCTION_FINISH(STB_DSKInitialise);
}

/**
 * @brief   Returns the number of disks currently detected
 * @return  Number of disks
 */
U16BIT STB_DSKGetNumDisks(void)
{
   FUNCTION_START(STB_DSKGetNumDisks);
   FUNCTION_FINISH(STB_DSKGetNumDisks);
   return(0);
}

/**
 * @brief   Returns the id of the disk at the given index
 * @param   index zero based index
 * @return  Disk id, 0 if no disk found
 */
U16BIT STB_DSKGetDiskIdByIndex(U16BIT index)
{
   FUNCTION_START(STB_DSKGetDiskIdByIndex);
   USE_UNWANTED_PARAM(index);
   FUNCTION_FINISH(STB_DSKGetDiskIdByIndex);

   return(INVALID_DISK_ID);
}

/**
 * @brief   Checks if the given disk is removeable
 * @param   disk_id ID of the disk to be checked
 * @return  TRUE if the disk is removeable, FALSE otherwise
 */
BOOLEAN STB_DSKIsRemoveable(U16BIT disk_id)
{
   FUNCTION_START(STB_DSKIsRemoveable);
   USE_UNWANTED_PARAM(disk_id);
   FUNCTION_FINISH(STB_DSKIsRemoveable);

   return FALSE;
}

/**
 * @brief   Checks if the given disk is mounted
 * @param   disk_id ID of the disk to be checked
 * @return  TRUE if the disk is mounted, FALSE otherwise
 */
BOOLEAN STB_DSKIsMounted(U16BIT disk_id)
{
   FUNCTION_START(STB_DSKIsMounted);
   USE_UNWANTED_PARAM(disk_id);
   FUNCTION_FINISH(STB_DSKIsMounted);

   return FALSE;
}

/**
 * @brief   Attempts to mount the given disk, if it isn't already mounted
 * @param   disk_id ID of the disk to be mounted
 * @return  TRUE if the disk is mounted, FALSE otherwise
 */
BOOLEAN STB_DSKMountDisk(U16BIT disk_id)
{
   FUNCTION_START(STB_DSKMountDisk);
   USE_UNWANTED_PARAM(disk_id);
   FUNCTION_FINISH(STB_DSKMountDisk);

   return FALSE;
}

/**
 * @brief   Attempts to unmount the given disk, if it isn't already unmounted
 * @param   disk_id ID of the disk to be unmounted
 * @return  TRUE if the disk is unmounted, FALSE otherwise
 */
BOOLEAN STB_DSKUnmountDisk(U16BIT disk_id)
{
   FUNCTION_START(STB_DSKUnmountDisk);
   USE_UNWANTED_PARAM(disk_id);
   FUNCTION_FINISH(STB_DSKUnmountDisk);

   return FALSE;
}

/**
 * @brief   Gets the name of a disk and copies it into the array provided
 * @param   disk_id ID of the disk
 * @param   name array of U8BIT into which the name will be copied
 * @param   name_len max number of characters in the name
 * @return  TRUE if the disk is found, FALSE otherwise
 */
BOOLEAN STB_DSKGetDiskName(U16BIT disk_id, U8BIT *name, U16BIT name_len)
{
   FUNCTION_START(STB_DSKGetDiskName);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(name);
   USE_UNWANTED_PARAM(name_len);
   FUNCTION_FINISH(STB_DSKGetDiskName);

   return FALSE;
}

/**
 * @brief   Returns the amount of space used on the disk
 * @param   disk_id ID of the disk
 * @return  Space used in kilobytes
 */
U32BIT STB_DSKGetUsed(U16BIT disk_id)
{
   FUNCTION_START(STB_DSKGetUsed);
   USE_UNWANTED_PARAM(disk_id);
   FUNCTION_FINISH(STB_DSKGetUsed);

   return 0;
}

/**
 * @brief   Queries whether the disk is formatted
 * @return  TRUE if formatted, FALSE otherwise
 */
BOOLEAN STB_DSKIsFormatted(U16BIT disk_id)
{
   FUNCTION_START(STB_DSKIsFormatted);
   USE_UNWANTED_PARAM(disk_id);
   FUNCTION_FINISH(STB_DSKIsFormatted);

   return FALSE;
}

/**
 * @brief   Returns the size (capacity) of the disk
 * @return  The size of the disk in kilobytes
 */
U32BIT STB_DSKGetSize(U16BIT disk_id)
{
   FUNCTION_START(STB_DSKGetSize);
   USE_UNWANTED_PARAM(disk_id);
   FUNCTION_FINISH(STB_DSKGetSize);

   return 0;
}

/**
 * @brief   Initiates formatting and partitioning of the hard disk. This will erase all data on the
 *          disk!
 */
void STB_DSKFormat(U16BIT disk_id)
{
   FUNCTION_START(STB_DSKFormat);
   USE_UNWANTED_PARAM(disk_id);
   FUNCTION_FINISH(STB_DSKFormat);
}

/**
 * @brief   Gets the progress of the format operation
 * @return  The progress as percent complete
 */
U8BIT STB_DSKGetFormatProgress(U16BIT disk_id)
{
   FUNCTION_START(STB_DSKGetFormatProgress);
   USE_UNWANTED_PARAM(disk_id);
   FUNCTION_FINISH(STB_DSKGetFormatProgress);
   return(0);
}

/**
 * @brief   Returns a summary of the disk integrity
 * @return   TRUE if disk integrity ok
 */
BOOLEAN STB_DSKGetIntegrity(U16BIT disk_id)
{
   FUNCTION_START(STB_DSKGetIntegrity);
   USE_UNWANTED_PARAM(disk_id);
   FUNCTION_FINISH(STB_DSKGetIntegrity);
   return FALSE;
}

/**
 * @brief   Initiates a data repair of the hard disk. This *may* cause the disk to become unreadable
 */
void STB_DSKRepair(U16BIT disk_id)
{
   FUNCTION_START(STB_DSKRepair);
   USE_UNWANTED_PARAM(disk_id);
   FUNCTION_FINISH(STB_DSKRepair);
}

/**
 * @brief   Gets the progress of the repair operation
 * @return  The progress as percent complete
 */
U8BIT STB_DSKGetRepairProgress(U16BIT disk_id)
{
   FUNCTION_START(STB_DSKGetRepairProgress);
   USE_UNWANTED_PARAM(disk_id);
   FUNCTION_FINISH(STB_DSKGetRepairProgress);
   return 0;
}

/**
 * @brief   Copies the full pathname for the given filename, including the mount directory,
 *          to the given string array
 * @param   disk_id disk
 * @param   filename name of the file on the disk
 * @param   pathname array into which the full pathname will be copied
 * @param   max_pathname_len size of the pathname array
 * @return  TRUE if the disk is valid exists and is mounted and the destination string
 *          is long enough to take the full pathname
 */
BOOLEAN STB_DSKFullPathname(U16BIT disk_id, U8BIT *filename, U8BIT *pathname, U16BIT max_pathname_len)
{
   FUNCTION_START(STB_DSKFullPathname);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(filename);
   USE_UNWANTED_PARAM(pathname);
   USE_UNWANTED_PARAM(max_pathname_len);
   FUNCTION_FINISH(STB_DSKFullPathname);

   return FALSE;
}

/**
 * @brief   Opens an existing file or creates a new one
 * @param   name The filename (including path)
 * @param   mode The access mode
 * @return  The file handle
 */
void* STB_DSKOpenFile(U16BIT disk_id, U8BIT *name, E_STB_DSK_FILE_MODE mode)
{
   FUNCTION_START(STB_DSKOpenFile);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(name);
   USE_UNWANTED_PARAM(mode);
   FUNCTION_FINISH(STB_DSKOpenFile);

   return(NULL);
}

/**
 * @brief   Flushes and closes an open file
 * @param   file The file handle
 */
void STB_DSKCloseFile(void *file)
{
   FUNCTION_START(STB_DSKCloseFile);
   USE_UNWANTED_PARAM(file);
   FUNCTION_FINISH(STB_DSKCloseFile);
}

/**
 * @brief   Reads data from an open file
 * @param   file The file handle
 * @param   data The caller's buffer
 * @param   size Number of bytes to be read
 * @return  Number of bytes successfully read
 */
U32BIT STB_DSKReadFile(void *file, U8BIT *data, U32BIT size)
{
   FUNCTION_START(STB_DSKReadFile);
   USE_UNWANTED_PARAM(file);
   USE_UNWANTED_PARAM(data);
   USE_UNWANTED_PARAM(size);
   FUNCTION_FINISH(STB_DSKReadFile);

   return 0;
}

/**
 * @brief   Writes data to an open file
 * @param   file The file handle
 * @param   data Pointer to the data to be written
 * @param   size Number of bytes to  write
 * @return  Number of bytes successfully written
 */
U32BIT STB_DSKWriteFile(void *file, U8BIT *data, U32BIT size)
{
   FUNCTION_START(STB_DSKWriteFile);
   USE_UNWANTED_PARAM(file);
   USE_UNWANTED_PARAM(data);
   USE_UNWANTED_PARAM(size);
   FUNCTION_FINISH(STB_DSKWriteFile);

   return 0;
}

/**
 * @brief   Sets the read/write position of an open file
 * @param   file The file handle
 * @param   position Position to move relative to, i.e. start,end,current
 * @param   offset Where to move to relative to position
 * @return  TRUE success, FALSE otherwise
 */
BOOLEAN STB_DSKSeekFile(void *file, E_STB_DSK_FILE_POSITION position, S32BIT offset)
{
   FUNCTION_START(STB_DSKSeekFile);
   USE_UNWANTED_PARAM(file);
   USE_UNWANTED_PARAM(position);
   USE_UNWANTED_PARAM(offset);
   FUNCTION_FINISH(STB_DSKSeekFile);

   return FALSE;
}

/**
 * @brief   Gets the current position in an open file
 * @param   file The file handle
 * @param   offset Variable to contain result (byte position in file)
 * @return  TRUE on success
 */
BOOLEAN STB_DSKTellFile(void *file, U32BIT *offset)
{
   FUNCTION_START(STB_DSKTellFile);
   USE_UNWANTED_PARAM(file);

   *offset = 0;

   FUNCTION_FINISH(STB_DSKTellFile);

   return(FALSE);
}

/**
 * @brief   Deletes the file from the given disk
 * @param   disk_id disk ID
 * @param   filename pathname on the disk of the file to be deleted
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_DSKDeleteFile(U16BIT disk_id, U8BIT *filename)
{
   FUNCTION_START(STB_DSKDeleteFile);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(filename);
   FUNCTION_FINISH(STB_DSKDeleteFile);

   return FALSE;
}

/**
 * @brief   Checks whether a file/directory will the given name exists
 * @param   disk_id disk ID
 * @param   filename pathname on the disk of the file
 * @return  TRUE if exists, FALSE otherwise
 */
BOOLEAN STB_DSKFileExists(U16BIT disk_id, U8BIT *filename)
{
   FUNCTION_START(STB_DSKFileExists);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(filename);
   FUNCTION_FINISH(STB_DSKFileExists);

   return FALSE;
}

/**
 * @brief   Returns the size in KB of the given file
 * @param   disk_id disk on which the file exists
 * @param   filename name of the file on disk
 * @param   filesize returned value giving the file size in KB
 * @return  TRUE if the file exists, FALSE otherwise
 */
BOOLEAN STB_DSKFileSize(U16BIT disk_id, U8BIT *filename, U32BIT *filesize)
{
   FUNCTION_START(STB_DSKFileSize);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(filename);

   *filesize = 0;

   FUNCTION_FINISH(STB_DSKFileSize);

   return FALSE;
}

/**
 * @brief   Opens a directory in order to read the entries
 * @param   disk_id disk containing to the directory to be read
 * @param   dir_name name of the directory to open
 * @return  Handle to be used in all other operations, NULL if the open fails
 */
void* STB_DSKOpenDirectory(U16BIT disk_id, U8BIT *dir_name)
{
   FUNCTION_START(STB_DSKOpenDirectory);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(dir_name);
   FUNCTION_FINISH(STB_DSKOpenDirectory);

   return(NULL);
}

/**
 * @brief   Reads the next entry from the directory, returning the name of the
 *          entry and the type of the entry
 * @param   dir handle returned when the directory was opened
 * @param   filename array in which the name is returned
 * @param   filename_len size of the filename array
 * @param   entry_type type of entry
 * @return  TRUE if and entry is read, FALSE otherwise which could indicate end of the directory
 */
BOOLEAN STB_DSKReadDirectory(void *dir, U8BIT *filename, U16BIT filename_len,
   E_STB_DIR_ENTRY_TYPE *entry_type)
{
   FUNCTION_START(STB_DSKReadDirectory);
   USE_UNWANTED_PARAM(dir);
   USE_UNWANTED_PARAM(filename);
   USE_UNWANTED_PARAM(filename_len);
   USE_UNWANTED_PARAM(entry_type);
   FUNCTION_FINISH(STB_DSKReadDirectory);

   return FALSE;
}

/**
 * @brief   Closes the directory for reading
 * @param   dir directory handle
 */
void STB_DSKCloseDirectory(void *dir)
{
   FUNCTION_START(STB_DSKCloseDirectory);
   USE_UNWANTED_PARAM(dir);
   FUNCTION_FINISH(STB_DSKCloseDirectory);
}

/**
 * @brief   Creates a directory with the given name
 * @param   disk_id disk on which the directory is to be created
 * @param   dir_name name of the directory to be created
 * @return  TRUE is the directory is successfully created
 */
BOOLEAN STB_DSKCreateDirectory(U16BIT disk_id, U8BIT *dir_path)
{
   FUNCTION_START(STB_DSKCreateDirectory);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(dir_path);
   FUNCTION_FINISH(STB_DSKCreateDirectory);

   return FALSE;
}

/**
 * @brief   Deletes a directory and all it contents, so use with care!
 * @param   disk_id disk
 * @param   dir_name name of the directory to be deleted
 */
BOOLEAN STB_DSKDeleteDirectory(U16BIT disk_id, U8BIT *dir_name)
{
   FUNCTION_START(STB_DSKDeleteDirectory);
   USE_UNWANTED_PARAM(disk_id);
   USE_UNWANTED_PARAM(dir_name);
   FUNCTION_FINISH(STB_DSKDeleteDirectory);

   return FALSE;
}

/**
 * @brief   Put all disks into or out of standby mode
 * @param   state standby mode, TRUE=Standby FALSE=On
 */
void STB_DSKSetStandby(BOOLEAN state)
{
   FUNCTION_START(STB_DSKSetStandby);
   USE_UNWANTED_PARAM(state);
   FUNCTION_FINISH(STB_DSKSetStandby);
}

/*---local function definitions----------------------------------------------*/

