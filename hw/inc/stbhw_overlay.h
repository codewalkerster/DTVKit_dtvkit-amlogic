/*******************************************************************************
* Copyright (c) 2019 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
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
 * @brief   Header file -
 * @file    stbhw_overlay.h
 * @date    August 2019
 */

#include <stdbool.h>

#ifndef STBHW_OVERLAY_H
#define STBHW_OVERLAY_H

#define SCREEN_ID    0
#define SUBTITLE_ID  1
#define MHEG5_ID     2

/**
 * @brief Method for setting overlay surface size (Android specific).
 */
typedef void (*F_OverlaySetSize)(int id, bool scale, unsigned int startx, unsigned int starty, unsigned int width, unsigned int height);

/**
 * @brief Method for updating to an overlay surface (Android specific).
 */
typedef void (*F_OverlayUpdate)(int id, int dst_x, int dst_y, unsigned int dst_width, unsigned int dst_height, const unsigned char *data);

/**
 * @brief Method for displaying target overlay surface (Android specific).
 */
typedef void (*F_OverlayDisplay)(void);

/**
 * @brief Register Android overlay functions.
 */
void STB_OSDRegisterOverlayFuncs(F_OverlaySetSize, F_OverlayUpdate, F_OverlayDisplay);

/**
 * @brief   Reconifugres the OSD for a new screen size
 * @param   scaling TRUE if osd scaling is required due to MHEG scene
 *          aspect ratio, FALSE otherwise
 * @param   width width of OSD in pixels
 * @param   height height of OSD in pixels
 * @param   x_offset offset of OSD from left of screen, in pixels
 * @param   y_offset offset of OSD from top of screen, in pixels
 */
void STB_OSDModifyPosition(BOOLEAN scaling, U16BIT width, U16BIT height, U16BIT x_offset, U16BIT y_offset);

#endif /*STBHW_OVERLAY_H*/