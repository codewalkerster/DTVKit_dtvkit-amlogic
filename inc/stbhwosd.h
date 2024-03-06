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
 * @brief   Header file - Function prototypes for OSD control
 * @file    stbhwosd.h
 * @date    06/02/2001
 */

// pre-processor mechanism so multiple inclusions don't cause compilation error

#ifndef _STBHWOSD_H
#define _STBHWOSD_H

#include "techtype.h"
#include "osdtype.h"

//---Constant and macro definitions for public use-----------------------------

//---Enumerations for public use-----------------------------------------------

//---Global type defs for public use-------------------------------------------

//---Global Function prototypes for public use---------------------------------

/**
 * @brief   Initialised the OSD hardware layer functions
 * @param   num_max_regions number of regions required (not used here)
 */
void STB_OSDInitialise(U8BIT num_max_regions);

/**
 * @brief   Enable/Disable the OSD
 * @param   enable TRUE to enable
 * @return  The new state ( i.e. will = param if successful )
 */
BOOLEAN STB_OSDEnable(BOOLEAN enable);

/**
 * @brief   Disables (makes invisible) the OSD. This function needs to be implemented on platforms
 *          that cannot display the UI at the same time as subtitles or teletext
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_OSDDisableUIRegion(void);

/**
 * @brief   Disables (makes invisible) the OSD. This function needs to be implemented on platforms
 *          that cannot display the UI at the same time as subtitles or teletext
 * @return  TRUE if successful, FALSE otherwise
 */
BOOLEAN STB_OSDEnableUIRegion(void);

/**
 * @brief   Sets the UI transparency level (0-100%)
 * @param   trans transparency in percent
 */
void STB_OSDSetTransparency(U8BIT trans);

/**
 * @brief   Returns the current UI transparency level
 * @return  The current transparency in percent
 */
U8BIT STB_OSDGetTransparency(void);

/**
 * @brief   Sets a range of palette entries to Trans/Red/Grn/Blue levels.
 *          This function is used for 8 bit colour depth only.
 * @param   index starting number of palette entry to be set
 * @param   num number of consecutive palette entries to set
 * @param   trgb the colour value array
 */
void STB_OSDSetPalette(U16BIT index, U16BIT num, U32BIT *trgb);

/**
 * @brief   Returns a pointer to the current TRGB palette (clut).
 *          This function is used for 8 bit colour depth only.
 * @return  Pointer to an array of 32bit trans,red,green,blue values
 */
U32BIT* STB_OSDGetCurrentPalette(void);

/**
 * @brief   Draw a bitmap into the UI composition (invisible) buffer
 * @param   x the x coordinate where to draw
 * @param   y the x coordinate where to draw
 * @param   width width of bitmap in pixels
 * @param   height height of bitmap in pixels
 * @param   bits bits per pixel of source bitmap
 * @param   data the bitmap data
 */
void STB_OSDDrawBitmap(U16BIT x, U16BIT y, U16BIT width, U16BIT height, U8BIT bits, U8BIT *data);

/**
 * @brief   Read a bitmap from the UI composition (invisible) buffer
 * @param   x the x coordinate where to read
 * @param   y the x coordinate where to read
 * @param   width width of bitmap in pixels
 * @param   height height of bitmap in pixels
 * @param   bits bits per pixel of destination bitmap
 * @param   data the resultant bitmap data
 */
void STB_OSDReadBitmap(U16BIT x, U16BIT y, U16BIT width, U16BIT height, U8BIT bits, U8BIT *data);

/**
 * @brief   Draw a single pixel in the UI composition buffer
 * @param   x x coordinate of pixel
 * @param   y y coordinate of pixel
 * @param   colour colour of pixel
 */
void STB_OSDDrawPixel(U16BIT x, U16BIT y, U32BIT colour);

/**
 * @brief   Read a single pixel from the UI composition buffer
 * @param   x x coordinate of pixel
 * @param   y y coordinate of pixel
 * @param   colour colour of pixel
 */
void STB_OSDReadPixel(U16BIT x, U16BIT y, U32BIT *colour);

/**
 * @brief   Draw a horizontal line in the UI composition buffer
 * @param   x x coordinate of line
 * @param   y y coordinate of line
 * @param   width width of line in pixels
 * @param   colour colour of line
 */
void STB_OSDDrawHLine(U16BIT x, U16BIT y, U16BIT width, U32BIT colour);

/**
 * @brief   Draw a vertical line in the UI composition buffer
 * @param   x x coordinate of line
 * @param   y y coordinate of line
 * @param   height height of line in pixels
 * @param   colour colour of line
 */
void STB_OSDDrawVLine(U16BIT x, U16BIT y, U16BIT height, U32BIT colour);

/**
 * @brief   Draw a rectangle in the UI composition buffer
 * @param   x x coordinate of rectangle
 * @param   y x coordinate of rectangle
 * @param   width width of rectangle
 * @param   height height of rectangle
 * @param   colour colour of rectangle
 * @param   thick thickness of outline for hollow rectangles
 * @param   fill TRUE for solid (filled) rectangle
 */
void STB_OSDDrawRectangle(U16BIT x, U16BIT y, U16BIT width, U16BIT height, U32BIT colour,
                          U8BIT thick, BOOLEAN fill);

/**
 * @brief   Clear the entire UI composition buffer to a single colour
 * @param   colour colour to clear to
 */
void STB_OSDClear(U32BIT colour);

