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
#include <string.h>
#include <math.h>

#include "techtype.h"
#include "dbgfuncs.h"

#include "stbhwdef.h"
#include "stbhwmem.h"
#include "stbhwos.h"
#include "stbhwc.h"
#include "stbhwosd.h"
#include "stb_osd.h"
#include "stbhw_overlay.h"


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

#define OVERLAY_TASK_PRIORITY   10

/*---local typedef structs for this file-------------------------------------*/
typedef struct
{
    U8BIT *pixels;
    U16BIT width;
    U16BIT height;
    U16BIT pitch;
    U8BIT depth;
} S_OSD_SURFACE;

typedef struct
{
    BOOLEAN scale;
    U16BIT screen_start_x;
    U16BIT screen_start_y;
    U16BIT screen_width;
    U16BIT screen_height;
    S_OSD_SURFACE subt_surface;
    S_OSD_SURFACE mheg_surface;

    U8BIT*  mheg_pixels;
    BOOLEAN osd_resoln;
    BOOLEAN subt_resoln;
    BOOLEAN subt_update;
    BOOLEAN subt_visible;
    BOOLEAN mheg_update;
    BOOLEAN mheg_visible;
} S_OSD_STATUS;

typedef struct
{
   U8BIT r;
   U8BIT g;
   U8BIT b;
   U8BIT a;
} S_OSD_COLOUR;

typedef struct s_osd_region
{
   U8BIT *region_data;
   U32BIT num_colours;
   S_OSD_COLOUR *clut;
   void *locked_sem;
   U16BIT x;
   U16BIT y;
   U16BIT width;
   U16BIT height;
   U8BIT depth;
   BOOLEAN visible;
   struct s_osd_region *next;
} S_OSD_REGION;


/*---local (static) variable declarations for this file----------------------*/
/* (internal variables declared static to make them local) */
static S_OSD_STATUS display_status = {0};

static S_OSD_REGION *osd_regions = NULL;
static void *osd_mutex = NULL;
static void *subtitle_mutex = NULL;

static U8BIT mheg_colourdepth = SCREEN_COLOUR_DEPTH;
static void *mheg_mutex = NULL;
static void* overlay_sem = NULL;
static void* update_sem = NULL;

static F_OverlaySetSize OverlaySetSize;
static F_OverlayUpdate OverlayUpdate;
static F_OverlayDisplay OverlayDisplay;

/*---local function prototypes for this file---------------------------------*/
/*   (internal functions declared static to make them local) */
static void* CreateSurface(U16BIT width, U16BIT height, U8BIT depth);
static void DestroySurface(void *surface);
static void FillSurface(S_OSD_SURFACE *surface, S_RECTANGLE *pRect, U32BIT colour, E_BLIT_OP bflg);
static void BlitBlendSurface32(U8BIT *dst, U32BIT *src, U32BIT width, U32BIT height, U16BIT s_pitch, U16BIT d_pitch);
static void BlitCopySurface32(U8BIT *dst, U32BIT *src, U32BIT width, U32BIT height, U16BIT s_pitch, U16BIT d_pitch);
static void OverlayTask(void *param);


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
   memset(&display_status, 0, sizeof(S_OSD_STATUS));
   display_status.screen_width = SCREEN_WIDTH;
   display_status.screen_height = SCREEN_HEIGHT;
   osd_mutex = STB_OSCreateMutex();
   subtitle_mutex = STB_OSCreateMutex();
   mheg_mutex = STB_OSCreateMutex();
   overlay_sem = STB_OSCreateSemaphore();
   update_sem = STB_OSCreateSemaphore();

   FUNCTION_FINISH(STB_OSDInitialise);
}

/**
 * @brief Register Android overlay functions.
 */
void STB_OSDRegisterOverlayFuncs(F_OverlaySetSize setsize, F_OverlayUpdate update, F_OverlayDisplay display)
{
    OverlaySetSize = setsize;
    OverlayUpdate = update;
    OverlayDisplay = display;

    if (STB_OSCreateTask(OverlayTask, NULL, 4096, OVERLAY_TASK_PRIORITY, (U8BIT*)"OverlayTesk") == NULL)
    {
        STB_SPDebugWrite("Failed to create overlay task");
    }
 }

/**
 * @brief modify osd posion and size.
 */
void STB_OSDModifyPosition(BOOLEAN scaling, U16BIT width, U16BIT height, U16BIT x_offset, U16BIT y_offset)
{
    FUNCTION_START(STB_OSDModifyPosition);

   if ((display_status.screen_width != width) || (display_status.screen_height != height) ||
           display_status.scale != scaling || display_status.screen_start_x != x_offset || display_status.screen_start_y != y_offset)
   {
      OSD_DBG("old=(%u,%u) %ux%u, new=(%u,%u) %ux%u", display_status.screen_start_x, display_status.screen_start_y, display_status.screen_width, display_status.screen_height,
         x_offset, y_offset, width, height);
      /*STB_OSMutexLock(osd_mutex);
      display_status.scale = scaling;
      display_status.screen_start_x = x_offset;
      display_status.screen_start_y = y_offset;
      display_status.screen_width = width;
      display_status.screen_height = height;
      display_status.osd_resoln = true;
      STB_OSMutexUnlock(osd_mutex);
      STB_OSSemaphoreSignal(overlay_sem);
      STB_OSSemaphoreWait(update_sem);*/

      STB_OSMutexLock(osd_mutex);
      display_status.subt_update = TRUE;
      STB_OSSemaphoreSignal(overlay_sem);
      STB_OSSemaphoreWait(update_sem);
      STB_OSMutexUnlock(osd_mutex);
   }

   FUNCTION_FINISH(STB_OSDModifyPosition);
 }

