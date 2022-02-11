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
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* third party header files */
/* STB Header Files */
#include "techtype.h"
#include "dbgfuncs.h"

#include "stbhwc.h"
#include "stbhwos.h"
#include "stbhwmem.h"
#include "stbhwdsk.h"


/*---macro definitions for this file-----------------------------------------*/
//#define DISK_DEBUG_LOOP 1
#ifdef  DISK_DEBUG_LOOP
   #define  DISK_DBGLOOP(x,...)      STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define  DISK_DBGLOOP(x,...)
#endif


#define DISK_DEBUG 1
#ifdef  DISK_DEBUG
   #define  DISK_DBG(x,...)      STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define  DISK_DBG(x,...)
#endif

#define  DISK_ERR(x,...)         STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

#if 0
#define STB_OSMutexLock(_m)\
   do {\
      STB_OSMutexLock(_m);\
      STB_SPDebugWrite("%s:%d lock",__FUNCTION__,__LINE__);\
   } while (0)

#define STB_OSMutexUnlock(_m)\
   do {\
      STB_SPDebugWrite("%s:%d unlock",__FUNCTION__,__LINE__);\
      STB_OSMutexUnlock(_m);\
   } while (0)
#endif

int add_flag = 0;

/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/
typedef struct s_disk_info
{
   struct s_disk_info* next;
   char *device_name;
   char *mount_path;
   BOOLEAN is_removeable;
   U16BIT disk_id;
   U32BIT disk_size;
   BOOLEAN blocked;
   BOOLEAN found;
} S_DISK_INFO;


/*---local (static) variable declarations for this file----------------------*/
static S_DISK_INFO* disk_list;
static void* disk_mutex;
static U8BIT next_disk_id = 0;


/*---local function prototypes for this file---------------------------------*/
static void DiskMonitorTask(void *param);
static void RefreshDiskList(BOOLEAN send_events);
static BOOLEAN SupportedFSType(char *fs_type);
static S_DISK_INFO* AddDisk(char *device_name, char *mount_path);
static void RemoveDisk(S_DISK_INFO* del_disk);
static S_DISK_INFO* FindDisk(U16BIT disk_id);
static BOOLEAN STB_DSKAddDevicePathAndLoad(char *device, char *path, BOOLEAN load);

/*---global function definitions----------------------------------------------*/

/**
 * @brief   Initialise the hard disk component
 */
void STB_DSKInitialise(void)
{
   FUNCTION_START(STB_DSKInitialise);

   if (disk_mutex == NULL)
   {
      disk_list = NULL;
      disk_mutex = STB_OSCreateMutex();
      if (disk_mutex != NULL)
      {
         /* Create a background task to monitor the disks */
         if (STB_OSCreateTask(DiskMonitorTask, NULL, 8192, 7, (U8BIT*)"DiskMonitor") == NULL)
         {
            DISK_ERR("Failed to create disk monitor background task");
         }
      }
   }

   FUNCTION_FINISH(STB_DSKInitialise);
}

/**
 * @brief   Returns the number of disks currently detected
 * @return  Number of disks
 */
U16BIT STB_DSKGetNumDisks(void)
{
   U16BIT num_disks;
   S_DISK_INFO* disk;

   FUNCTION_START(STB_DSKGetNumDisks);

   STB_OSMutexLock(disk_mutex);

   for (disk = disk_list, num_disks = 0; disk != NULL; disk = disk->next)
   {
      num_disks++;
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKGetNumDisks);

   return(num_disks);
}

/**
 * @brief   Returns the id of the disk at the given index
 * @param   index zero based index
 * @return  Disk id, 0 if no disk found
 */
U16BIT STB_DSKGetDiskIdByIndex(U16BIT index)
{
   U16BIT disk_id = INVALID_DISK_ID;
   S_DISK_INFO* disk;

   FUNCTION_START(STB_DSKGetDiskIdByIndex);

   STB_OSMutexLock(disk_mutex);

   for (disk = disk_list; (index > 0) && (disk != NULL); index--)
   {
      disk = disk->next;
   }

   if (disk != NULL)
   {
      disk_id = disk->disk_id;
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKGetDiskIdByIndex);

   return(disk_id);
}

/**
 * @brief   Checks if the given disk is removeable
 * @param   disk_id ID of the disk to be checked
 * @return  TRUE if the disk is removeable, FALSE otherwise
 */
BOOLEAN STB_DSKIsRemoveable(U16BIT disk_id)
{
   BOOLEAN retval;
   S_DISK_INFO* disk;

   FUNCTION_START(STB_DSKIsRemoveable);

   STB_OSMutexLock(disk_mutex);

   disk = FindDisk(disk_id);
   if (disk != NULL)
   {
      retval = disk->is_removeable;
   }
   else
   {
      retval = FALSE;
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKIsRemoveable);

   return(retval);
}

/**
 * @brief   Checks if the given disk is mounted
 * @param   disk_id ID of the disk to be checked
 * @return  TRUE if the disk is mounted, FALSE otherwise
 */
