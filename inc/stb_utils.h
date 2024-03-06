/**
 * @brief   Header file - macros and function prototypes for public use
 *
 * @file    stb_utils.h
 * @date    14/07/2022
 */

#ifndef _STB_UTILS_H
#define _STB_UTILS_H

#include "techtype.h"


/**
   @brief       read sysfs file
   @param[in]   name, File name
   @param[out]  buf, store sysfs node value
   @return      TRUE if got, FALSE otherwise
 */
BOOLEAN STB_File_Read(const char *name, char *buf, int len);

/**
   @brief      Write a string cmd to a file
   @param[in]  name, File name
   @param[in]  cmd, String command
   @return     TRUE if got, FALSE otherwise
 */
BOOLEAN STB_File_Echo(const char *name, const char *cmd);

/**
 * @brief   get dynamic prop
   @param   name prop name
   @param   buf returned value
   @param   len length of buf
   @return  TRUE if got, FALSE otherwise
 */
BOOLEAN STB_DVRProp_Get(const char *name, char *buf, int len);

/**
 * @brief       set dynamic prop
   @param[in]   name prop name
   @param[in]   value set value
   @return      TRUE if got, FALSE otherwise
*/
BOOLEAN STB_DVRProp_Set(const char *name, const char *value);

/**
 * @brief       check is X4 demux or not
*/
BOOLEAN STB_IsNewHW(void);
#endif /* _STB_UTILS_H */
