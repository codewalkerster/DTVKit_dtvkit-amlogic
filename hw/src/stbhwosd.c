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
 * @brief   Set Top Box - Hardware Layer, On Screen Display
 * @file    stbhwosd.c
 * @date    October 2018
 */

//#define OSD_DEBUG
//#define SUBT_DEBUG
//#define MHEG_DEBUG

/*---includes for this file--------------------------------------------------*/
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwosd.h"

/*---constant definitions for this file--------------------------------------*/

/*---macro definitions for this file-----------------------------------------*/
#ifdef OSD_DEBUG
   #define OSD_DBG(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define OSD_DBG(x,...)
#endif

#ifdef SUBT_DEBUG
   #define SUBT_DBG(x,...)       STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define SUBT_DBG(x,...)
#endif

#ifdef MHEG_DEBUG
   #define MHEG_DBG(x,...)       STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define MHEG_DBG(x,...)
#endif


/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/
/* (internal variables declared static to make them local) */

/*---local function prototypes for this file---------------------------------*/
/*   (internal functions declared static to make them local) */

/*---global variable definitions---------------------------------------------*/

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialised the OSD hardware layer functions
 * @param   num_max_regions - number of regions required (not used here)
 */
void STB_OSDInitialise(U8BIT num_max_regions)
{
   FUNCTION_START(STB_OSDInitialise);

   USE_UNWANTED_PARAM(num_max_regions);

   FUNCTION_FINISH(STB_OSDInitialise);
}

/**
 * @brief   Reconifugres the OSD for a new screen size
 * @param   scaling - TRUE if osd scaling is required due to MHEG scene
 *          aspect ratio, FALSE otherwise
 * @param   width - width of OSD in pixels
 * @param   height - height of OSD in pixels
 * @param   x_offset - offset of OSD from left of screen, in pixels
 * @param   y_offset - offset of OSD from top of screen, in pixels
 */
void STB_OSDResize(BOOLEAN scaling, U16BIT width, U16BIT height, U16BIT x_offset, U16BIT y_offset)
{
   FUNCTION_START(STB_OSDResize);

   USE_UNWANTED_PARAM(scaling);
   USE_UNWANTED_PARAM(x_offset);
   USE_UNWANTED_PARAM(y_offset);

   FUNCTION_FINISH(STB_OSDResize);
}

/**
 * @brief   Commit invisible buffer to visible surface and copy back
 */
void STB_OSDUpdate(void)
{
   FUNCTION_START(STB_OSDUpdate);
   FUNCTION_FINISH(STB_OSDUpdate);
}

/**
 * @brief   Enable/Disable the OSD
 * @param   enable - TRUE to enable
 * @return  The new state ( i.e. will = param if successful )
 */
BOOLEAN STB_OSDEnable(BOOLEAN enable)
{
   FUNCTION_START(STB_OSDEnable);
   USE_UNWANTED_PARAM(enable);
   FUNCTION_FINISH(STB_OSDEnable);

   return FALSE;
}

/**
 * @brief   Sets the OSD transparency level (0-100%)
 * @param   trans - transparency in percent
 */
void STB_OSDSetTransparency(U8BIT trans)
{
   FUNCTION_START(STB_OSDSetTransparency);
   USE_UNWANTED_PARAM(trans);
   FUNCTION_FINISH(STB_OSDSetTransparency);
}

/**
 * @brief   Returns the current OSD transparency level
 * @return  The current transparency in percent
 */
U8BIT STB_OSDGetTransparency(void)
{
   FUNCTION_START(STB_OSDGetTransparency);
   FUNCTION_FINISH(STB_OSDGetTransparency);
   return 0;
}

/**
 * @brief   Sets a range of palette entries to Trans/Red/Grn/Blue levels
 * @param   index - starting number of palette entry to be set
 * @param   num - number of consecutive palette entries to set
 * @param   trgb - the colour value array
 */
void STB_OSDSetPalette(U16BIT index, U16BIT num, U32BIT *trgb)
{
   FUNCTION_START(STB_OSDSetPalette);
   USE_UNWANTED_PARAM(index);
   USE_UNWANTED_PARAM(num);
   USE_UNWANTED_PARAM(trgb);
   FUNCTION_FINISH(STB_OSDSetPalette);
}

/**
 * @brief   Draw a bitmap into the composition (invisible) buffer
 * @param   x - the x coordinate where to draw
 * @param   y - the x coordinate where to draw
 * @param   width - width of bitmap in pixels
 * @param   height - height of bitmap in pixels
 * @param   bits - bits per pixel of source bitmap
 * @param   data - the bitmap data
 */
