/*******************************************************************************
 * Copyright © 2014 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * Copyright © 2008 Ocean Blue Software Ltd
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
 * @brief   Graphics functions required by the HD MHEG5 engine.
 *          All references to colour used in these functions can be one of three formats:
 *          * 8-bit colour palette
 *          * 16-bit ARGB (4-4-4-4)
 *          * 32-bit ARGB (8-8-8-8)
 *          The MHEG engine will use the format indicated in MHEG5_Open()
 * @file    stb_osd.h
 * @date    30/04/2008
 * @author  Adam Sturtridge
 */

#ifndef _STB_OSD_H
#define _STB_OSD_H

#include "techtype.h"
#include "osdtype.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---Constant and macro definitions for public use-----------------------------*/

/*---Enumerations for public use-----------------------------------------------*/

/*---Global type defs for public use-------------------------------------------*/

/*---Global Function prototypes for public use---------------------------------*/

/**
 * @brief   Sets Colour Palette array of up to 256 values, for single byte colour depth.
 *          This palette being an array of 'U32BIT' (8 Alpha, 8 Red, 8 Green, 8 Blue).
 * @param   index
 * @param   number   Size of palette array
 * @param   argb     pointer to palette array
 */
void STB_OSDMhegSetPalette(U16BIT index, U16BIT number, const U32BIT *argb);

/**
 * @brief   Sets the size of the OSD to be used by MHEG engine.
 *          The return must be a surface handle for the entire screen back-buffer.
 *          The Engine will draw to the surface using STB_OSDMhegBlitStretch, which
 *          would be equivalent to it using STB_OSDMhegBlitBitmap() without stretching.
 * @param   width   Width of MHEG OSD resolution
 * @param   height  Height of MHEG OSD resolution
 * @param   bits    Number of bits per pixel
 * @return  Surface handle of MHEG OSD layer
 */
void* STB_OSDMhegSetResolution(U16BIT width, U16BIT height, U8BIT bits);

/**
 * @brief   Creates a hardware surface on which MHEG5 engine will draw an individual
 *          MHEG object.
 *          At its basic the function can just allocate the buffer to be returned by
 *          STB_OSDMhegLockBuffer(). It's size being: (width * height * bytes_per_pixel)
 *          Also, when 'init' is TRUE, function initialises surface buffer to the
 *          specified colour. For pixel colour format of less than four bytes, use
 *          least significant bits of 'colour'.
 * @param   width Width of requested surface in pixels
 * @param   height Height of requested surface in pixels
 * @param   init If TRUE, initialise buffer with colour.
 * @param   colour colour for all pixels in buffer.
 * @return  void*    Success  -  Handle to surface.
 *          Failure  -  NULL (or zero)
 */
void* STB_OSDMhegCreateSurface( U16BIT width, U16BIT height,
   BOOLEAN init, U32BIT colour );

/**
 * @brief   Converts hardware surface handle returned by STB_OSDMhegCreateSurface()
 *          to buffer address that the engine needs in order to draw the MHEG object.
 *          This function can inform HW that the engine needs write access to buffer.
 *          MHEG5 will use the return address and 'pitch' (or stride) value to
 *          locate pixel data.
 *          Before calling this function, 'pitch' is  initialised to width as given
 *          by STB_OSDMhegCreateSurface(), but platform can alter this here.
 * @param   surface Handle of surface returned by STB_OSDMhegCreateSurface
 * @param   pitch width in bytes of one line of pixel data in buffer
 * @return  void*    Address of the buffer
 */
void* STB_OSDMhegLockBuffer( void *surface, U32BIT *pPitch );

/**
 * @brief   This function informs HW that MHEG5 is finished writing to the buffer.
 * @param   surface Handle of surface returned by STB_OSDMhegCreateSurface
 * @return  void
 */
void STB_OSDMhegUnlockBuffer( void *surface );