BOOLEAN STB_DSKIsMounted(U16BIT disk_id)
{
   BOOLEAN retval;

   FUNCTION_START(STB_DSKIsMounted);

   STB_OSMutexLock(disk_mutex);

   /* All disks are mounted if they're in the list */
   if (FindDisk(disk_id) != NULL)
   {
      retval = TRUE;
   }
   else
   {
      retval = FALSE;
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKIsMounted);

   return(retval);
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

   /* Disks can't be mounted directly as this is an Android function */
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

   /* Disks can't be unmounted directly as this is an Android function */
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

   /* Disk name isn't supported */
   return FALSE;
}

/**
 * @brief   Returns the amount of space used on the disk
 * @param   disk_id ID of the disk
 * @return  Space used in kilobytes
 */
U32BIT STB_DSKGetUsed(U16BIT disk_id)
{
   U32BIT disk_used = 0;
   S_DISK_INFO* disk;
   struct statfs fs_info;
   U32BIT blocks_used;

   FUNCTION_START(STB_DSKGetUsed);

   STB_OSMutexLock(disk_mutex);

   disk = FindDisk(disk_id);
   
   if (disk != NULL)
   {
      /* Found the disk */
      if (statfs(disk->mount_path, &fs_info) == 0)
      {
         /* Calculate the disk space used in KB */
         blocks_used = fs_info.f_blocks - fs_info.f_bavail;

         if ((fs_info.f_bsize > 1024) && ((fs_info.f_bsize % 1024) == 0))
         {
            disk_used = (fs_info.f_bsize / 1024) * blocks_used;
         }
         else
         {
            disk_used = (fs_info.f_bsize * blocks_used) / 1024;
         }
         DISK_ERR("disk used 0x%04x, path %s [%d][%d][%u]", disk_id, disk->mount_path,blocks_used, fs_info.f_bsize, disk_used);
      }
      else
      {
         DISK_ERR("Failed to get filesystem info on disk 0x%04x, errno %d", disk_id, errno);
      }
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKGetUsed);

   return(disk_used);
}

/**
 * @brief   Queries whether the disk is formatted
 * @return  TRUE if formatted, FALSE otherwise
 */
BOOLEAN STB_DSKIsFormatted(U16BIT disk_id)
{
   BOOLEAN retval;

   FUNCTION_START(STB_DSKIsFormatted);

   STB_OSMutexLock(disk_mutex);

   /* Any disks in the list are mounted, so will be formatted */
   if (FindDisk(disk_id) != NULL)
   {
      retval = TRUE;
   }
   else
   {
      retval = FALSE;
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKIsFormatted);

   return(retval);
}

/**
 * @brief   Returns the size (capacity) of the disk
 * @return  The size of the disk in kilobytes
 */
U32BIT STB_DSKGetSize(U16BIT disk_id)
{
   U32BIT disk_size;
   S_DISK_INFO* disk;

   FUNCTION_START(STB_DSKGetSize);

   STB_OSMutexLock(disk_mutex);

   disk = FindDisk(disk_id);
   if (disk != NULL)
   {
      disk_size = disk->disk_size;
   }
   else
   {
      disk_size = disk->disk_size;
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKGetSize);

   return(disk_size);
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
 * @return  TRUE if disk integrity ok
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
   BOOLEAN retval = FALSE;
   S_DISK_INFO* disk;

   FUNCTION_START(STB_DSKFullPathname);

   STB_OSMutexLock(disk_mutex);

   disk = FindDisk(disk_id);
   if (disk != NULL)
   {
      if ((filename == NULL) ||
         (strlen((char*)filename) + strlen(disk->mount_path) + 2 <= max_pathname_len))
      {
         if ((filename == NULL) || (strlen((char *)filename) == 0))
         {
            snprintf((char*)pathname, max_pathname_len, "%s", disk->mount_path);
         }
         else
         {
            snprintf((char*)pathname, max_pathname_len, "%s/%s", disk->mount_path, filename);
         }
         retval = TRUE;
      }
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKFullPathname);

   return(retval);
}

/**
 * @brief   Opens an existing file or creates a new one
 * @param   name The filename (including path)
 * @param   mode The access mode
 * @return  The file handle
 */