void STB_OSDDrawBitmap(U16BIT x, U16BIT y, U16BIT width, U16BIT height, U8BIT bits, U8BIT *data)
{
   FUNCTION_START(STB_OSDDrawBitmap);
   USE_UNWANTED_PARAM(x);
   USE_UNWANTED_PARAM(y);
   USE_UNWANTED_PARAM(width);
   USE_UNWANTED_PARAM(height);
   USE_UNWANTED_PARAM(bits);
   USE_UNWANTED_PARAM(data);
   FUNCTION_FINISH(STB_OSDDrawBitmap);
}

/**
 * @brief   Read a bitmap from the composition (invisible) buffer
 * @param   x - the x coordinate where to read
 * @param   y - the x coordinate where to read
 * @param   width - width of bitmap in pixels
 * @param   height - height of bitmap in pixels
 * @param   bits - bits per pixel of destination bitmap
 * @param   data - the resultant bitmap data
 */
void STB_OSDReadBitmap(U16BIT x, U16BIT y, U16BIT width, U16BIT height, U8BIT bits, U8BIT *data)
{
   FUNCTION_START(STB_OSDReadBitmap);
   USE_UNWANTED_PARAM(x);
   USE_UNWANTED_PARAM(y);
   USE_UNWANTED_PARAM(width);
   USE_UNWANTED_PARAM(height);
   USE_UNWANTED_PARAM(bits);
   USE_UNWANTED_PARAM(data);
   FUNCTION_FINISH(STB_OSDReadBitmap);
}

/**
 * @brief   Draw a single pixel in the composition buffer
 * @param   x - x coordinate of pixel
 * @param   y - y coordinate of pixel
 * @param   colour - colour of pixel
 */
void STB_OSDDrawPixel(U16BIT x, U16BIT y, U32BIT colour)
{
   FUNCTION_START(STB_OSDDrawPixel);
   USE_UNWANTED_PARAM(x);
   USE_UNWANTED_PARAM(y);
   FUNCTION_FINISH(STB_OSDDrawPixel);
}

/**
 * @brief   Read a single pixel from the composition buffer
 * @param   x - x coordinate of pixel
 * @param   y - y coordinate of pixel
 * @param   colour - colour of pixel
 */
void STB_OSDReadPixel(U16BIT x, U16BIT y, U32BIT *colour)
{
   FUNCTION_START(STB_OSDReadPixel);
   USE_UNWANTED_PARAM(x);
   USE_UNWANTED_PARAM(y);
   USE_UNWANTED_PARAM(colour);
   FUNCTION_FINISH(STB_OSDReadPixel);
}

/**
 * @brief   Draw a horizontal line in the composition buffer
 * @param   x - x coordinate of line
 * @param   y - y coordinate of line
 * @param   width - width of line in pixels
 * @param   colour - colour of line
 */
void STB_OSDDrawHLine(U16BIT x, U16BIT y, U16BIT width, U32BIT colour)
{
   FUNCTION_START(STB_OSDDrawHLine);
   USE_UNWANTED_PARAM(x);
   USE_UNWANTED_PARAM(y);
   USE_UNWANTED_PARAM(width);
   USE_UNWANTED_PARAM(colour);
   FUNCTION_FINISH(STB_OSDDrawHLine);
}

/**
 * @brief   Draw a vertical line in the composition buffer
 * @param   x - x coordinate of line
 * @param   y - y coordinate of line
 * @param   height - height of line in pixels
 * @param   colour - colour of line
 */
void STB_OSDDrawVLine(U16BIT x, U16BIT y, U16BIT height, U32BIT colour)
{
   FUNCTION_START(STB_OSDDrawHLine);
   USE_UNWANTED_PARAM(x);
   USE_UNWANTED_PARAM(y);
   USE_UNWANTED_PARAM(height);
   USE_UNWANTED_PARAM(colour);
   FUNCTION_FINISH(STB_OSDDrawHLine);
}

/**
 * @brief   Draw a rectangle in the composition buffer
 * @param   x - x coordinate of rectangle
 * @param   y - x coordinate of rectangle
 * @param   width - width of rectangle
 * @param   height - height of rectangle
 * @param   colour - colour of rectangle
 * @param   thick - thickness of outline for hollow rectangles
 * @param   fill - TRUE for solid (filled) rectangle
 */
void STB_OSDDrawRectangle(U16BIT x, U16BIT y, U16BIT width, U16BIT height, U32BIT colour,
   U8BIT thick, BOOLEAN fill)
{
   FUNCTION_START(STB_OSDDrawRectangle);
   USE_UNWANTED_PARAM(x);
   USE_UNWANTED_PARAM(y);
   USE_UNWANTED_PARAM(width);
   USE_UNWANTED_PARAM(height);
   USE_UNWANTED_PARAM(colour);
   USE_UNWANTED_PARAM(thick);
   USE_UNWANTED_PARAM(fill);
   FUNCTION_FINISH(STB_OSDDrawRectangle);
}