/**
 * @brief   Reconfigures the OSD for a new screen size
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

   if ((display_status.screen_width != width) || (display_status.screen_height != height))
   {
      OSD_DBG("old=%ux%u, new=%ux%u", display_status.screen_width, display_status.screen_height, width, height);
      display_status.screen_width = width;
      display_status.screen_height = height;
   }

   FUNCTION_FINISH(STB_OSDResize);
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

   *width = display_status.screen_width;
   *height = display_status.screen_height;

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
 * @brief   Creates a new OSD region (for subtitling)
 * @param   width - width of new region
 * @param   height - height of new region
 * @param   depth - bits per pixel of new region
 * @return  handle (pointer to) new region
 */
void* STB_OSDCreateRegion(U16BIT width, U16BIT height, U8BIT depth)
{
   S_OSD_REGION *region;

   FUNCTION_START(STB_OSDCreateRegion);

   region = STB_MEMGetSysRAM(sizeof(S_OSD_REGION));
   if (region != NULL)
   {
      SUBT_DBG("w=%u h=%u d=%u: region=%p", width, height, depth, region);

      /* Create the region */
      region->region_data = STB_MEMGetSysRAM(width * height * depth / 8);
      if (region->region_data != NULL)
      {
         region->depth = depth;
         region->num_colours = pow(2, depth);
         region->clut = STB_MEMGetSysRAM(region->num_colours * sizeof(S_OSD_COLOUR));
         if (region->clut != NULL)
         {
            region->width = width;
            region->height = height;
            region->visible = FALSE;
            region->x = 0;
            region->y = 0;
            region->locked_sem = STB_OSCreateSemaphore();

            region->next = osd_regions;
            osd_regions = region;
         }
         else
         {
            STB_MEMFreeSysRAM(region->region_data);
            STB_MEMFreeSysRAM(region);
            region = NULL;
         }
      }
      else
      {
         STB_MEMFreeSysRAM(region);
         region = NULL;
      }
   }

   FUNCTION_FINISH(STB_OSDCreateRegion);

   return(region);
}

/**
 * @brief   Destroys (free the resources used by) a region
 * @param   handle - handle of (pointer to) the region
 */
void STB_OSDDestroyRegion(void* handle)
{
   S_OSD_REGION *region, *prev_region;

   FUNCTION_START(STB_OSDDestroyRegion);

   if (handle != NULL)
   {
      for (prev_region = osd_regions; prev_region != NULL; prev_region = prev_region->next)
      {
         if (prev_region->next == handle)
         {
            break;
         }
      }

      region = (S_OSD_REGION *)handle;

      /* Remove the region from the list */
      if (prev_region == NULL)
      {
         osd_regions = region->next;
      }
      else
      {
         prev_region->next = region->next;
      }

      STB_OSSemaphoreWait(region->locked_sem);

      SUBT_DBG("region=%p", handle);

      /* Free the resources */
      STB_MEMFreeSysRAM(region->region_data);
      STB_MEMFreeSysRAM(region->clut);
      STB_OSDeleteSemaphore(region->locked_sem);
      STB_MEMFreeSysRAM(handle);
   }

   FUNCTION_FINISH(STB_OSDDestroyRegion);
}

/**
 * @brief   Sets a regions entire palette to a T,Y,CR,CB clut
 * @param   handle - handle (pointer to) the region to configure
 * @param   tycrcb - pointer to the CLUT entries
 */
void STB_OSDSetYCrCbPalette(void *handle, U32BIT *tycrcb)
{
   S_OSD_REGION *region;
   U32BIT i;
   S16BIT r, g, b;
   U8BIT y;
   U8BIT cr;
   U8BIT cb;

   FUNCTION_START(STB_OSDSetYCrCbPalette);

   if (handle != NULL)
   {
      region = (S_OSD_REGION *)handle;

      SUBT_DBG("region=%p", handle);

      STB_OSSemaphoreWait(region->locked_sem);

      /* Convert the OBS format palette to DirectFB ARGB format */
      for (i = 0; i < region->num_colours; i++)
      {
         region->clut[i].a = (U8BIT)((~tycrcb[i] & 0xff000000) >> 24);
         if (region->clut[i].a == 0)
         {
            /* Full transparency */
            r = 0;
            g = 0;
            b = 0;
         }
         else
         {
            y = (U8BIT)((tycrcb[i] & 0x00ff0000) >> 16);
            cr = (U8BIT)((tycrcb[i] & 0x0000ff00) >> 8);
            cb = (U8BIT)((tycrcb[i] & 0x000000ff));

            if ((r = y + (cr - 128) * 1.40200) > 255)
            {
               r = 255;
            }
            else if (r < 0)
            {
               r = 0;
            }

            if ((g = y + (cb - 128) * -0.34414 + (cr - 128) * -0.71414) > 255)
            {
               g = 255;
            }
            else if (g < 0)
            {
               g = 0;
            }

            if ((b = y + (cb - 128) * 1.77200) > 255)
            {
               b = 255;
            }
            else if (b < 0)
            {
               b = 0;
            }
         }

         region->clut[i].r = r;
         region->clut[i].g = g;
         region->clut[i].b = b;
      }

      STB_OSSemaphoreSignal(region->locked_sem);
   }

   FUNCTION_FINISH(STB_OSDSetYCrCbPalette);
}