void* STB_DSKOpenFile(U16BIT disk_id, U8BIT *name, E_STB_DSK_FILE_MODE mode)
{
   FILE* file;
   S_DISK_INFO* disk;
   int pathlen;
   char* fullpath;

   FUNCTION_START(STB_DSKOpenFile);

   file = NULL;

   STB_OSMutexLock(disk_mutex);

   disk = FindDisk(disk_id);
   if (disk != NULL)
   {
      pathlen = strlen(disk->mount_path) + strlen((char*)name) + 2;

      fullpath = (char *)STB_MEMGetSysRAM(pathlen);
      if (fullpath != NULL)
      {
         snprintf(fullpath, pathlen, "%s/%s", disk->mount_path, name);

         switch (mode)
         {
            case FILE_MODE_READ :
               file = fopen((const char*)fullpath, "rb");
               break;

            case FILE_MODE_WRITE :
               file = fopen((const char*)fullpath, "rb+");
               break;

            case FILE_MODE_OVERWRITE :
               file = fopen((const char*)fullpath, "wb");
               break;

            default :
               DISK_ERR("Unknown file mode %u for file %s", mode, name);
               break;
         }

         if (file != NULL)
         {
            //DISK_DBG("Opened file %s: %p", fullpath, file);
         }
         else
         {
            DISK_DBG("Failed to open %s, error %d", fullpath, errno);
         }

         STB_MEMFreeSysRAM(fullpath);
      }
   }
   else
   {
      DISK_DBG("Failed to find disk with ID 0x%x", disk_id);
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKOpenFile);

   return((void *)file);
}

/**
 * @brief   Flushes and closes an open file
 * @param   file The file handle
 */
void STB_DSKCloseFile(void *file)
{
   FILE *fp;

   FUNCTION_START(STB_DSKCloseFile);

   if (file != NULL)
   {
      fp = (FILE *)file;
      fflush(fp);
      fsync(fileno(fp));

      fclose(fp);

      //DISK_DBG("Closed %p", file);
   }

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
   U32BIT num_bytes;

   FUNCTION_START(STB_DSKReadFile);

   if (file != NULL)
   {
      num_bytes = (U32BIT)fread((void*)data, 1, (size_t)size, (FILE*)file);
   }
   else
   {
      num_bytes = 0;
   }

   FUNCTION_FINISH(STB_DSKReadFile);

   return(num_bytes);
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
   U32BIT num_bytes;

   FUNCTION_START(STB_DSKWriteFile);

   if (file != NULL)
   {
      num_bytes = (U32BIT)fwrite((void*)data, 1, (size_t)size, (FILE*)file);
   }
   else
   {
      num_bytes = 0;
   }

   FUNCTION_FINISH(STB_DSKWriteFile);

   return(num_bytes);
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
   BOOLEAN retval;

   FUNCTION_START(STB_DSKSeekFile);

   retval = FALSE;

   if (file != NULL)
   {
      switch (position)
      {
         case FILE_POSITION_START :
         {
            if (fseek((FILE*)file, offset, SEEK_SET) == 0)
            {
               retval = TRUE;
            }
            break;
         }

         case FILE_POSITION_END :
         {
            if (fseek((FILE*)file, offset, SEEK_END) == 0)
            {
               retval = TRUE;
            }
            break;
         }

         case FILE_POSITION_CURRENT :
         {
            if ((offset == 0L) || (fseek((FILE*)file, offset, SEEK_CUR) == 0))
            {
               retval = TRUE;
            }
            break;
         }

         default :
         {
            break;
         }
      }
   }

   FUNCTION_FINISH(STB_DSKSeekFile);

   return(retval);
}

/**
 * @brief   Gets the current position in an open file
 * @param   file The file handle
 * @param   offset Variable to contain result (byte position in file)
 * @return  TRUE on success
 */
BOOLEAN STB_DSKTellFile(void *file, U32BIT *offset)
{
   BOOLEAN retval;

   FUNCTION_START(STB_DSKTellFile);

   if (file != NULL)
   {
      *offset = (U32BIT)ftell((FILE *)file);
      retval = TRUE;
   }
   else
   {
      retval = FALSE;
   }

   FUNCTION_FINISH(STB_DSKTellFile);

   return(retval);
}

/**
 * @brief   Deletes the file from the given disk
 * @param   disk_id disk ID
 * @param   filename pathname on the disk of the file to be deleted
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_DSKDeleteFile(U16BIT disk_id, U8BIT *filename)
{
   BOOLEAN retval;
   S_DISK_INFO* disk;
   int pathlen;
   char* fullpath;

   FUNCTION_START(STB_DSKDeleteFile);

   retval = FALSE;

   STB_OSMutexLock(disk_mutex);

   disk = FindDisk(disk_id);
   if (disk != NULL)
   {
      pathlen = strlen(disk->mount_path) + strlen((char*)filename) + 2;
      fullpath = (char *)STB_MEMGetSysRAM(pathlen);
      if (fullpath != NULL)
      {
         snprintf(fullpath, pathlen, "%s/%s", disk->mount_path, filename);
         if (unlink(fullpath) == 0)
         {
            retval = TRUE;
         }
         else
         {
            DISK_DBG("Failed to delete %s, errno %d", fullpath, errno);
         }

         STB_MEMFreeSysRAM(fullpath);
      }
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKDeleteFile);

   return(retval);
}

/**
 * @brief   Checks whether a file/directory will the given name exists
 * @param   disk_id disk ID
 * @param   filename pathname on the disk of the file
 * @return  TRUE if exists, FALSE otherwise
 */