/**
 * @brief   Returns the current width and height of the OSD
 * @param   width - width of OSD in pixels
 * @param   height - height of OSD in pixels
 */
void STB_OSDGetSize(U16BIT *width, U16BIT *height)
{
   FUNCTION_START(STB_OSDGetSize);

   *width = 0;
   *height = 0;

   FUNCTION_FINISH(STB_OSDGetSize);
}

/**
 * @brief   Clear the entire composition buffer to a single colour
 * @param   colour - colour to clear to
 */
void STB_OSDClear(U32BIT colour)
{
   FUNCTION_START(STB_OSDClear);
   USE_UNWANTED_PARAM(colour);
   FUNCTION_FINISH(STB_OSDClear);
}

/**
 * @brief   Clear the user interface layer to the given colour using the given blit op
 * @param   colour - colour to clear to
 * @param   bflg - blit operation
 */
void STB_OSDFill(U32BIT colour, E_BLIT_OP bflg)
{
   FUNCTION_START(STB_OSDFill);
   USE_UNWANTED_PARAM(colour);
   USE_UNWANTED_PARAM(bflg);
   FUNCTION_FINISH(STB_OSDFill);
}

/**
 * @brief   Register app fn that can be called to redraw OSD
 * @param   func - the callback function
 */
void STB_OSDRegisterRefreshHandler(void (*func)(void))
{
   FUNCTION_START(STB_OSDRegisterRefreshHandler);
   FUNCTION_FINISH(STB_OSDRegisterRefreshHandler);
}

/**
 * @brief   Can be called by anyone to force redraw of the OSD by the UI
 */
void STB_OSDRefreshDisplayCallback(void)
{
   FUNCTION_START(STB_OSDRefreshDisplayCallback);
   FUNCTION_FINISH(STB_OSDRefreshDisplayCallback);
}

/**
 * @brief   Returns a pointer to the current TRGB palette (clut)
 * @return  Pointer to an array of 32bit trans,red,green,blue values
 */
U32BIT* STB_OSDGetCurrentPalette(void)
{
   FUNCTION_START(STB_OSDGetCurrentPalette);
   FUNCTION_FINISH(STB_OSDGetCurrentPalette);
   return(NULL);
}


/**
 * @brief   Enables (makes visible) the OSD
 * @return  TRUE if succesful, FALSE otherwise
 */
BOOLEAN STB_OSDEnableUIRegion(void)
{
   FUNCTION_START(STB_OSDEnableUIRegion);
   FUNCTION_FINISH(STB_OSDEnableUIRegion);

   return FALSE;
}

/**
 * @brief   Disables (makes invisible) the OSD
 * @return  TRUE if succesful, FALSE otherwise
 */
BOOLEAN STB_OSDDisableUIRegion(void)
{
   FUNCTION_START(STB_OSDDisableUIRegion);
   FUNCTION_FINISH(STB_OSDDisableUIRegion);

   return FALSE;
}

/**
 * @brief   Sets a regions entire palette to a T,Y,CR,CB clut
 * @param   handle - handle (pointer to) the region to configure
 * @param   tycrcb - pointer to the CLUT entries
 */
void STB_OSDSetYCrCbPalette(void *handle, U32BIT *tycrcb)
{
   FUNCTION_START(STB_OSDSetYCrCbPalette);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(tycrcb);
   FUNCTION_FINISH(STB_OSDSetYCrCbPalette);
}

/**
 * @brief   Creates a new OSD region (for subtitling)
 * @param   width - width of new region
 * @param   height - height of new region
 * @param   depth - bits per pixel of new region
 * @return  handle (pointer to) new region
 */
void* STB_OSDCreateRegion(U16BIT width, U16BIT height, U8BIT depth)
{
   FUNCTION_START(STB_OSDCreateRegion);
   USE_UNWANTED_PARAM(width);
   USE_UNWANTED_PARAM(height);
   USE_UNWANTED_PARAM(depth);
   FUNCTION_FINISH(STB_OSDCreateRegion);

   return NULL;
}

/**
 * @brief   Destroys (free the resources used by) a region
 * @param   handle - handle of (pointer to) the region
 */
void STB_OSDDestroyRegion(void* handle)
{
   FUNCTION_START(STB_OSDDestroyRegion);
   USE_UNWANTED_PARAM(handle);
   FUNCTION_FINISH(STB_OSDDestroyRegion);
}

/**
 * @brief   Move a region to new coordinates
 * @param   handle - handle of (pointer to) the region
 * @param   x - new x coordinate of region
 * @param   y - new y coordinate of region
 */
void STB_OSDMoveRegion(void *handle, U16BIT x, U16BIT y)
{
   FUNCTION_START(STB_OSDMoveRegion);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(x);
   USE_UNWANTED_PARAM(y);
   FUNCTION_FINISH(STB_OSDMoveRegion);
}