/**
 * @brief   Move a region to new coordinates
 * @param   handle - handle of (pointer to) the region
 * @param   x - new x coordinate of region
 * @param   y - new y coordinate of region
 */
void STB_OSDMoveRegion(void *handle, U16BIT x, U16BIT y)
{
   S_OSD_REGION *region;

   FUNCTION_START(STB_OSDMoveRegion);

   if (handle != NULL)
   {
      region = (S_OSD_REGION *)handle;

      STB_OSSemaphoreWait(region->locked_sem);

      SUBT_DBG("region=%p to %u,%u", handle, x, y);

      region->x = x;
      region->y = y;

      STB_OSSemaphoreSignal(region->locked_sem);
   }

   FUNCTION_FINISH(STB_OSDMoveRegion);
}

/**
 * @brief   Makes a region invisible
 * @param   handle - handle of (pointer to) the region
 */
void STB_OSDHideRegion(void* handle)
{
   S_OSD_REGION *region;

   FUNCTION_START(STB_OSDHideRegion);

   if (handle != NULL)
   {
      region = (S_OSD_REGION *)handle;

      STB_OSSemaphoreWait(region->locked_sem);

      SUBT_DBG("region=%p", handle);

      region->visible = FALSE;

      STB_OSSemaphoreSignal(region->locked_sem);
   }

   FUNCTION_FINISH(STB_OSDHideRegion);
}

/**
 * @brief   Makes a region visible
 * @param   handle - handle of (pointer to) the region
 */
void STB_OSDShowRegion(void* handle)
{
   S_OSD_REGION *region;

   FUNCTION_START(STB_OSDShowRegion);

   if (handle != NULL)
   {
      region = (S_OSD_REGION *)handle;

      STB_OSSemaphoreWait(region->locked_sem);

      SUBT_DBG("region=%p", handle);

      region->visible = TRUE;

      STB_OSSemaphoreSignal(region->locked_sem);
   }

   FUNCTION_FINISH(STB_OSDShowRegion);
}

/**
 * @brief   Updates the display of all subtitle regions
 */