BOOLEAN STB_DSKFileExists(U16BIT disk_id, U8BIT *filename)
{
   BOOLEAN retval;
   S_DISK_INFO* disk;
   int pathlen;
   char* fullpath;
   struct stat statbuf;

   FUNCTION_START(STB_DSKFileExists);

   retval = FALSE;

   STB_OSMutexLock(disk_mutex);

   disk = FindDisk(disk_id);
   if (disk != NULL)
   {
      pathlen = strlen(disk->mount_path) + strlen((char*)filename) + 2;

      fullpath = (char *)STB_MEMGetSysRAM(pathlen);
      if (fullpath != NULL)
      {
         snprintf(fullpath, pathlen, "%s/%s", disk->mount_path, filename);

         if (stat(fullpath, &statbuf) == 0)
         {
            retval = TRUE;
         }

         STB_MEMFreeSysRAM(fullpath);
      }
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKFileExists);

   return(retval);
}

/**
 * @brief   Returns the size in KB of the given file
 * @param   disk_id disk on which the file exists
 * @param   filename name of the file on disk
 * @param   filesize returned value giving the file size in B
 * @return  TRUE if the file exists, FALSE otherwise
 */
BOOLEAN STB_DSKFileSize(U16BIT disk_id, U8BIT *filename, U32BIT *filesize)
{
   BOOLEAN retval;
   S_DISK_INFO* disk;
   int pathlen;
   char* fullpath;
   struct stat statbuf;

   FUNCTION_START(STB_DSKFileSize);

   retval = FALSE;

   STB_OSMutexLock(disk_mutex);

   disk = FindDisk(disk_id);
   if (disk != NULL)
   {
      pathlen = strlen(disk->mount_path) + strlen((char*)filename) + 2;

      fullpath = (char *)STB_MEMGetSysRAM(pathlen);
      if (fullpath != NULL)
      {
         snprintf(fullpath, pathlen, "%s/%s", disk->mount_path, filename);

         if (stat(fullpath, &statbuf) == 0)
         {
            *filesize = statbuf.st_size;
            retval = TRUE;
         }

         STB_MEMFreeSysRAM(fullpath);
      }
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKFileSize);

   return(retval);
}

/**
 * @brief   Opens a directory in order to read the entries
 * @param   disk_id disk containing to the directory to be read
 * @param   dir_name name of the directory to open
 * @return  Handle to be used in all other operations, NULL if the open fails
 */
void* STB_DSKOpenDirectory(U16BIT disk_id, U8BIT *dir_name)
{
   U8BIT* fullpath;
   U16BIT path_len;
   S_DISK_INFO* disk;
   DIR* dir;

   FUNCTION_START(STB_DSKOpenDirectory);

   dir = NULL;

   STB_OSMutexLock(disk_mutex);

   /* Get the full pathname of the directory to be opened */
   disk = FindDisk(disk_id);
   if (disk != NULL)
   {
      path_len = strlen((char*)dir_name) + strlen(disk->mount_path) + 3;
      fullpath = (U8BIT*)STB_MEMGetSysRAM(path_len);

      if (fullpath != NULL)
      {
         if (STB_DSKFullPathname(disk_id, dir_name, fullpath, path_len))
         {
            dir = opendir((char*)fullpath);

            if (dir == NULL)
            {
               DISK_ERR("Failed to open directory \"%s\" on disk 0x%x, errno %d",
                  fullpath, disk_id, errno);
            }
         }

         STB_MEMFreeSysRAM(fullpath);
      }
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKOpenDirectory);

   return(dir);
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
   BOOLEAN retval;
   struct dirent* entry;

   FUNCTION_START(STB_DSKReadDirectory);

   retval = FALSE;

   if (dir != NULL)
   {
      entry = readdir((DIR*)dir);
      if (entry != NULL)
      {
         strncpy((char*)filename, entry->d_name, filename_len - 1);
         filename[filename_len - 1] = '\0';

         switch (entry->d_type)
         {
            case 0:
            case 8:
               *entry_type = DIR_ENTRY_FILE;
               break;

            case 1:
               *entry_type = DIR_ENTRY_DIRECTORY;
               break;

            default:
               *entry_type = DIR_ENTRY_OTHER;
               break;
         }

         retval = TRUE;
      }
   }

   FUNCTION_FINISH(STB_DSKReadDirectory);

   return(retval);
}

/**
 * @brief   Closes the directory for reading
 * @param   dir directory handle
 */
