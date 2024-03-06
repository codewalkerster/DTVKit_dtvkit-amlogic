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
 * @file    stbhwdsk.h
 * @brief   Function prototypes for disk functions
 * @date    20/08/2003
 */

// pre-processor mechanism so multiple inclusions don't cause compilation error

#ifndef _STBHWDSK_H

#define _STBHWDSK_H

#include "techtype.h"

//---Constant and macro definitions for public use-----------------------------

#define INVALID_DISK_ID       0xffff

//---Enumerations for public use-----------------------------------------------

typedef enum
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_OVERWRITE,
    FILE_MODE_SYNC_OVERWRITE
} E_STB_DSK_FILE_MODE;

typedef enum
{
    FILE_POSITION_START,
    FILE_POSITION_END,
    FILE_POSITION_CURRENT
} E_STB_DSK_FILE_POSITION;

typedef enum
{
    DIR_ENTRY_FILE,
    DIR_ENTRY_DIRECTORY,
    DIR_ENTRY_OTHER
} E_STB_DIR_ENTRY_TYPE;


//---Global type defs for public use-------------------------------------------

//---Global Function prototypes for public use---------------------------------
/**
 * @brief   Initialise the hard disk component
 */
void STB_DSKInitialise(void);

/**
 * @brief   Returns the number of disks currently detected
 * @return  Number of disks
 */
U16BIT STB_DSKGetNumDisks(void);

/**
 * @brief   Returns the id of the disk at the given index
 * @param   index zero based index
 * @return  Disk id, 0xffff if no disk found
 */
U16BIT STB_DSKGetDiskIdByIndex(U16BIT index);

/**
 * @brief   Checks if the given disk is removeable
 * @param   disk_id ID of the disk to be checked
 * @return  TRUE if the disk is removeable, FALSE otherwise
 */
BOOLEAN STB_DSKIsRemoveable(U16BIT disk_id);

/**
 * @brief   Checks if the given disk is mounted
 * @param   disk_id ID of the disk to be checked
 * @return  TRUE if the disk is mounted, FALSE otherwise
 */
BOOLEAN STB_DSKIsMounted(U16BIT disk_id);

/**
 * @brief   Attempts to mount the given disk, if it isn't already mounted
 * @param   disk_id ID of the disk to be mounted
 * @return  TRUE if the disk is mounted, FALSE otherwise
 */
BOOLEAN STB_DSKMountDisk(U16BIT disk_id);

/**
 * @brief   Attempts to unmount the given disk, if it isn't already unmounted
 * @param   disk_id ID of the disk to be unmounted
 * @return  TRUE if the disk is unmounted, FALSE otherwise
 */
BOOLEAN STB_DSKUnmountDisk(U16BIT disk_id);

/**
 * @brief   Gets the name of a disk and copies it into the array provided
 * @param   disk_id ID of the disk
 * @param   name array of U8BIT into which the name will be copied
 * @param   name_len max number of characters in the name
 * @return  TRUE if the disk is found, FALSE otherwise
 */
BOOLEAN STB_DSKGetDiskName(U16BIT disk_id, U8BIT *name, U16BIT name_len);

/**
 * @brief   Put all disks into or out of standby mode
 * @param   state standby mode, TRUE=Standby FALSE=On
 */
void STB_DSKSetStandby(BOOLEAN state);

/**
 * @brief   Put all disks into or out of refresh mode
 * @param   refresh refresh mode, TRUE=refresh disk FALSE=not
 */
void STB_DSKSetRefresh(BOOLEAN refresh);

/**
 * @brief   Opens an existing file or creates a new one
 * @param   name The filename (including path)
 * @param   mode The access mode
 * @return  The file handle
 */
void* STB_DSKOpenFile(U16BIT disk_id, U8BIT *name, E_STB_DSK_FILE_MODE mode);

/**
 * @brief   Reads data from an open file
 * @param   file The file handle
 * @param   data The caller's buffer
 * @param   size Number of bytes to be read
 * @return  Number of bytes successfully read
 */
U32BIT STB_DSKReadFile(void *file, U8BIT *data, U32BIT size);

/**
 * @brief   Writes data to an open file
 * @param   file The file handle
 * @param   data Pointer to the data to be written
 * @param   size Number of bytes to  write
 * @return  Number of bytes successfully written
 */
U32BIT STB_DSKWriteFile(void *file, U8BIT *data, U32BIT size);

/**
 * @brief   Sets the read/write position of an open file
 * @param   file The file handle
 * @param   position Position to move relative to, i.e. start,end,current
 * @param   offset Where to move to relative to position
 * @return  TRUE success, FALSE otherwise
 */
BOOLEAN STB_DSKSeekFile(void *file, E_STB_DSK_FILE_POSITION position, S32BIT offset);

/**
 * @brief   Gets the current position in an open file
 * @param   file The file handle
 * @param   offset Variable to contain result (byte position in file)
 * @return  TRUE on success
 */
BOOLEAN STB_DSKTellFile(void *file, U32BIT *offset);

/**
 * @brief   Flushes and closes an open file
 * @param   file The file handle
 */
void STB_DSKCloseFile(void *file);