void STB_OSDUpdateRegions(void)
{
   U32BIT bytes_per_pixel;
   S_RECTANGLE rect;
   S_OSD_REGION *region;
   U8BIT *base, *dest_buff;
   U8BIT *bitmap;
   S_OSD_COLOUR *clut;
   U16BIT x, y;
   U32BIT dest_pitch;
   U32BIT src_pitch;
   BOOLEAN visible;

   FUNCTION_START(STB_OSDUpdateRegions);

   STB_OSMutexLock(subtitle_mutex);

   if (display_status.subt_surface.pixels != NULL)
   {
      bytes_per_pixel = SCREEN_COLOUR_DEPTH / 8;

      /* Clear the subtitle surface before drawing currently visible subtitle regions on to it */
      rect.top = 0;
      rect.left = 0;
      rect.width = display_status.subt_surface.width;
      rect.height = display_status.subt_surface.height;

      FillSurface(&display_status.subt_surface, &rect, 0, STB_BLIT_COPY);
      visible = FALSE;

      /* Blit the subtitles */
      for (region = osd_regions; region != NULL; region = region->next)
      {
         if (region->visible)
         {
            visible = TRUE;
            STB_OSSemaphoreWait(region->locked_sem);

            bitmap = (U8BIT *)region->region_data;
            src_pitch = region->width * region->depth / 8;

            dest_pitch = display_status.subt_surface.pitch;
            dest_buff = display_status.subt_surface.pixels + region->y * dest_pitch +
               region->x * bytes_per_pixel;

            base = dest_buff;

            if (region->depth == 4)
            {
               for (y = 0; y < region->height; y++)
               {
                  for (x = 0; x < region->width; x += 2)
                  {
                     clut = &region->clut[(bitmap[x >> 1] >> 4)];
                    *(base + (x << 2) + 0) = clut->r;
                    *(base + (x << 2) + 1) = clut->g;
                    *(base + (x << 2) + 2) = clut->b;
                    *(base + (x << 2) + 3) = clut->a;


                     clut = &region->clut[(bitmap[x >> 1] & 0x0f)];
                     *(base + (x << 2) + 4) = clut->r;
                     *(base + (x << 2) + 5) = clut->g;
                     *(base + (x << 2) + 6) = clut->b;
                     *(base + (x << 2) + 7) = clut->a;

                  }

                  base += dest_pitch;
                  bitmap += src_pitch;
               }
            }
            else
            {
               /* Assume a region depth of 8 bits/pixel */
               for (y = 0; y < region->height; y++)
               {
                  for (x = 0; x < region->width; x++)
                  {
                     clut = &region->clut[bitmap[x]];
                     *(base + (x << 2) + 0) = clut->r;
                     *(base + (x << 2) + 1) = clut->g;
                     *(base + (x << 2) + 2) = clut->b;
                     *(base + (x << 2) + 3) = clut->a;
                  }

                  base += dest_pitch;
                  bitmap += src_pitch;
               }
            }

            STB_OSSemaphoreSignal(region->locked_sem);
         }
      }
      display_status.subt_update = TRUE;
      display_status.subt_visible= visible;
      STB_OSSemaphoreSignal(overlay_sem);
      STB_OSSemaphoreWait(update_sem);
   }

   STB_OSMutexUnlock(subtitle_mutex);

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
   S_OSD_REGION *region;
   U32BIT src_pitch, dst_pitch;
   U32BIT i;
   U8BIT *src, *dst;

   FUNCTION_START(STB_OSDDrawBitmapInRegion);

   USE_UNWANTED_PARAM(non_modifying_colour);

   if (handle != NULL)
   {
      region = handle;

      if (y == 0)
      {
         SUBT_DBG("region=%p, x=%u, y=%u, w=%u, h=%u", handle, x, y, w, h);
      }

      STB_OSSemaphoreWait(region->locked_sem);

      dst_pitch = region->width * region->depth / 8;
      src_pitch = w * region->depth / 8;
      dst = region->region_data + (y * dst_pitch) + (x * region->depth / 8);
      src = bitmap;
      for (i = 0; i < h; i++)
      {
         memcpy(dst, src, src_pitch);
         dst += dst_pitch;
         src += src_pitch;
      }

      STB_OSSemaphoreSignal(region->locked_sem);
   }

   FUNCTION_FINISH(STB_OSDDrawBitmapInRegion);
}

/**
 * @brief   Copy a region to another region, including palette
 * @param   handle_new - handle of (pointer to) new (destination) region
 * @param   handle_old - handle of (pointer to) old (source) region
 */
void STB_OSDRegionToRegionCopy(void *handle_new, void *handle_orig)
{
   S_OSD_REGION *src_region;
   S_OSD_REGION *dest_region;

   FUNCTION_START(STB_OSDRegionToRegionCopy);

   if ((handle_new != NULL) && (handle_orig != NULL))
   {
      src_region = (S_OSD_REGION *)handle_orig;
      dest_region = (S_OSD_REGION *)handle_new;

      STB_OSSemaphoreWait(src_region->locked_sem);
      STB_OSSemaphoreWait(dest_region->locked_sem);

      SUBT_DBG("from=%p, to=%p", handle_orig, handle_new);

      memcpy(dest_region->clut, src_region->clut, dest_region->num_colours * sizeof(S_OSD_COLOUR));
      memcpy(dest_region->region_data, src_region->region_data, dest_region->width * dest_region->height * dest_region->depth / 8);

      STB_OSSemaphoreSignal(dest_region->locked_sem);
      STB_OSSemaphoreSignal(src_region->locked_sem);
   }

   FUNCTION_FINISH(STB_OSDRegionToRegionCopy);
}

/**
 * @brief   Fill a region with a colour
 * @param   handle - handle of (pointer to) the region
 * @param   colour - the colour index to fill with
 */