void STB_DSKCloseDirectory(void *dir)
{
   FUNCTION_START(STB_DSKCloseDirectory);

   if (dir != NULL)
   {
      closedir((DIR*)dir);
   }

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
   S_DISK_INFO* disk;
   int pathlen;
   char* pathname;
   char* dirname;
   struct stat statbuf;
   BOOLEAN retval;

   FUNCTION_START(STB_DSKCreateDirectory);

   retval = FALSE;

   STB_OSMutexLock(disk_mutex);

   disk = FindDisk(disk_id);
   if (disk != NULL)
   {
      pathlen = strlen(disk->mount_path) + strlen((char*)dir_path) + 2;

      pathname = (char*)STB_MEMGetSysRAM(pathlen);
      if (pathname != NULL)
      {
         retval = TRUE;
         dirname = (char*)dir_path;

         /* Create all directories in the path upto the final name */
         while ((dirname = strchr(dirname, '/')) != NULL)
         {
            /* Terminate the directory path at the next slash char */
            *dirname = '\0';

            snprintf(pathname, pathlen, "%s/%s", disk->mount_path, dir_path);

            /* Check whether the directory already exists */
            if (stat((char*)pathname, &statbuf) != 0)
            {
               /* Create the directory up to this point */
               if (mkdir((char*)pathname, (S_IRWXU | S_IRWXG | S_IRWXO)) != 0)
               {
                  /* Restore the slash char */
                  *dirname = '/';
                  retval = FALSE;
                  break;
               }
            }

            /* Restore the slash char and move to the next char to start search for the next one */
            *dirname = '/';
            dirname++;
         }

         if (retval)
         {
            snprintf(pathname, pathlen, "%s/%s", disk->mount_path, dir_path);

            /* Check whether the directory already exists */
            if (stat((char*)pathname, &statbuf) != 0)
            {
               /* Now create the final directory in the path */
               if (mkdir((char*)pathname, (S_IRWXU | S_IRWXG | S_IRWXO)) != 0)
               {
                  retval = FALSE;
               }
            }
         }

         STB_MEMFreeSysRAM(pathname);
      }
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKCreateDirectory);

   return(retval);
}

/**
 * @brief   Deletes a directory and all it contents, so use with care!
 * @param   disk_id disk
 * @param   dir_name name of the directory to be deleted
 */
BOOLEAN STB_DSKDeleteDirectory(U16BIT disk_id, U8BIT *dir_name)
{
   S_DISK_INFO* disk;
   int pathlen;
   char* dir_path;
   char* pathname;
   DIR* dp;
   struct dirent* entry;
   struct stat statbuf;
   BOOLEAN retval;

   FUNCTION_START(STB_DSKDeleteDirectory);

   retval = FALSE;

   STB_OSMutexLock(disk_mutex);

   DISK_DBG("Delete directory \"%s\" on disk 0x%04x", dir_name, disk_id);

   disk = FindDisk(disk_id);
   if (disk != NULL)
   {
      pathlen = strlen(disk->mount_path) + strlen((char*)dir_name) + 2;

      dir_path = (char*)STB_MEMGetSysRAM(pathlen);
      if (dir_path != NULL)
      {
         snprintf(dir_path, pathlen, "%s/%s", disk->mount_path, dir_name);

         dp = opendir((char*)dir_path);
         if (dp != NULL)
         {
            retval = TRUE;

            while ((entry = readdir(dp)) != NULL)
            {
               if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0))
               {
                  pathlen = strlen((char*)dir_path) + strlen(entry->d_name) + 2;

                  pathname = (char*)STB_MEMGetSysRAM(pathlen);
                  if (pathname != NULL)
                  {
                     snprintf(pathname, pathlen, "%s/%s", (char*)dir_path, entry->d_name);

                     if (stat(pathname, &statbuf) == 0)
                     {
                        if (S_ISDIR(statbuf.st_mode))
                        {
                           /* Recurse down into this directory to delete its contents */
                           DISK_DBG("Recursing into \"%s\"...", pathname);
                           if (!STB_DSKDeleteDirectory(disk_id, (U8BIT*)pathname))
                           {
                              retval = FALSE;
                           }
                        }
                        else
                        {
                           DISK_DBG("Deleting \"%s\"", pathname);
                           if (unlink(pathname) != 0)
                           {
                              /* Failed to remove an item but continue trying to delete the rest */
                              retval = FALSE;
                              DISK_DBG("Failed to delete \"%s\", errno %d", pathname, errno);
                           }
                        }
                     }

                     STB_MEMFreeSysRAM(pathname);
                  }
               }
            }

            closedir(dp);

            if (rmdir((char*)dir_path) != 0)
            {
               retval = FALSE;
               DISK_DBG("Failed to delete directory \"%s\", errno %d", dir_path, errno);
            }

            sync();
         }

         STB_MEMFreeSysRAM(dir_path);
      }
   }

   STB_OSMutexUnlock(disk_mutex);

   FUNCTION_FINISH(STB_DSKDeleteDirectory);

   return(retval);
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

BOOLEAN STB_DSKAddDevicePath(char *device, char *path)
{
   return STB_DSKAddDevicePathAndLoad(device, path, TRUE);
}

