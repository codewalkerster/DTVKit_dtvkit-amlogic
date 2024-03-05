/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _STBHWDMXUSB_H
#define _STBHWDMXUSB_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SUBFRAG 16
#define FRAG_SIZE 10

/*
 * Negative Error values for read/write operations
 * Note: CIUSB_NOT_ENABLED is treated as though CAM remains plugged in
 */
#define CIUSB_GEN_ERROR -1
#define CIUSB_NOT_ENABLED -2 /* value = -ENOENT */
#define CIUSB_NO_DEVICE -19  /* value = -ENODEV */

    typedef struct DataBlock_s DataBlock;
    struct DataBlock_s
    {
        size_t left;
        size_t start;
        DataBlock *next;
        uint8_t data[4096];
    };

    struct fragment_header
    {
        U8BIT protocol_version;
        U8BIT LTS_id;
        U8BIT track_id;
        U8BIT flush;
        U8BIT first_fragment;
        U8BIT last_fragment;
        U8BIT reserved_future_use;
        U32BIT number_subsamples;

#if 0 // Mpeg ts mode need no sub_fragment.
    struct sub_fragment
    {
        U16BIT clear_bytes;
        U16IT encrypted_bytes;
        U8BIT crypto_reload_period;
        U8BIT scrambling_control;
        U8BIT padding_size;
        U16BIT padding_offset;
    }sub_frags[MAX_SUBFRAG];
    U16BIT descriptor_length;
    for (i = 0; i < N; i++)
    {
        descriptor()
    }
#endif
    };

    ///////////////////////////////////////////////////////////////////////////////
    //                                  Aml_MP                                   //
    ///////////////////////////////////////////////////////////////////////////////
    /**
     * \brief STB_DMXUsbModuleInit
     *        1. Alloc record/inject demux
     *        2. Create injection thread.
     * \return 0 if success
     */
    BOOLEAN STB_DMXUsbModuleInit();

    /**
     * \brief STB_DMXUsbModuleExit
     * \return no ret
     */
    void STB_DMXUsbModuleExit();

    /**
     * \brief STB_DMXUsbGetTsDemux
     *        get the inject usb demux number, and set other
     *        demux source to DMA.
     * \return 0 if success
     */
    int STB_DMXUsbGetTsDemux();

    /**
     * \brief STB_DMXUsbIsEnable
     *        if usb cam function is valid.
     * \return 0 if success
     */
    BOOLEAN STB_DMXUsbIsEnable();

    /**
     * \brief STB_CIUsbOpen
     *        called by usbt, usb monitor thread will call this function
     *          to see if usb cam is plug in/unplug.
     * \return 0 if success
     */
    int STB_CIUsbOpen();

    /**
     * \brief STB_CIUsbClose
     *        called by usbt, if device node open failed, or spdu transfer failed
     *          then this function will be called to release resource.
     * \return 0 if success
     */
    int STB_CIUsbClose();

    /**
     * \brief STB_CIUsbRead
     *        Read data from usbcam.
     * \param buffer read buffer
     * \param len read len.
     * \return 0 if success
     */
    S32BIT STB_CIUsbRead(U8BIT *buffer, U32BIT len);

    /**
     * \brief STB_CIUsbWrite
     *        write data to usbcam.
     * \param buffer write buffer
     * \param len write len.
     * \return readlen
     */
    S32BIT STB_CIUsbWrite(U8BIT *buffer, U32BIT len);

    /**
     * \brief   Return number of supported USB CAMs on the receiver
     * \return  Number supported
     */
    U8BIT STB_CIUsbCamTotal(void);

    /**
     * \brief   Check if usbcam is plugged.
     * \return  TRUE if cam is inserted.
     */
    BOOLEAN STB_CIUsbModuleInserted();

    /**
     * \brief   When ts data route using usbcam, play/record etc demux
     *          need set source to usbcam demux.
     *          This function will return usbcam demux number.
     * \param live if requirement is called by live path.
     * \return  demux source in code.
     */
    U8BIT STB_CIUsbGetDmxSource(BOOLEAN live);

    static int inject_ts(void *data, int data_len);

#ifdef __cplusplus
}
#endif

#endif