void STB_OSDFillRegion(void *handle, U8BIT colour)
{
   S_OSD_REGION *region;
   U16BIT pixels_per_byte;
   U8BIT fill_value;
   U16BIT x;

   FUNCTION_START(STB_OSDFillRegion);

   SUBT_DBG("region=%p, colour=%u", handle, colour);

   region = (S_OSD_REGION *)handle;

   if ((region != NULL) && (region->region_data != NULL))
   {
      STB_OSSemaphoreWait(region->locked_sem);

      /* Calculate the fill colour as a whole byte */
      fill_value = colour;
      pixels_per_byte = 8 / region->depth;

      for (x = 1; x < pixels_per_byte; x++)
      {
         fill_value <<= region->depth;
         fill_value |= colour;
      }

      /* Fill the whole region with the colour */
      memset(region->region_data, fill_value, region->width * region->height * region->depth / 8);

      STB_OSSemaphoreSignal(region->locked_sem);
   }

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

   SUBT_DBG("w=%u, h=%u", width, height);

    if ((display_status.subt_surface.width != width) || (display_status.subt_surface.height != height))
    {
        STB_OSMutexLock(subtitle_mutex);

        if (display_status.subt_surface.pixels != NULL)
        {
           STB_MEMFreeSysRAM(display_status.subt_surface.pixels);
        }
        display_status.subt_surface.pixels = STB_MEMGetSysRAM(width * height * SCREEN_COLOUR_DEPTH / 8);
        display_status.subt_surface.width = width;
        display_status.subt_surface.height = height;
        display_status.subt_surface.depth = SCREEN_COLOUR_DEPTH;
        display_status.subt_surface.pitch = width * SCREEN_COLOUR_DEPTH / 8;
        if (display_status.subt_surface.pixels != NULL)
        {
           display_status.subt_resoln = TRUE;
           STB_OSSemaphoreSignal(overlay_sem);
           STB_OSSemaphoreWait(update_sem);
        }
        STB_OSMutexUnlock(subtitle_mutex);
    }

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
   S_OSD_REGION *region;
   U16BIT y;
   U8BIT *dest_ptr;
   U32BIT src_pitch;

   FUNCTION_START(STB_OSDRegionFillRect);

   region = (S_OSD_REGION *)handle;

   if ((region != NULL) && (region->region_data != NULL))
   {
      STB_OSSemaphoreWait(region->locked_sem);

      /* This function is used by the teletext decoder which uses a region of 8 bits/pixel,
       * so this is assumed to simplify the code */
      if ((left + width) > region->width)
      {
         width = region->width - left;
      }

      src_pitch = region->width * region->depth / 8;
      dest_ptr = region->region_data + top * src_pitch + left;

      for (y = top; (y < height) && (y < region->height); dest_ptr += src_pitch, y++)
      {
         memset(dest_ptr, colour, width);
      }

      STB_OSSemaphoreSignal(region->locked_sem);
   }

   FUNCTION_FINISH(STB_OSDRegionFillRect);
}

/**
 * @brief   Sets the palette for the given region based on a set of RGB values
 * @param   handle region handle
 * @param   trgb array of TRGB palette entries, the size of which is defined
 *               by the colour depth of the region
 */
void STB_OSDSetRGBPalette(void *handle, U32BIT *trgb)
{
   S_OSD_REGION *region;
   U32BIT i;

   FUNCTION_START(STB_OSDSetRGBPalette);

   SUBT_DBG("region=%p", handle);

   if (handle != NULL)
   {
      region = (S_OSD_REGION *)handle;

      STB_OSSemaphoreWait(region->locked_sem);

      for (i = 0; i != region->num_colours; i++)
      {
         region->clut[i].r = (U8BIT)(trgb[i] >> 16) & 0xff;
         region->clut[i].g = (U8BIT)(trgb[i] >> 8) & 0xff;
         region->clut[i].b = (U8BIT)(trgb[i] & 0xff);
         region->clut[i].a = (U8BIT)(255 - ((trgb[i] >> 24) & 0xff));
      }

      STB_OSSemaphoreSignal(region->locked_sem);
   }

   FUNCTION_FINISH(STB_OSDSetRGBPalette);
}

/**
 * @brief   Sets Colour Palette array of up to 256 values, for single byte colour depth.
 *          This palette being an array of 'U32BIT' (8 Alpha, 8 Red, 8 Green, 8 Blue).
 * @param   index
 * @param   number   Size of palette array
 * @param   argb     pointer to palette array
 */
void STB_OSDMhegSetPalette(U16BIT index, U16BIT number, const U32BIT *argb)
{
   FUNCTION_START(STB_OSDMhegSetPalette);
   USE_UNWANTED_PARAM(index);
   USE_UNWANTED_PARAM(number);
   USE_UNWANTED_PARAM(argb);
   FUNCTION_FINISH(STB_OSDMhegSetPalette);
}

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
void* STB_OSDMhegSetResolution(U16BIT width, U16BIT height, U8BIT bits)
{
   FUNCTION_START(STB_OSDMhegSetResolution);

   MHEG_DBG("%u x %u, depth %u", width, height, bits);

   /*destroy the MHEG OSD region and backbuffer if they exists*/
   STB_OSMutexLock(mheg_mutex);

    if (display_status.mheg_surface.pixels != NULL)
    {
        STB_MEMFreeSysRAM(display_status.mheg_surface.pixels);
    }

    display_status.mheg_surface.width = width;
    display_status.mheg_surface.height= height;
    display_status.mheg_surface.pitch = width * bits / 8;
    display_status.mheg_surface.depth = bits;
    display_status.mheg_surface.pixels = STB_MEMGetSysRAM(display_status.mheg_surface.pitch * height);
    if (display_status.mheg_surface.pixels == NULL)
    {
        STB_SPDebugWrite("malloc fail: MHEG pixel buffer");
    }

    display_status.mheg_pixels = STB_MEMGetSysRAM(display_status.mheg_surface.pitch * height);
    if (display_status.mheg_pixels == NULL)
    {
        STB_SPDebugWrite("Failed to create MHEG pixel buffer");
    }

    STB_OSMutexUnlock(mheg_mutex);

    FUNCTION_FINISH(STB_OSDMhegSetResolution);

    return (void*)&display_status.mheg_surface;
}


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
void* STB_OSDMhegCreateSurface(U16BIT width, U16BIT height, BOOLEAN init, U32BIT colour)
{ 
   S_OSD_SURFACE *surface;
   S_RECTANGLE rect;

   FUNCTION_START(STB_OSDMhegCreateSurface);

   surface = CreateSurface(width, height, mheg_colourdepth);
   if ((surface != NULL) && init)
   {
      rect.top = 0;
      rect.left = 0;
      rect.width = width;
      rect.height = height;

      FillSurface(surface, &rect, colour, STB_BLIT_COPY);
   }

   FUNCTION_FINISH(STB_OSDMhegCreateSurface);

   return (void*)surface;
}

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
void* STB_OSDMhegLockBuffer( void *surface, U32BIT *pPitch )
{
   S_OSD_SURFACE *buffer = (S_OSD_SURFACE*)surface;
   void* data = NULL;

   FUNCTION_START(STB_OSDMhegLockBuffer);

   if (buffer != NULL)
   {
      data = buffer->pixels;
      *pPitch = buffer->pitch;
   }

   FUNCTION_FINISH(STB_OSDMhegLockBuffer);

   return data;
}