void STB_DSKCheckSpace(U16BIT disk_id)
{
    U32BIT free = STB_DSKGetSize(disk_id) - STB_DSKGetUsed(disk_id);
    DISK_DBG("Disk: id[0x%x] free[%uKB] total[%u]", disk_id, free, STB_DSKGetSize(disk_id));
    if (free < STB_PVRGetMinDiskSpaceLeft())
    {
       DISK_DBG("Disk: Exceed the free space limit[%uKB] for PVR, now[%uKB]",
          STB_PVRGetMinDiskSpaceLeft(), free);
       STB_OSSendEvent(FALSE, HW_EV_CLASS_DISK, HW_EV_TYPE_DISK_FULL,
          &disk_id, sizeof(U16BIT));
    }
}

/*---local function definitions----------------------------------------------*/
#define SUBOF(_path1, _path2) \
      ((strlen(_path1) > strlen(_path2)) \
      && !strncmp((_path1), (_path2), strlen(_path2)))

static BOOLEAN STB_DSKAddDevicePathAndLoad(char *device, char *path, BOOLEAN load)
{

   BOOLEAN send_events = load;
   char device_name[256];
   char mount_path[256];
   char fs_type[32];
   char read_write[8];
   S_DISK_INFO* disk;
   S_DISK_INFO* next_disk;
   BOOLEAN added = FALSE;

   STB_OSMutexLock(disk_mutex);

   for (disk = disk_list; (disk != NULL) &&
      (/*(strcmp(disk->device_name, device) != 0) ||*/ (strcmp(disk->mount_path, path) != 0)); )
   {
      /*DISK_DBG("Existed disk: %s, mounted on %s", disk->device_name, disk->mount_path);*/
      disk = disk->next;
   }

   if (disk != NULL)
   {
      DISK_DBG("Existed disk: %s, mounted on %s, removeable %s", disk->device_name, disk->mount_path, disk->is_removeable? "true" : "false");
   }
   else
   {
      //init device name and mount path
      memset(device_name, 0, sizeof(device_name));
      memset(mount_path, 0, sizeof(mount_path));
      strncpy(device_name, device, sizeof(device_name) - 1);
      strncpy(mount_path, path, sizeof(mount_path) - 1);

      /* Add this disk to the list */
      disk = AddDisk(device_name, mount_path);

      /* create if user added and not exists */
      if (disk != NULL)
      {
         void *dir = STB_DSKOpenDirectory(disk->disk_id, "");
         if (dir == NULL)
         {
            DISK_DBG("AddDisk %s: creating folder %s", device_name, mount_path);
            if (!STB_DSKCreateDirectory(disk->disk_id, ""))
            {
               DISK_DBG("AddDisk: failed creating folder %s", mount_path);
               RemoveDisk(disk);
               disk = NULL;
            }
            else
            {
               struct statfs fs_info;
               int retval = statfs(disk->mount_path, &fs_info);

               if (retval == 0)
               {
                  /* Calculate the disk size in KB. It's done various ways to avoid overflow */
                  if ((fs_info.f_bsize > 1024) && ((fs_info.f_bsize % 1024) == 0))
                  {
                     disk->disk_size = (fs_info.f_bsize / 1024) * fs_info.f_blocks;
                  }
                  else
                  {
                     disk->disk_size = (fs_info.f_bsize * fs_info.f_blocks) / 1024;
                  }
               }
               else
               {
                  DISK_ERR("Failed to get disk info to calculate the size, errno %d", errno);
               }
            }
         }
         else
         {
            STB_DSKCloseDirectory(dir);
         }
      }

      if (disk != NULL)
      {
         added = TRUE;

         DISK_DBG("Added disk %s, mounted on %s, ID 0x%04x, size %lu KB, removeable %s",
            disk->device_name, disk->mount_path, disk->disk_id, disk->disk_size, disk->is_removeable? "true":"false");

         if (send_events)
         {
            STB_OSMutexUnlock(disk_mutex);

            /* Send an event to indicate a device has been attached */
            STB_OSSendEvent(FALSE, HW_EV_CLASS_DISK, HW_EV_TYPE_DISK_CONNECTED,
               &(disk->disk_id), sizeof(disk->disk_id));

            STB_OSMutexLock(disk_mutex);
         }
      }
   }

   STB_OSMutexUnlock(disk_mutex);

   return (added);
}

static void DiskMonitorTask(void *param)
{
   USE_UNWANTED_PARAM(param);

   /* Create the initial list of disks, but don't send events on start up */
#ifdef DTVKIT_IN_VENDOR_PARTITION
   STB_DSKAddDevicePathAndLoad("user", "/data/vendor/dtvkit", FALSE);
#else
   STB_DSKAddDevicePathAndLoad("user", "/data/data/com.droidlogic.dtvkit.inputsource", FALSE);
#endif

   RefreshDiskList(FALSE);

   while (TRUE)
   {
      /* Run the task every 3 seconds */
      STB_OSTaskDelay(3000);

      RefreshDiskList(TRUE);
      DISK_DBG("check disk start");
      /*check for the free space of the disk which has recording running*/
      STB_PVRCheckDiskSpace();
   }
}

