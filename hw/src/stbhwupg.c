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
 * @brief   Set Top Box - Hardware Layer, Functions for writing upgrade modules to non volatile memory
 * @file    stbhwupg.c
 * @date    October 2018
 */

/*#define UPG_DEBUG*/

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */
/* third party header files */
/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "dtv_log.h"
#define TAG  "STBHWUPG"

/*---constant definitions for this file--------------------------------------*/
#ifdef UPG_DEBUG
#define UPG_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define UPG_DBG(x,...)
#endif

/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/

/*---local function prototypes for this file---------------------------------*/

/*---local function definitions----------------------------------------------*/

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialisation of the necessary structures
 */
void STB_UPGInitialise(void)
{
   FUNCTION_START(STB_UPGInitialise);
   FUNCTION_FINISH(STB_UPGInitialise);
}

/**
 * @brief   Specifies the file path that STB_UPWrite would write to and STB_UPGRead would read from
 *          when performing upgrade. If this function is not called, a default file name will be
 *          used.
 * @param   image_type type of image to be written, this is meaningful
 *          for the platform code only, as each platform may have different
 *          upgradable modules (kernel, application, kernel modules...). The
 *          midware passes this parameter as it finds it in the upgrade
 *          stream without interpretation.
 * @param   filename
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_UPGStart(U8BIT image_type, U8BIT *filename)
{
   FUNCTION_START(STB_UPGStart);
   USE_UNWANTED_PARAM(image_type);
   USE_UNWANTED_PARAM(filename);
   FUNCTION_FINISH(STB_UPGStart);
   return(FALSE);
}

/**
 * @brief   Writes size bytes to the upgrade storage area specified by
 *          image_type.
 * @param   image_type type of image to be written, this is meaningful
 *          for the platform code only, as each platform may have different
 *          upgradable modules (kernel, application, kernel modules...). The
 *          midware passes this parameter as it finds it in the upgrade
 *          stream without interpretation.
 * @param   offset offset inside the specified area where to write to
 * @param   size number of bytes to write
 * @param   buffer buffer containing the data to be written
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_UPGWrite(U8BIT image_type, U32BIT offset, U32BIT size, U8BIT *buffer)
{
   FUNCTION_START(STB_UPGWrite);
   USE_UNWANTED_PARAM(image_type);
   USE_UNWANTED_PARAM(offset);
   USE_UNWANTED_PARAM(size);
   USE_UNWANTED_PARAM(buffer);
   FUNCTION_FINISH(STB_UPGWrite);
   return FALSE;
}

/**
 * @brief   Read size bytes from the upgrade storage area specified by image_type.
 * @param   image_type type of image to be read, this
 *          is meaningful for the platform code only, as each platform may
 *          have different upgradable modules (kernel, application, kernel
 *          modules...). The midware passes this parameter as it finds it in
 *          the upgrade stream without interpretation.
 * @param   offset offset inside the specified area where to read from
 * @param   size number of bytes to read
 * @param   buffer buffer where to return the read data.
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_UPGRead(U8BIT image_type, U32BIT offset, U32BIT size, U8BIT *buffer)
{
   FUNCTION_START(STB_UPGRead);
   USE_UNWANTED_PARAM(image_type);
   USE_UNWANTED_PARAM(offset);
   USE_UNWANTED_PARAM(size);
   USE_UNWANTED_PARAM(buffer);
   FUNCTION_FINISH(STB_UPGRead);
   return FALSE;
}

/**
 * @brief   Finalises the upgrade performing all the required actions needed when all the upgrade
 *          data have been written or the upgrade process failed.
 * @param   image_type type of image to be read, this
 *          is meaningful for the platform code only, as each platform may
 *          have different upgradable modules (kernel, application, kernel
 *          modules...). The midware passes this parameter as it finds it in
 *          the upgrade stream without interpretation.
 * @param   upgrade_successful TRUE if the upgrade data has been successfully written, FALSE
 *          otherwise
 * @return  TRUE if the required actions have been performed successfully, FALSE otherwise
 */
BOOLEAN STB_UPGFinish(U8BIT image_type, BOOLEAN upgrade_successful)
{
   FUNCTION_START(STB_UPGFinish);
   USE_UNWANTED_PARAM(image_type);
   USE_UNWANTED_PARAM(upgrade_successful);
   FUNCTION_FINISH(STB_UPGFinish);
   return(FALSE);
}

/**
 * @brief   Returns the size of the NVM area available for the application
 *          and all the upgradable modules. In the last part of this area
 *          Intellibyte loader places some version information.
 * @return  Size of the application area.
 */
U32BIT STB_UPGGetApplicationSize(void)
{
   FUNCTION_START(STB_UPGGetApplicationSize);
   FUNCTION_FINISH(STB_UPGGetApplicationSize);
   return 0;
}

/**
 * @brief   Returns the application's offset inside its area.
 * @return  Application offset.
 */
U32BIT STB_UPGGetApplicationOffset(void)
{
   FUNCTION_START(STB_UPGGetApplicationSize);
   FUNCTION_FINISH(STB_UPGGetApplicationSize);
   return 0;
}