/**
 * @brief   Makes a region invisible
 * @param   handle - handle of (pointer to) the region
 */
void STB_OSDHideRegion(void* handle)
{
   FUNCTION_START(STB_OSDHideRegion);
   USE_UNWANTED_PARAM(handle);
   FUNCTION_FINISH(STB_OSDHideRegion);
}

/**
 * @brief   Makes a region visible
 * @param   handle - handle of (pointer to) the region
 */
void STB_OSDShowRegion(void* handle)
{
   FUNCTION_START(STB_OSDShowRegion);
   USE_UNWANTED_PARAM(handle);
   FUNCTION_FINISH(STB_OSDShowRegion);
}

/**
 * @brief   Updates the display of all subtitle regions
 */
void STB_OSDUpdateRegions(void)
{
   FUNCTION_START(STB_OSDUpdateRegions);
   FUNCTION_FINISH(STB_OSDUpdateRegions);
}

/**
 * @brief   Draw a bitmap in a specified region
 * @param   handle - handle of (pointer to) the region
 * @param   x - x coordinate to draw bitmap
 * @param   y - y coordinate to draw bitmap
 * @param   w - width of bitmap
 * @param   h - height of bitmap
 * @param   bitmap - the bitmap data
 * @param   non_modifying_colour - not used
 */
void STB_OSDDrawBitmapInRegion(void* handle, U16BIT x, U16BIT y, U16BIT w, U16BIT h,
      U8BIT* bitmap, BOOLEAN non_modifying_colour)
{
   FUNCTION_START(STB_OSDDrawBitmapInRegion);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(x);
   USE_UNWANTED_PARAM(y);
   USE_UNWANTED_PARAM(w);
   USE_UNWANTED_PARAM(h);
   USE_UNWANTED_PARAM(bitmap);
   USE_UNWANTED_PARAM(non_modifying_colour);
   FUNCTION_FINISH(STB_OSDDrawBitmapInRegion);
}

/**
 * @brief   Copy a region to another region, including palette
 * @param   handle_new - handle of (pointer to) new (destination) region
 * @param   handle_old - handle of (pointer to) old (source) region
 */
void STB_OSDRegionToRegionCopy(void *handle_new, void *handle_orig)
{
   FUNCTION_START(STB_OSDRegionToRegionCopy);
   USE_UNWANTED_PARAM(handle_new);
   USE_UNWANTED_PARAM(handle_orig);
   FUNCTION_FINISH(STB_OSDRegionToRegionCopy);
}

/**
 * @brief   Fill a region with a colour
 * @param   handle - handle of (pointer to) the region
 * @param   colour - the colour index to fill with
 */
void STB_OSDFillRegion(void *handle, U8BIT colour)
{
   FUNCTION_START(STB_OSDFillRegion);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(colour);
   FUNCTION_FINISH(STB_OSDFillRegion);
}

/**
 * @brief   Should be called to set the size of the display so that SD subtitles
 *          can be scaled correctly for an HD display, or vice versa.
 * @param   width - display width defined by the subtitle DDS
 * @param   height - display height defined by the subtitle DDS
 */
void STB_OSDSetRegionDisplaySize(U16BIT width, U16BIT height)
{
   FUNCTION_START(STB_OSDSetRegionDisplaySize);
   USE_UNWANTED_PARAM(width);
   USE_UNWANTED_PARAM(height);
   FUNCTION_FINISH(STB_OSDSetRegionDisplaySize);
}

/**
 * @brief   Fill the rectangle within the given region with the given colour
 * @param   handle - region handle
 * @param   left - x position of rectangle within the region
 * @param   top - y position of rectangle within the region
 * @param   width - rectangle width
 * @param   height rectangle height
 * @param   colour - fill colour
 */
void STB_OSDRegionFillRect(void *handle, U16BIT left, U16BIT top, U16BIT width,
   U16BIT height, U8BIT colour)
{
   FUNCTION_START(STB_OSDRegionFillRect);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(left);
   USE_UNWANTED_PARAM(top);
   USE_UNWANTED_PARAM(width);
   USE_UNWANTED_PARAM(height);
   USE_UNWANTED_PARAM(colour);
   FUNCTION_FINISH(STB_OSDRegionFillRect);
}

/**
 * @brief   Sets the palette for the given region based on a set of RGB values
 * @param   handle - region handle
 * @param   trgb - array of TRGB palette entries, the size of which is defined
 *                 by the colour depth of the region
 */
void STB_OSDSetRGBPalette(void *handle, U32BIT *trgb)
{
   FUNCTION_START(STB_OSDSetRGBPalette);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(trgb);
   FUNCTION_FINISH(STB_OSDSetRGBPalette);
}

/*---local function definitions----------------------------------------------*/