static void RefreshDiskList(BOOLEAN send_events)
{
   FILE* fp;
   char device_name[128];
   char mount_path[128];
   char fs_type[32];
   char read_write[8];
   S_DISK_INFO* disk;
   S_DISK_INFO* next_disk;

   /* Read the partition list to see if there are any new disks */
   fp = fopen("/proc/mounts", "r");
   if (fp != NULL)
   {
      STB_OSMutexLock(disk_mutex);

      /* Mark all disks as not found so that any that have been removed can be detected */
      for (disk = disk_list; disk != NULL; disk = disk->next)
      {
         if (disk->is_removeable) {
            disk->found = FALSE;
         }
      }

      STB_OSMutexUnlock(disk_mutex);

      while (fscanf(fp, "%127s %127s %31s %7[^,] %*[^\r\n]\n", device_name, mount_path, fs_type, read_write) == 4)
      {
         /*DISK_DBGLOOP(" dev=\"%s\", mnt=\"%s\", fs=\"%s\", rw=\"%s\"\n", device_name, mount_path, fs_type, read_write);*/

         /* Check to see if the device is one of the filesystem types used for PVR
          * and it's mounted for read/write access */
         if (SupportedFSType(fs_type) && (strcmp(read_write, "rw") == 0))
         {
            BOOLEAN found = FALSE;

            /* Check to see if this disk is already known */
            STB_OSMutexLock(disk_mutex);

            for (disk = disk_list; disk != NULL; disk = disk->next)
            {
               if (strcmp(disk->mount_path, mount_path) == 0)
               {
                  disk->found = TRUE;
                  found = TRUE;
                  DISK_DBGLOOP("found: dev:%s mnt:%s remove:%d", disk->device_name, disk->mount_path, disk->is_removeable);
               }
               else if (SUBOF(disk->mount_path, mount_path))
               {
                  disk->found = TRUE;
                  DISK_DBGLOOP("found: sub: dev:%s mnt:%s remove:%d", disk->device_name, disk->mount_path, disk->is_removeable);
               }
            }

            if (found == FALSE)
            {
               /* Add this disk to the list */
               disk = AddDisk(device_name, mount_path);

               if (disk != NULL)
               {
                  DISK_DBGLOOP("Added %s disk %s, mounted on %s, ID 0x%04x, size %lu KB removeable %s", fs_type,
                     disk->device_name, disk->mount_path, disk->disk_id, disk->disk_size, disk->is_removeable? "true":"false");

                  if (send_events)
                  {
                     STB_OSMutexUnlock(disk_mutex);

                     /* Send an event to indicate a device has been attached */
                     STB_OSSendEvent(FALSE, HW_EV_CLASS_DISK, HW_EV_TYPE_DISK_CONNECTED,
                        &(disk->disk_id), sizeof(disk->disk_id));

                     STB_OSMutexLock(disk_mutex);
                  }
               }
            }

            STB_OSMutexUnlock(disk_mutex);
         }

      }

      fclose(fp);

      STB_OSMutexLock(disk_mutex);

      /* Now check all disks and remove any that weren't found in the file */
      for (disk = disk_list; disk != NULL; )
      {
         if (!disk->found)
         {
            U16BIT disk_id = disk->disk_id;
            DISK_DBGLOOP("Removed disk 0x%04x, mounted on %s", disk->disk_id, disk->mount_path);

            /* Now the disk has disappeared, delete it from the list of known disks */
            next_disk = disk->next;
            RemoveDisk(disk);
            disk = next_disk;

            if (send_events)
            {
               STB_OSMutexUnlock(disk_mutex);

               /* Send an event to indicate a device has been removed */
               STB_OSSendEvent(FALSE, HW_EV_CLASS_DISK, HW_EV_TYPE_DISK_REMOVED,
                  &disk_id, sizeof(disk_id));

               STB_OSMutexLock(disk_mutex);
            }
         }
         else
         {
            disk = disk->next;
         }
      }

      STB_OSMutexUnlock(disk_mutex);
   }
}

static BOOLEAN SupportedFSType(char *fs_type)
{
   BOOLEAN retval;

   if ((strcmp(fs_type, "vfat") == 0) || (strcmp(fs_type, "exfat") == 0) ||
      (strcmp(fs_type, "ext2") == 0) || (strcmp(fs_type, "ext3") == 0) ||
      (strcmp(fs_type, "ext4") == 0) || (strcmp(fs_type, "fuseblk") == 0))
   {
      retval = TRUE;
   }
   else
   {
      retval = FALSE;
   }

   return(retval);
}