/**
 * @brief   Deletes the file from the given disk
 * @param   disk_id disk ID
 * @param   filename pathname on the disk of the file to be deleted
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_DSKDeleteFile(U16BIT disk_id, U8BIT *filename);

/**
 * @brief   Checks whether a file/directory will the given name exists
 * @param   disk_id disk ID
 * @param   filename pathname on the disk of the file
 * @return  TRUE if exists, FALSE otherwise
 */
BOOLEAN STB_DSKFileExists(U16BIT disk_id, U8BIT *filename);

/**
 * @brief   Returns the size in KB of the given file
 * @param   disk_id disk on which the file exists
 * @param   filename name of the file on disk
 * @param   filesize returned value giving the file size in B
 * @return  TRUE if the file exists, FALSE otherwise
 */
BOOLEAN STB_DSKFileSize(U16BIT disk_id, U8BIT *filename, U32BIT *filesize);

/**
 * @brief   Opens a directory in order to read the entries
 * @param   disk_id disk containing to the directory to be read
 * @param   dir_name name of the directory to open
 * @return  Handle to be used in all other operations, NULL if the open fails
 */
void* STB_DSKOpenDirectory(U16BIT disk_id, U8BIT *dir_name);

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
                             E_STB_DIR_ENTRY_TYPE *entry_type);

/**
 * @brief   Closes the directory for reading
 * @param   dir directory handle
 */
void STB_DSKCloseDirectory(void *dir);

/**
 * @brief   Creates a directory with the given name
 * @param   disk_id disk on which the directory is to be created
 * @param   dir_name name of the directory to be created
 * @return  TRUE is the directory is successfully created
 */
BOOLEAN STB_DSKCreateDirectory(U16BIT disk_id, U8BIT *dir_path);

/**
 * @brief   Deletes a directory and all it contents, so use with care!
 * @param   disk_id disk
 * @param   dir_name name of the directory to be deleted
 */
BOOLEAN STB_DSKDeleteDirectory(U16BIT disk_id, U8BIT *dir_name);

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
BOOLEAN STB_DSKFullPathname(U16BIT disk_id, U8BIT *filename, U8BIT *pathname, U16BIT max_pathname_len);

/**
 * @brief   Returns the size (capacity) of the disk
 * @return  The size of the disk in kilobytes
 */
U32BIT STB_DSKGetSize(U16BIT disk_id);

/**
 * @brief   Returns the amount of space used on the disk
 * @param   disk_id ID of the disk
 * @return  Space used in kilobytes
 */
U32BIT STB_DSKGetUsed(U16BIT disk_id);

/**
 * @brief   Initiates formatting and partitioning of the hard disk. This will erase all data on the
 *          disk!
 */
void STB_DSKFormat(U16BIT disk_id);

/**
 * @brief   Gets the progress of the format operation
 * @return  The progress as percent complete
 */
U8BIT STB_DSKGetFormatProgress(U16BIT disk_id);

/**
 * @brief   Queries whether the disk is formatted
 * @return  FALSE unformatted
 */
BOOLEAN STB_DSKIsFormatted(U16BIT disk_id);

/**
 * @brief   Initiates a data repair of the hard disk. This *may* cause the disk to become unreadable
 */
void STB_DSKRepair(U16BIT disk_id);

/**
 * @brief   Gets the progress of the repair operation
 * @return  The progress as percent complete
 */
U8BIT STB_DSKGetRepairProgress(U16BIT disk_id);

/**
 * @brief   Returns a summary of the disk integrity
 * @return  TRUE if disk integrity ok
 */
BOOLEAN STB_DSKGetIntegrity(U16BIT disk_id);

/**
 * @brief   Add a device path
 * @return  TRUE if add ok
 */
BOOLEAN STB_DSKAddDevicePath(char *device, char *path, U16BIT *p_disk_id);

/**
 * @brief   Load the data in the device path
 * @return  TRUE if ok
 */
BOOLEAN STB_DSKLoadDevicePath(U16BIT disk_id);

void STB_DSKCheckSpace(U16BIT disk_id);

/**
 * @brief Gets various disk basic info.
 *
 * @param [in] disk_id ID of the disk
 * @param [out] dev_name device name buffer. Caller is responsible for managing its lifecycle.
 * @param [in] name_len device name buffer length.
 * @param [out] mount_path device mount path buffer. Caller is responsible for manage its lifecycle.
 * @param [in] path_len mount path buffer length.
 * @param [out] used_in_kb a pointer to a U32BIT containing used disk space in kb.
 * @param [out] size_in_kb a pointer to a U32BIT containing total disk space in kb.
 * @param [out] is_removeable a pointer to a BOOLEAN indicating whether disk is removable.
 * @return TRUE if the disk is mounted and all output values are meaningful, FALSE otherwise.
 */
BOOLEAN STB_DSKGetDiskInfo(U16BIT disk_id, U8BIT* dev_name, U8BIT name_len,
      U8BIT* mount_path, U8BIT path_len, U32BIT *used_in_kb, U32BIT *size_in_kb,
      BOOLEAN *is_removeable);

#endif //  _STBHWDSK_H