/**
 * @brief   This function destroys surface and all data allocated by
 *          STB_OSDMhegCreateSurface()
 * @param   surface Handle of surface returned by STB_OSDMhegCreateSurface
 * @return  void
 */
void STB_OSDMhegDestroySurface( void *surface );

/**
 * @brief   Render bitmap on OSD back buffer in the given screen location, with given
 *          operation.
 *          The bitmap is referenced 'surface' - a handle returned by
 *          STB_OSDMhegCreateSurface()
 *          1. It is a one-to-one mapping between surface pixels and screen pixels, so
 *          rect.width and rect.height give size of rectangle on the screen as well.
 *          2. (rect.top + rect.height) is guaranteed to be less than or equal to height
 *          given to STB_OSDMhegCreateSurface()
 * @param   surface Handle of surface returned by STB_OSDMhegCreateSurface
 * @param   rect source rectangle within surface - top/left is offset into
 *          bitmap referenced by 'surface', width/height gives size.
 * @param   pitch Width of line of source bitmap data - as returned by
 *          STB_OSDMhegLockBuffer()
 * @param   screen_x Left or X position on screen to draw bitmap
 * @param   screen_y Top or Y position on screen to draw bitmap
 * @param   bflg Operation - COPY or ALPHA BLEND
 * @return  void
 */
void STB_OSDMhegBlitBitmap( void *surface, S_RECTANGLE *pRect, U32BIT pitch,
   U16BIT screen_x, U16BIT screen_y, E_BLIT_OP bflg );

/**
 * @brief   Draw a filled rectangle on OSD back buffer in the location given.
 *          Where pixel colour is less than four bytes, use least significant bits
 *          in 'colour'.
 *          'rect' can be part of the screen or the entire screen.
 * @param   rect rectangle on screen - with top,left starting position
 * @param   colour colour for all pixels in rectangle.
 * @param   bflg Operation - COPY or ALPHA BLEND
 * @return  void
 */
void STB_OSDMhegFillRectangle( S_RECTANGLE *pRect, U32BIT colour, E_BLIT_OP bflg );

/**
 * @brief   Stretch blit bitmap data from source surface to destination surface
 *          using source and destination rectangles. When 'dst_surf' is handle
 *          returned by STB_OSDMhegSetResolution(), and rectangles are same size,
 *          this function is equivalent to STB_OSDMhegBlitBitmap.
 * @param   pSrcRect rectangle for bitmap data
 * @param   src_surf handle returned by STB_OSDMhegCreateSurface
 * @param   pDstRect rectangle for destination on surface
 * @param   dst_surf handle returned by STB_OSDMhegCreateSurface or
 *          STB_OSDMhegSetResolution
 * @param   bflg Operation - COPY or ALPHA BLEND
 * @return  void
 */
void STB_OSDMhegBlitStretch( S_RECTANGLE *pSrcRect, void *src_surf,
   S_RECTANGLE *pDstRect, void *dst_surf, E_BLIT_OP bflg );

/**
 * @brief   Draw a filled rectangle on surface in the location given.
 *          Where pixel colour is less than four bytes, use least significant bits
 *          in 'colour'.
 * @param   surface handle returned by STB_OSDMhegCreateSurface
 * @param   rect rectangle on screen - with top,left starting position
 * @param   colour colour for all pixels in rectangle.
 * @param   bflg Operation - COPY or ALPHA BLEND
 * @return  void
 */
void STB_OSDMhegFillSurface( void *surface, S_RECTANGLE *pRect, U32BIT colour,
   E_BLIT_OP bflg );

/**
 * @brief   Commit OSD changes to the screen - changes given by previous calls to
 *          STB_OSDMhegDrawRectangle() and STB_OSDMhegDrawBitmap().
 * @return  void
 */
void STB_OSDMhegUpdate(void);

/**
 * @brief   Clear MHEG's entire OSD
 */
void STB_OSDMhegClear(void);

#ifdef __cplusplus
}
#endif

#endif /* _STB_OSD_H */