static S_DISK_INFO* AddDisk(char *device_name, char *mount_path)
{
   S_DISK_INFO *disk;
   S_DISK_INFO *disk_ptr;
   int major, minor;
   struct statfs fs_info;
   int retval;

   disk = (S_DISK_INFO*)STB_MEMGetSysRAM(sizeof(S_DISK_INFO));
   if (disk != NULL)
   {
      memset(disk, 0, sizeof(S_DISK_INFO));

      disk->device_name = (char *)STB_MEMGetSysRAM(strlen(device_name) + 1);
      if (disk->device_name != NULL)
      {
         strcpy(disk->device_name, device_name);

         disk->mount_path = (char *)STB_MEMGetSysRAM(strlen(mount_path) + 1);
         if (disk->mount_path != NULL)
         {
            strcpy(disk->mount_path, mount_path);

            /* If major/minor IDs are available then they're included in the device name */
            if (sscanf(device_name, "%*[^:]:%d,%d", &major, &minor) == 2)
            {
               disk->disk_id = (U16BIT)(major << 8) + minor;

               /* As this disk has major/minor IDs, we'll assume it's a real disk that can be removed */
               disk->is_removeable = TRUE;
            }
            else
            {
               /* Use a generated ID for the disk */
               next_disk_id++;
               disk->disk_id = next_disk_id;

               /* Assume this is some form of system partition and so it can't be removed */
               disk->is_removeable = FALSE;
            }

            /* renew the info of the user added path */
            {
               S_DISK_INFO *d;

               for (d = disk_list; d != NULL; d = d->next)
               {
                  /* sync from existed non-user-add disk */
                  if (strcmp(disk->device_name, "user") == 0)
                  {
                     if ((strcmp(d->device_name, "user") != 0)
                        && SUBOF(disk->mount_path, d->mount_path)
                        && (disk->is_removeable != d->is_removeable))
                     {
                        disk->is_removeable = d->is_removeable;
                        DISK_DBG("Changed disk %s, mount on %s, removeable %s",
                           disk->device_name, disk->mount_path, disk->is_removeable? "true" : "false");
                        break;
                     }
                  }
                  /* sync to existed user-add disks */
                  else
                  {
                     if ((strcmp(d->device_name, "user") == 0)
                        && SUBOF(d->mount_path, disk->mount_path)
                        && (d->is_removeable != disk->is_removeable))
                     {
                        d->is_removeable = disk->is_removeable;
                        DISK_DBG("Changed disk %s, mount on %s, removeable %s",
                           d->device_name, d->mount_path, d->is_removeable? "true" : "false");
                     }
                  }
               }
            }

            disk->found = TRUE;
            disk->blocked = FALSE;

            /* Get the size of the disk.
             * statfs can take a long time the first time it's called for a disk,
             * so release the mutex to prevent a lockup while it's running */
            //STB_OSMutexUnlock(disk_mutex);
            retval = statfs(disk->mount_path, &fs_info);
            //STB_OSMutexLock(disk_mutex);

            if (retval == 0)
            {
               /* Calculate the disk size in KB. It's done various ways to avoid overflow */
               if ((fs_info.f_bsize > 1024) && ((fs_info.f_bsize % 1024) == 0))
               {
                  disk->disk_size = (fs_info.f_bsize / 1024) * fs_info.f_blocks;
               }
               else
               {
                  disk->disk_size = (fs_info.f_bsize * fs_info.f_blocks) / 1024;
               }
            }
            else
            {
               DISK_ERR("Failed to get disk info to calculate the size, errno %d", errno);
            }

            /* Add the new disk to the end of the disk list */
            if (disk_list == NULL)
            {
               disk_list = disk;
            }
            else
            {
               disk_ptr = disk_list;

               while (disk_ptr->next != NULL)
               {
                  disk_ptr = disk_ptr->next;
               }

               disk_ptr->next = disk;
            }
         }
         else
         {
            DISK_ERR("Failed to allocate %d bytes", strlen(mount_path) + 1);
            STB_MEMFreeSysRAM(disk->device_name);
            STB_MEMFreeSysRAM(disk);
            disk = NULL;
         }
      }
      else
      {
         DISK_ERR("Failed to allocate %d bytes", strlen(device_name) + 1);
         STB_MEMFreeSysRAM(disk);
         disk = NULL;
      }
   }
   else
   {
      DISK_ERR("Failed to allocate memory for disk %s / %s", device_name, mount_path);
   }

   return(disk);
}

static void RemoveDisk(S_DISK_INFO* del_disk)
{
   S_DISK_INFO* prev;
   S_DISK_INFO* disk;

   prev = NULL;

   for (disk = disk_list; disk != del_disk; )
   {
      prev = disk;
      disk = disk->next;
   }

   if (disk != NULL)
   {
      if (prev == NULL)
      {
         /* First disk in the list is being deleted */
         disk_list = disk->next;
      }
      else
      {
         prev->next = disk->next;
      }

      if (disk->device_name != NULL)
      {
         STB_MEMFreeSysRAM(disk->device_name);
      }
      if (disk->mount_path != NULL)
      {
         STB_MEMFreeSysRAM(disk->mount_path);
      }

      STB_MEMFreeSysRAM(disk);
   }
}

static S_DISK_INFO* FindDisk(U16BIT disk_id)
{
   S_DISK_INFO* disk;

   for (disk = disk_list; (disk != NULL) && (disk->disk_id != disk_id); )
   {
      disk = disk->next;
   }

   return(disk);
}