/**
 * @brief   This function informs HW that MHEG5 is finished writing to the buffer.
 * @param   surface Handle of surface returned by STB_OSDMhegCreateSurface
 * @return  void
 */
void STB_OSDMhegUnlockBuffer( void *surface )
{
   FUNCTION_START(STB_OSDMhegUnlockBuffer);
   USE_UNWANTED_PARAM(surface);
   FUNCTION_FINISH(STB_OSDMhegUnlockBuffer);
}

/**
 * @brief   This function destroys surface and all data allocated by
 *          STB_OSDMhegCreateSurface()
 * @param   surface Handle of surface returned by STB_OSDMhegCreateSurface
 * @return  void
 */
void STB_OSDMhegDestroySurface(void *surface)
{
   FUNCTION_START(STB_OSDMhegDestroySurface);

   DestroySurface(surface);

   FUNCTION_FINISH(STB_OSDMhegDestroySurface);
}


/**
 * @brief   Render bitmap on OSD back buffer in the given screen location, with given
 *          operation.
 *          The bitmap is referenced 'surface' - a handle returned by
 *          STB_OSDMhegCreateSurface()
 *          1. It is a one-to-one mapping between surface pixels and screen pixels, so
 *          rect.width and rect.height give size of rectangle on the screen as well.
 *          2. (rect.top + rect.height) is guarenteed to be less than or equal to height
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
   U16BIT screen_x, U16BIT screen_y, E_BLIT_OP bflg )
{
   S_OSD_SURFACE *buffer = (S_OSD_SURFACE*)surface;
   U32BIT* cursor_src;
   U8BIT* cursor_dst;


   FUNCTION_START(STB_OSDMhegBlitBitmap);

   if (display_status.mheg_surface.pixels != NULL)
   {
      cursor_src = (U32BIT *)buffer->pixels;
      cursor_src += (buffer->width * pRect->top) + pRect->left;

      cursor_dst = display_status.mheg_surface.pixels;
      cursor_dst += display_status.mheg_surface.pitch * screen_y;
      cursor_dst += screen_x * display_status.mheg_surface.depth / 8;
      if (bflg == STB_BLIT_A_BLEND)
      {
         MHEG_DBG("BlitBlend32(%d,%d,%u,%u) pitch=%u w=%u", screen_x, screen_y, pRect->width, pRect->height, pitch, buffer->width);
         BlitBlendSurface32(cursor_dst, cursor_src, pRect->width, pRect->height, pitch/4, display_status.mheg_surface.pitch);
      }
      else
      {
         MHEG_DBG("BlitCopy32(%d,%d,%u,%u) pitch=%u w=%u", screen_x, screen_y, pRect->width, pRect->height, pitch, buffer->width);
         BlitCopySurface32(cursor_dst, cursor_src, pRect->width, pRect->height, pitch/4, display_status.mheg_surface.pitch);
      }
      display_status.mheg_visible = TRUE;
   }

   FUNCTION_FINISH(STB_OSDMhegBlitBitmap);
}

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
void STB_OSDMhegFillRectangle( S_RECTANGLE *pRect, U32BIT colour, E_BLIT_OP bflg )
{
   FUNCTION_START(STB_OSDMhegFillRectangle);

   if (display_status.mheg_surface.pixels != NULL)
   {
        MHEG_DBG("FillSurface(%d,%d,%u,%u) colour=%x bflg=%u", pRect->left, pRect->top, pRect->width, pRect->height, colour, bflg);
        FillSurface(&display_status.mheg_surface, pRect, colour, bflg);
        if (colour != 0)
        {
            display_status.mheg_visible = TRUE;
        }
   }

   FUNCTION_FINISH(STB_OSDMhegFillRectangle);
}

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
   S_RECTANGLE *pDstRect, void *dst_surf, E_BLIT_OP bflg )
{
   FUNCTION_START(STB_OSDMhegBlitStretch);
   FUNCTION_FINISH(STB_OSDMhegBlitStretch);
}

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
void STB_OSDMhegFillSurface( void *surface, S_RECTANGLE *pRect, U32BIT colour, E_BLIT_OP bflg )
{
   FUNCTION_START(STB_OSDMhegFillSurface);

   MHEG_DBG("FillSurface(%d,%d,%u,%u) colour=%x bflg=%u", pRect->left, pRect->top, pRect->width, pRect->height, colour, bflg);
   FillSurface((S_OSD_SURFACE *)surface, pRect, colour, bflg);

   FUNCTION_FINISH(STB_OSDMhegFillSurface);
}

/**
 * @brief   Commit OSD changes to the screen - changes given by previous calls to
 *          STB_OSDMhegDrawRectangle() and STB_OSDMhegDrawBitmap().
 * @return  void
 */