/**
 * @brief   Clear the user interface layer to the given colour using the given blit op
 * @param   colour colour to clear to
 * @param   bflg blit operation
 */
void STB_OSDFill(U32BIT colour, E_BLIT_OP bflg);

/**
 * @brief   Returns the current width and height of the OSD
 * @param   width width of OSD in pixels
 * @param   height height of OSD in pixels
 */
void STB_OSDGetSize(U16BIT *width, U16BIT *height);

/**
 * @brief   Commit invisible UI buffer to visible surface and copy back
 */
void STB_OSDUpdate(void);

/**
 * @brief   Register app fn that can be used by the platform code OSD module
 *          to notify the application when the UI needs to be redrawn
 * @param   func the callback function
 */
void STB_OSDRegisterRefreshHandler(void (*func)(void));

/**
 * @brief   App registered callback to indicate if OSD is in use
 * @param   func the callback function
 */
void STB_OSDRegisterInUseCallback(U8BIT (*func)(void));

/**
 * @brief   Can be called by anyone to force redraw of the OSD by the UI. This function will
 *          cause the platform code OSD module to call the registered refresh handler (see
 *          STB_OSDRegisterRefreshHandler).
 */
void STB_OSDRefreshDisplayCallback(void);

/**
 * @brief   Reconifugres the OSD for a new screen size
 * @param   scaling TRUE if osd scaling is required due to MHEG scene
 *          aspect ratio, FALSE otherwise
 * @param   width width of OSD in pixels
 * @param   height height of OSD in pixels
 * @param   x_offset offset of OSD from left of screen, in pixels
 * @param   y_offset offset of OSD from top of screen, in pixels
 */
void STB_OSDResize(BOOLEAN scaling, U16BIT width, U16BIT height, U16BIT x_offset, U16BIT y_offset);


/* Subtitles and teletext OSD functions*/

/**
 * @brief   Should be called to set the size of the display so that SD subtitles
 *          can be scaled correctly for an HD display, or vice versa.
 * @param   width - display width defined by the subtitle DDS
 * @param   height - display height defined by the subtitle DDS
 */
void STB_OSDSetRegionDisplaySize(U16BIT width, U16BIT height);

/**
 * @brief   Creates a new OSD region (for subtitling)
 * @param   width width of new region
 * @param   height height of new region
 * @param   depth bits per pixel of new region
 * @return  handle (pointer to) new region
 */
void* STB_OSDCreateRegion(U16BIT width, U16BIT height, U8BIT depth);

/**
 * @brief   Destroys (free the resources used by) a region
 * @param   handle handle of (pointer to) the region
 */
void STB_OSDDestroyRegion(void *handle);

/**
 * @brief   Sets a regions entire palette to a T,Y,CR,CB clut
 * @param   handle handle (pointer to) the region to configure
 * @param   tycrcb pointer to the CLUT entries
 */
void STB_OSDSetYCrCbPalette(void *region_handle, U32BIT *tycrcb);

/**
 * @brief   Move a region to new coordinates
 * @param   handle handle of (pointer to) the region
 * @param   x new x coordinate of region
 * @param   y new y coordinate of region
 */
void STB_OSDMoveRegion(void *handle, U16BIT x, U16BIT y);

/**
 * @brief   Makes a region invisible
 * @param   handle handle of (pointer to) the region
 */
void STB_OSDHideRegion(void *handle);

/**
 * @brief   Makes a region visible
 * @param   handle handle of (pointer to) the region
 */
void STB_OSDShowRegion(void *handle);

/**
 * @brief   Draw a bitmap in a specified region
 * @param   handle handle of (pointer to) the region
 * @param   x x coordinate to draw bitmap
 * @param   y y coordinate to draw bitmap
 * @param   w width of bitmap
 * @param   h height of bitmap
 * @param   bitmap the bitmap data
 * @param   non_modifying_colour not used
 */
void STB_OSDDrawBitmapInRegion(void *handle, U16BIT x, U16BIT y, U16BIT w, U16BIT h, U8BIT *bitmap, BOOLEAN non_modifying_colour);

/**
 * @brief   Copy a region to another region, including palette
 * @param   handle_new handle of (pointer to) new (destination) region
 * @param   handle_old handle of (pointer to) old (source) region
 */
void STB_OSDRegionToRegionCopy(void *handle_new, void *handle_orig);

/**
 * @brief   Fill a region with a colour
 * @param   handle handle of (pointer to) the region
 * @param   colour the colour index to fill with
 */
void STB_OSDFillRegion(void *handle, U8BIT colour);

/**
 * @brief   Fill the rectangle within the given region with the given colour
 * @param   handle - region handle
 * @param   left - x position of rectangle within the region
 * @param   top - y position of rectangle within the region
 * @param   width - rectangle width
 * @param   height rectangle height
 * @param   colour - fill colour
 */
void STB_OSDRegionFillRect(void *handle, U16BIT left, U16BIT top, U16BIT width, U16BIT height, U8BIT colour);

/**
 * @brief   Updates the display of all subtitle regions
 */
void STB_OSDUpdateRegions(void);

/**
 * @brief   Sets the RGB palette for the given region. This function is used for Teletext
 * @param   handle region handle as returned by STB_OSDCreateRegion.
 * @param   trgb pointer to the palette array
 */
void STB_OSDSetRGBPalette(void *handle, U32BIT *trgb);

#endif //  _STBHWOSD_H

//*****************************************************************************
// End of file
//*****************************************************************************

