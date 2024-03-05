/*******************************************************************************
 * Copyright © 2014 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * Copyright © 2009 Ocean Blue Software Ltd
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
 * @brief   Functions for writing upgrade modules to non volatile memory
 * @file    stbhwupg.h
 * @date    28/10/2009
 * @author  Sergio Panseri
 */

/* pre-processor mechanism so multiple inclusions don't cause compilation error*/

#ifndef _STBHWUPG_H

#define _STBHWUPG_H

#include "techtype.h"

/*---Constant and macro definitions for public use-----------------------------*/

/*---Enumerations for public use-----------------------------------------------*/

/*---Global type defs for public use-------------------------------------------*/

/*---Global Function prototypes for public use---------------------------------*/

/**
 * @brief   Initialisation of the necessary structures
 */
void STB_UPGInitialise(void);

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
BOOLEAN STB_UPGStart(U8BIT image_type, U8BIT *filename);

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
BOOLEAN STB_UPGWrite(U8BIT image_type, U32BIT offset, U32BIT size, U8BIT *buffer);

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
BOOLEAN STB_UPGRead(U8BIT image_type, U32BIT offset, U32BIT size, U8BIT *buffer);

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
BOOLEAN STB_UPGFinish(U8BIT image_type, BOOLEAN upgrade_successful);

/**
 * @brief   Returns the size of the NVM area available for the application
 *          and all the upgradable modules. In the last part of this area
 *          Intellibyte loader places some version information.
 * @return  Size of the application area.
 */
U32BIT STB_UPGGetApplicationSize(void);

/**
 * @brief   Returns the application's offset inside its area.
 * @return  Application offset.
 */
U32BIT STB_UPGGetApplicationOffset(void);

#endif /*  _STBHWUPG_H */