void STB_OSDMhegUpdate(void)
{
   U32BIT size_bytes;
   U32BIT pixel;

   FUNCTION_START(STB_OSDMhegUpdate);

   STB_OSMutexLock(mheg_mutex);

   if (display_status.mheg_surface.pixels != NULL && display_status.mheg_pixels != NULL)
   {
        memcpy(display_status.mheg_pixels, display_status.mheg_surface.pixels, display_status.mheg_surface.pitch * display_status.mheg_surface.height);
        display_status.mheg_update = TRUE;
        STB_OSSemaphoreSignal(overlay_sem);
        STB_OSSemaphoreWait(update_sem);
   }

   STB_OSMutexUnlock(mheg_mutex);

   FUNCTION_FINISH(STB_OSDMhegUpdate);
}

/**
 * @brief   Clear MHEG's entire OSD
 */
void STB_OSDMhegClear(void)
{
    S_RECTANGLE rect;

    FUNCTION_START(STB_OSDMhegClear);

    MHEG_DBG("");

    rect.top = 0;
    rect.left = 0;
    rect.width = display_status.mheg_surface.width;
    rect.height = display_status.mheg_surface.height;
    display_status.mheg_visible = FALSE;

    STB_OSDMhegFillRectangle(&rect, 0x00, STB_BLIT_COPY);

    FUNCTION_FINISH(STB_OSDMhegClear);
}


/*---local function definitions----------------------------------------------*/

static void* CreateSurface(U16BIT width, U16BIT height, U8BIT depth)
{
   S_OSD_SURFACE *surface;

   FUNCTION_START(CreateSurface);

   surface = STB_MEMGetSysRAM(sizeof(S_OSD_SURFACE));
   if (surface != NULL)
   {
      surface->pixels = STB_MEMGetSysRAM(width * height * depth / 8);
      if (surface->pixels != NULL)
      {
         surface->depth = depth;
         surface->width = width;
         surface->height = height;
         surface->pitch = surface->width * depth / 8;
      }
      else
      {
         STB_MEMFreeSysRAM(surface);
         surface = NULL;
      }
   }

   FUNCTION_FINISH(CreateSurface);

   return (void*)surface;
}

static void DestroySurface(void *surface)
{
   FUNCTION_START(DestroySurface);

   if (surface != NULL)
   {
      STB_MEMFreeSysRAM(((S_OSD_SURFACE *)surface)->pixels);
      STB_MEMFreeSysRAM(surface);
   }

   FUNCTION_FINISH(DestroySurface);
}

static void FillSurface(S_OSD_SURFACE *surface, S_RECTANGLE *pRect, U32BIT colour, E_BLIT_OP bflg)
{
   U32BIT *pixel;
   U32BIT x,y;
   U32BIT src_a;
   U32BIT jump;

   FUNCTION_START(FillSurface);


   pixel = (U32BIT *)surface->pixels;
   pixel += (surface->width * pRect->top) + pRect->left;

   src_a = (colour >> 24) & 0xFF;
   jump = surface->width - pRect->width;

   if (bflg == STB_BLIT_A_BLEND && src_a != 0xFF)
   {
      int source_transparency = 0xff - src_a;
      U32BIT src_r, src_g, src_b;

      src_r = (colour >> 16) & 0xFF;
      src_g = (colour >> 8)  & 0xFF;
      src_b = colour & 0xFF;

      y = pRect->height;
      while (y--)
      {
         x = pRect->width;
         while (x--)
         {
            U32BIT R, G, B, A;
            U32BIT dc = *pixel;
            R = dc & 0xFF;
            G = (dc >> 8) & 0xFF;
            B = (dc >> 16) & 0xFF;
            R = (R + ((src_r - R) * src_a >> 8));
            G = (G + ((src_g - G) * src_a >> 8)) << 8;
            B = (B + ((src_b - B) * src_a >> 8)) << 16;

            int dest_opacity = (dc >> 24) & 0xFF;
            int dest_transparency = 0xff - dest_opacity;
            int final_transparency = (dest_transparency * source_transparency) >> 8;
            A = (0xff - final_transparency) << 24;
            *pixel++ = R | G | B | A;
         }
         pixel += jump;
      }
   }
   else
   {
      U32BIT col = (colour & 0xFF00FF00) | ((colour & 0xFF) << 16) | ((colour >> 16) & 0xFF);
      y = pRect->height;
      while (y--)
      {
         x = pRect->width;
         while (x--)
         {

            *pixel++ = col;
         }
         pixel += jump;
      }
    }

    FUNCTION_FINISH(FillSurface);
 }


static void BlitBlendSurface32(U8BIT *dst, U32BIT *src, U32BIT width, U32BIT height, U16BIT s_pitch, U16BIT d_pitch)
{
   U32BIT x;
   d_pitch -= (width * 4);
   s_pitch -= width;
   MHEG_DBG("d_pitch=%u s_pitch=%u", d_pitch, s_pitch );
   while (height--)
   {
      x = width;
      while (x--)
      {
         U32BIT scol = *src++;
         U32BIT src_a = scol >> 24;
         if (src_a == 0)
         {
            // Skip pixel - source has no effect
            dst += 4;
         }
         else if (src_a == 0xFF)
         {
            *dst++ = (scol >> 16) & 0xFF; // Red
            *dst++ = (scol >> 8) & 0xFF;  // Green
            *dst++ = scol & 0xFF;         // Blue
            *dst++ = 0xFF;                // Alpha
         }
         else
         {
            U32BIT src_r, src_g, src_b;
            U32BIT R, G, B;
            int dst_trans = 0xFF - dst[3];
            int src_trans = 0xFF - src_a;

            src_r = (scol >> 16) & 0xFF;
            src_g = (scol >> 8)  & 0xFF;
            src_b = scol & 0xFF;

            R = dst[0];
            G = dst[1];
            B = dst[2];

            *dst++ = (U8BIT)((R + ((src_r - R) * src_a >> 8)) & 0xFF);
            *dst++ = (U8BIT)((G + ((src_g - G) * src_a >> 8)) & 0xFF);
            *dst++ = (U8BIT)((B + ((src_b - B) * src_a >> 8)) & 0xFF);
            *dst++ = 0xFF - ((dst_trans * src_trans) >> 8);
         }
      }
      dst += d_pitch;
      src += s_pitch;
   }
}

static void BlitCopySurface32(U8BIT *dst, U32BIT *src, U32BIT width, U32BIT height, U16BIT s_pitch, U16BIT d_pitch)
{
   U32BIT x;

   d_pitch -= (width * 4);
   s_pitch -= width;
   MHEG_DBG("d_pitch=%u s_pitch=%u", d_pitch, s_pitch );
   while (height--)
   {
      x = width;
      while (x--)
      {
         U32BIT scol = *src++;
         *dst++ = (scol >> 16) & 0xFF; // Red
         *dst++ = (scol >> 8) & 0xFF;  // Green
         *dst++ = scol & 0xFF;         // Blue
         *dst++ = (scol >> 24) & 0xFF; // Alpha
      }
      dst += d_pitch;
      src += s_pitch;
   }
}

static void OverlayTask(void *param)
{
   BOOLEAN redraw;
   ASSERT(mheg_mutex);

   OverlaySetSize(SCREEN_ID, display_status.scale, display_status.screen_start_x, display_status.screen_start_y, display_status.screen_width, display_status.screen_height);
   OverlaySetSize(MHEG5_ID,  0, 0, 0, display_status.mheg_surface.width, display_status.mheg_surface.height);

   redraw = FALSE;
   while (1)
   {
      STB_OSSemaphoreWait(overlay_sem);
      if (display_status.osd_resoln) {
         display_status.osd_resoln = false;
         OverlaySetSize(SCREEN_ID, display_status.scale, display_status.screen_start_x, display_status.screen_start_y, display_status.screen_width, display_status.screen_height);
         redraw = TRUE;
      }
      if (display_status.subt_resoln)
      {
         display_status.subt_resoln = FALSE;
         OverlaySetSize(SUBTITLE_ID, 0, 0, 0, display_status.subt_surface.width, display_status.subt_surface.height);
      }
      if (display_status.subt_update)
      {
         STB_OSMutexLock(subtitle_mutex);
         display_status.subt_update = FALSE;
         if (display_status.subt_visible)
         {
            OverlayUpdate(SUBTITLE_ID, 0, 0, display_status.subt_surface.width, display_status.subt_surface.height,
               display_status.subt_surface.pixels);
         }
         else
         {
             OverlayUpdate(SUBTITLE_ID, 0, 0, 0, 0, NULL);
         }
         STB_OSMutexUnlock(subtitle_mutex);
         redraw = TRUE;
      }

      if (display_status.mheg_update)
      {
         STB_OSMutexLock(mheg_mutex);
         display_status.mheg_update = FALSE;
         if (display_status.mheg_visible)
         {
            OverlayUpdate(MHEG5_ID, 0, 0, display_status.mheg_surface.width, display_status.mheg_surface.height,
               display_status.mheg_pixels);
         }
         else
         {
             OverlayUpdate(MHEG5_ID, 0, 0, 0, 0, NULL);
         }

         STB_OSMutexUnlock(mheg_mutex);
         redraw = TRUE;
      }

      if (redraw)
      {
         redraw = FALSE;
         OverlayDisplay();
      }
      STB_OSSemaphoreSignal(update_sem);
    }
 }


