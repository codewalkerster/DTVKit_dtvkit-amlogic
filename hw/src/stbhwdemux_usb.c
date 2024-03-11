/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: usbcam handler.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include "dtv_log.h"
#define TAG  "STBHWDEMUX_USB"
/* third party header files */
#include <dmx.h>
#include "techtype.h"
#include "dbgfuncs.h"

#include "stbhwmem.h"
#include "stbhwcfg.h"
#include "internal.h"
#include "stbhwdef.h"
#include "stbhwc.h"
#include "stbhwos.h"
#include "stbhwdmx.h"

#include "stb_utils.h"
#include "stbhwdemux_usb.h"
#include "usbcimoduleapi.h"
//#include <Aml_MP/Aml_MP.h>

/*---constant definitions for this file--------------------------------------*/
//#define INJECT_FROM_FILE
// #define DEMUX_USB_MODULE_DEBUG
#define CIPLUS_USB_INDEX 1
#define DEMUX_USB_DEBUG
#ifdef DEMUX_USB_DEBUG
#define DMX_USB_DBG(x, ...) DTV_LOG(ANDROID_LOG_INFO, TAG, "CIP_USB %s:%d " x, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define DMX_USB_DBG(x, ...)
#endif

#define USBCAM_UNPLUG (-71)
#define USBCAM_NODEVICE (-19)

#define REC_BUFF_SIZE (USB_CIMODULE_MEDIA_MAX_SIZE * 120)

#define MEDIA_INPUT_ENABLE 1
#define MEDIA_OUTPUT_ENABLE 1

// #define DMX_USB_TEST
// #define SMITTEST
static int rec_dev_id;
static int inj_dev_id;
static int rec_dvr_fd = -1;
static int rec_dmx_fd = -1;
static int inj_dvr_fd = -1;
static int ev_fd;
static BOOLEAN thread_running = FALSE;

static int cmd_r_fd = -1;
static int cmd_w_fd = -1;
static int media_read_fd = -1;
static int media_write_fd = -1;

static pthread_mutex_t media_read_condmut;
static pthread_mutex_t media_write_condmut;

static unsigned char *media_writebuf = NULL;
static unsigned char *media_readbuf = NULL;

pthread_mutex_t cmd_read_mutex;
pthread_mutex_t resource_mutex;

static BOOLEAN module_inserted = FALSE;
static BOOLEAN module_init = FALSE;
static BOOLEAN mutex_init = FALSE;


static pthread_t media_read_taskid;
static pthread_t media_write_taskid;

static BOOLEAN prepare_working_demuxes();
static void *cimodule_media_read_task(void *args);
static void *cimodule_media_write_task(void *args);
static int record_from_tsin(void *buff, int buff_len);
static int inject_usbcam_source_demux(void *data, int data_len);
static BOOLEAN set_usbcam_recording_demux(int source);

//---local function prototypes for this file-----------------------------------
static void init_mutex()
{
    if (!mutex_init)
    {
        pthread_mutex_init(&cmd_read_mutex, NULL);
        pthread_mutex_init(&resource_mutex, NULL);
        pthread_mutex_init(&media_write_condmut, NULL);
        pthread_mutex_init(&media_read_condmut, NULL);
        mutex_init = TRUE;
    }
}

static void reset_resource()
{
    DMX_USB_DBG("enter");
    if (cmd_r_fd > 0)
    {
        close(cmd_r_fd);
        cmd_r_fd = -1;
    }
    if (cmd_w_fd > 0)
    {
        close(cmd_w_fd);
        cmd_w_fd = -1;
    }
    if (media_readbuf)
    {
        STB_MEMFreeSysRAM(media_readbuf);
        media_readbuf = NULL;
    }
    if (media_writebuf)
    {
        STB_MEMFreeSysRAM(media_writebuf);
        media_writebuf = NULL;
    }
    if (media_read_fd > 0)
    {
        close(media_read_fd);
        media_read_fd = -1;
    }
    if (media_write_fd > 0)
    {
        close(media_write_fd);
        media_write_fd = -1;
    }
    if (rec_dmx_fd > 0)
    {
        ioctl(rec_dmx_fd, DMX_STOP, 0);
        close(rec_dmx_fd);
        rec_dmx_fd = -1;
    }
    if (rec_dvr_fd > 0)
    {
        close(rec_dvr_fd);
        rec_dvr_fd = -1;
    }
    if (inj_dvr_fd > 0)
    {
        ioctl(inj_dvr_fd, DMX_SET_INPUT, INPUT_DEMOD);
        close(inj_dvr_fd);
        inj_dvr_fd = -1;
    }
}

static void *cimodule_media_read_task(void *args)
{
    int ret, read_len, inj_len, usbdata_len = 0;
    int save_fd = -1;
    struct usb_cimodule_info usbci_module_info = {0};
    unsigned char media_output_ctrl = 0;
    char *usbdata_buf;
    int count = 0;
    usbdata_buf = STB_MEMGetSysRAM(USB_CIMODULE_MEDIA_MAX_SIZE * 100);
    if (!usbdata_buf)
    {
        DMX_USB_DBG("no mem to alloc usbdata_buf");
        return NULL;
    }

    DMX_USB_DBG("entry");
    ret = ioctl(media_read_fd, AML_USBCAM_IOC_GET_INFO, &usbci_module_info);
    if (ret < 0)
    {
        DMX_USB_DBG("(handle: %d),get device info error,error code:%d", media_read_fd, ret);
        STB_MEMFreeSysRAM(usbdata_buf);
        return NULL;
    }

#ifdef DEMUX_USB_MODULE_DEBUG
    DMX_USB_DBG("usbci_module_info.is_ci20_detected %d",usbci_module_info.is_ci20_detected);
    DMX_USB_DBG("usbci_module_info.ci_compatibility 0x%x",usbci_module_info.ci_compatibility);
#endif

    if (!usbci_module_info.is_ci20_detected)
    {
        // refer to CI_OVER_USB_1.0 SPEC
        media_output_ctrl = (unsigned char)((usbci_module_info.ci_compatibility >> 7) & 0x01);
        if (MEDIA_OUTPUT_ENABLE != media_output_ctrl)
        {
            DMX_USB_DBG("can not read media from usb ci module in this mode");
            STB_MEMFreeSysRAM(usbdata_buf);
            return NULL;
        }
    }

    while (thread_running)
    {
        read_len = read(media_read_fd, media_readbuf, USB_CIMODULE_MEDIA_MAX_SIZE);
        if (read_len < 0) {
            DMX_USB_DBG("read error: read_len = %d, [%d]%s", read_len, -errno, strerror(errno));
            if (((-errno) == USBCAM_UNPLUG) || ((-errno) == USBCAM_NODEVICE))
                goto EXIT;
        }
        if (read_len > 0)
        {
            // Dummy data
            if (read_len == 10)
            {
                continue;
            }
            else
            {
                // TODO: How to determine precise short packet length
                if (read_len < 1024 * 5)
                {
                    DMX_USB_DBG("short packet");
                    // available data + short packet, so if short packet comes first, drop it.
                    if (usbdata_len == 0)
                        continue;
                }
                memcpy(usbdata_buf + usbdata_len, media_readbuf, read_len);
                usbdata_len += read_len;
                if (usbdata_len >= USB_CIMODULE_MEDIA_MAX_SIZE)
                {
                    inj_len = inject_usbcam_source_demux(usbdata_buf, usbdata_len);
                    if (inj_len < 0)
                        continue;
                    usbdata_len -= inj_len;
                    if (usbdata_len > 0)
                        memmove(usbdata_buf, usbdata_buf + inj_len, usbdata_len);
                }

#ifdef DEMUX_USB_MODULE_DEBUG
                DMX_USB_DBG("read %d, inject %d", read_len, inj_len);
#endif

#ifdef DMX_USB_TEST
                if (save_fd < 0)
                    save_fd = open("/data/w.ts", O_RDWR | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
                write(save_fd, media_readbuf, read_len);
#endif
            }
        }
        else
        {
            // if ((-errno) == USBCAM_UNPLUG)
            //     goto EXIT;
        }
    }

EXIT:
    DMX_USB_DBG("usbcam unplug, media read task exit.");
    module_inserted = FALSE;
    STB_MEMFreeSysRAM(usbdata_buf);

    return NULL;
}

static void *cimodule_media_write_task(void *args)
{
    int ret,inj_len,usbdata_len = 0;
    int write_len, rec_len, threshold = 0;

    struct usb_cimodule_info usbci_module_info;
    usbci_module_capabilities_t  usbci_module_capabilities;

    unsigned char media_input_ctrl;
    unsigned char *buffer;
    static int fd = -1;
    int count = 0;
    unsigned char arDummyTsHdr[10] = {0x00, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    BOOLEAN first_run = TRUE;
    int i;
    DMX_USB_DBG("entry");
    buffer = STB_MEMGetSysRAM(REC_BUFF_SIZE);

    DMX_USB_DBG("open usb cimodlue media interface successfully,handle:%d", media_write_fd);

    ret = ioctl(media_write_fd, AML_USBCAM_IOC_GET_INFO, &usbci_module_info);
    if (ret < 0)
    {
        DMX_USB_DBG("(handle: %d),get device info error,error code:%d", media_write_fd, ret);
        STB_MEMFreeSysRAM(buffer);
        return NULL;
    }

#ifdef DEMUX_USB_MODULE_DEBUG
    DMX_USB_DBG("usbci_module_info.is_ci20_detected %d",usbci_module_info.is_ci20_detected);
    DMX_USB_DBG("usbci_module_info.ci_compatibility 0x%x",usbci_module_info.ci_compatibility);
#endif

    if (!usbci_module_info.is_ci20_detected)
    {
        // refer to CI_OVER_USB_1.0 SPEC
        media_input_ctrl = (unsigned char)((usbci_module_info.ci_compatibility >> 6) & 0x01);
        if (MEDIA_INPUT_ENABLE != media_input_ctrl)
        {
            DMX_USB_DBG("can not write media to usb ci module in this mode");
            STB_MEMFreeSysRAM(buffer);
            return NULL;
        }
    }
    DMX_USB_DBG("ci20 detected ok");

    while (thread_running)
        {
#ifndef INJECT_FROM_FILE
            rec_len = record_from_tsin(buffer + threshold, USB_CIMODULE_MEDIA_MAX_SIZE * 10);
#else
            if (fd <= 0)
                fd = open("/data/test.ts", O_RDONLY);
            rec_len = read(fd, buffer, USB_CIMODULE_MEDIA_MAX_SIZE);
#endif
            if (rec_len > 0)
                threshold += rec_len;

            if (first_run)
            {
                if (threshold <= USB_CIMODULE_MEDIA_MAX_SIZE*100)
                {
#ifdef DEMUX_USB_MODULE_DEBUG
                    // DMX_USB_DBG("first run ,%x buf %d rec %d", buffer[0], threshold, rec_len);
#endif
                    continue;
                }
            }

            while ((threshold > USB_CIMODULE_MEDIA_MAX_SIZE) && thread_running)
            {

                memcpy(media_writebuf, arDummyTsHdr, 10);
                if (first_run)
                {
                    first_run = FALSE;
                    media_writebuf[3] |= (1<<7);
                }
                ret = write(media_write_fd, media_writebuf, 10);

                memcpy(media_writebuf, buffer, USB_CIMODULE_MEDIA_MAX_SIZE);
                if (ret > 0)
                {
                    write_len = write(media_write_fd, media_writebuf, USB_CIMODULE_MEDIA_MAX_SIZE);
                    if (write_len < 0)
                    {
                        DMX_USB_DBG("write usb cam failed!!!! %d", write_len);
                        goto EXIT;
                    }
#ifdef DEMUX_USB_MODULE_DEBUG
                    DMX_USB_DBG("write ts len %d", write_len);
#endif
                    if (threshold - write_len > 0)
                    {
                        memmove(buffer, buffer + write_len, threshold - write_len);
                    }
                    if (threshold - write_len < 0)
                        DMX_USB_DBG("write data error occur");
                    threshold -= write_len;
                }
                else if (ret < 0)
                {
                    DMX_USB_DBG("write error: ret = %d, [%d]%s", ret, -errno, strerror(errno));
                    if (((-errno) == USBCAM_UNPLUG) || ((-errno) == USBCAM_NODEVICE))
                        goto EXIT;
                }
            }
        }

EXIT:
    DMX_USB_DBG("usbcam unplug, media write task exit.");
    module_inserted = FALSE;
    if (buffer > 0)
        STB_MEMFreeSysRAM(buffer);

    return NULL;
}

static BOOLEAN set_usbcam_recording_demux(int source)
{
    char rec_dmx_path[64];
    struct dmx_pes_filter_params params;
    int ret;

    STB_DMXSetSource(rec_dev_id, source);
    DMX_USB_DBG("================= set usb camcard data source %d", source);

    snprintf(rec_dmx_path, sizeof(rec_dmx_path), "/dev/dvb0.demux%d", rec_dev_id);
    if (rec_dmx_fd < 0)
        rec_dmx_fd = open(rec_dmx_path, O_RDWR);
    DMX_USB_DBG("================= rec_dmx_path %s", rec_dmx_path);
    DMX_USB_DBG("================= rec_dmx_fd %d", rec_dmx_fd);

    if (fcntl(rec_dmx_fd, F_SETFL, O_NONBLOCK) == -1)
    {
        DMX_USB_DBG("================= rec_dmx_fd %d fail to set non-block mode", rec_dmx_fd);
        return FALSE;
    }
    memset(&params, 0, sizeof(params));

    params.pid = 0x2000;
    // params.pid = 0x211;
    params.input = DMX_IN_FRONTEND;
    params.output = DMX_OUT_TS_TAP;
    params.pes_type = DMX_PES_OTHER;

    ret = ioctl(rec_dmx_fd, DMX_SET_PES_FILTER, &params);
    if (ret < 0)
    {
        DMX_USB_DBG("DMX_SET_PES_FILTER failed");
        return FALSE;
    }
    ret = ioctl(rec_dmx_fd, DMX_START, 0);
    if (ret < 0)
    {
        DMX_USB_DBG("DMX_START failed");
        return FALSE;
    }
    return TRUE;
}

static BOOLEAN prepare_working_demuxes()
{
    char inj_dvr_path[64];
    char rec_dvr_path[64];

    ev_fd = eventfd(0, 0);
    snprintf(inj_dvr_path, sizeof(inj_dvr_path), "/dev/dvb0.dvr%d", inj_dev_id);
    if (inj_dvr_fd < 0)
        inj_dvr_fd = open(inj_dvr_path, O_WRONLY);
    if (inj_dvr_fd < 0)
    {
#ifdef DEMUX_USB_MODULE_DEBUG
        DMX_USB_DBG("inj_dvr_fd open failed");
#endif
        return FALSE;
    }
    ioctl(inj_dvr_fd, DMX_SET_INPUT, INPUT_LOCAL);

    snprintf(rec_dvr_path, sizeof(rec_dvr_path), "/dev/dvb0.dvr%d", rec_dev_id);
    if (rec_dvr_fd < 0)
        rec_dvr_fd = open(rec_dvr_path, O_RDONLY);
    if (rec_dvr_fd < 0)
    {
#ifdef DEMUX_USB_MODULE_DEBUG
        DMX_USB_DBG("rec_dvr_fd open failed");
#endif
        return FALSE;
    }
    if (fcntl(rec_dvr_fd, F_SETFL, fcntl(rec_dvr_fd, F_GETFL, 0) | O_NONBLOCK, 0) == -1)
    {
#ifdef DEMUX_USB_MODULE_DEBUG
        DMX_USB_DBG("set rec_dvr_fd non-block failed");
#endif
        return FALSE;
    }
    ioctl(rec_dvr_fd, DMX_SET_BUFFER_SIZE, REC_BUFF_SIZE);

    return set_usbcam_recording_demux(aml_hw_cfg.tuners[aml_hw_cfg.tuner_num - 1].ts_input_idx);
}

static int record_from_tsin(void *buff, int buff_len)
{
    int ret, read_len;
    struct pollfd fds[2];

    memset(fds, 0, sizeof(fds));

    fds[0].fd = rec_dvr_fd;
    fds[1].fd = ev_fd;
    fds[0].events = fds[1].events = POLLIN | POLLERR;
    ret = poll(fds, 2, 20);
    if (ret <= 0)
    {
#ifdef DEMUX_USB_MODULE_DEBUG
        DMX_USB_DBG("poll ret %d rec_dvr_fd %d rec_dmx_fd %d", ret, rec_dvr_fd, rec_dmx_fd);
#endif
        return -1;
    }

    if (!(fds[0].revents & POLLIN))
    {
#ifdef DEMUX_USB_MODULE_DEBUG
        DMX_USB_DBG("fds revents %d", fds[0].revents);
#endif
    }
    ret = read(rec_dvr_fd, buff, buff_len);
    return ret;
}

static int inject_usbcam_source_demux(void *data, int data_len)
{
    return write(inj_dvr_fd, data, data_len);
}

/**
 * \brief STB_CIUsbOpen
 *        called by usbt, usb monitor thread will call this function
 *          to see if usb cam is plug in/unplug.
 * \return TRUE if success
 */
int STB_CIUsbOpen()
{
    unsigned int driver_version = 0;

    usbci_module_capabilities_t  usbci_module_capabilities;
    int ret = 0;
    const char *cmd_node = "/dev/cimodule_command0";
    const char *media_node = "/dev/cimodule_media0";

    init_mutex();

    pthread_mutex_lock(&resource_mutex);
    inj_dev_id = 4;
    rec_dev_id = 5;

    if (module_init)
    {
        DMX_USB_DBG("module already init, skip open");
        pthread_mutex_unlock(&resource_mutex);
        return TRUE;
    }

    if (0 == access(cmd_node, F_OK) && 0 == access(media_node, F_OK)) {
            cmd_r_fd = open(cmd_node, O_RDONLY | O_NONBLOCK);
            cmd_w_fd = open(cmd_node, O_WRONLY);
            if (cmd_r_fd < 0 || cmd_w_fd < 0) {
                DMX_USB_DBG("open %s failed", cmd_node);
                goto ERR;
            }
    } else {
        DMX_USB_DBG("access %s failed", cmd_node);
        goto ERR;
    }

    if (media_read_fd < 0)
        media_read_fd = open(media_node, O_RDONLY);
    if (media_write_fd < 0)
        media_write_fd = open(media_node, O_WRONLY);
    if ((media_read_fd < 0) || (media_write_fd < 0))
    {
        DMX_USB_DBG("Failed to open media device, read fd: %d write: %d", media_read_fd, media_write_fd);
        goto ERR;
    }

    ioctl(cmd_r_fd, AML_USBCAM_IOC_GET_DRIVER_VERSION, &driver_version);
    DMX_USB_DBG("usbcimodule driver version: %d.%d.%d.%d",
                (driver_version & 0xFF000000) >> 24,
                (driver_version & 0x00FF0000) >> 16,
                (driver_version & 0x0000FF00) >> 8,
                driver_version & 0x000000FF);

    ioctl(cmd_r_fd, AML_USBCAM_IOC_MODULE_CAPABILITIES, &usbci_module_capabilities);
    DBG("ci_manufacturer_name = %s\n",usbci_module_capabilities.ci_manufacturer_name);
    DBG("ci_product_name = %s\n",usbci_module_capabilities.ci_product_name);
    DBG("ci_plus_supported = %d\n",usbci_module_capabilities.ci_plus_supported);
    DBG("op_profile_supported = %d\n",usbci_module_capabilities.op_profile_supported);

    media_writebuf = STB_MEMGetSysRAM(USB_CIMODULE_MEDIA_MAX_SIZE);
    media_readbuf = STB_MEMGetSysRAM(USB_CIMODULE_MEDIA_MAX_SIZE);

    if (prepare_working_demuxes() == FALSE)
    {
        DMX_USB_DBG("prepare_working_demuxes() failed");
        goto ERR;
    }
    module_init = TRUE;
    module_inserted = TRUE;
    thread_running = TRUE;
    ret = pthread_create(&media_read_taskid, NULL, (void *)cimodule_media_read_task, NULL);
    if (ret != 0)
        goto ERR;
    ret = pthread_create(&media_write_taskid, NULL, (void *)cimodule_media_write_task, NULL);
    if (ret != 0)
        goto ERR;

    pthread_mutex_unlock(&resource_mutex);
    return TRUE;

ERR:
    DMX_USB_DBG("error occur, go exit.");
    reset_resource();

    module_inserted = FALSE;
    thread_running = FALSE;

    pthread_mutex_unlock(&resource_mutex);

    return FALSE;
}

/**
 * \brief STB_CIUsbClose
 *        called by usbt, if device node open failed, or spdu transfer failed
 *          then this function will be called to release resource.
 * \return 0 if success
 */
int STB_CIUsbClose()
{
    void *status = NULL;

    DMX_USB_DBG("exit.");

    if (!module_init)
    {
        DMX_USB_DBG("usbci module not opened");
        return -1;
    }

    thread_running = FALSE;

    pthread_mutex_lock(&resource_mutex);

    if (pthread_join(media_read_taskid, &status) != 0)
    {
        DMX_USB_DBG("media read task join failed");
    }
    if (pthread_join(media_write_taskid, &status) != 0)
    {
        DMX_USB_DBG("media write task join failed");
    }

    reset_resource();
    module_init = FALSE;

    pthread_mutex_unlock(&resource_mutex);

    return 0;
}

S32BIT STB_CIUsbWrite(U8BIT *buffer, U32BIT len)
{
    int ret = -1;

#ifdef DEMUX_USB_MODULE_DEBUG
    char buf[2048];
    char errorbuf[2048];
    unsigned int i;
#endif

    if (len > USB_CIMODULE_COMMAND_MAX_SIZE)
    {
        DMX_USB_DBG("write data is longer than buffer size, failed");
        return -1;
    }

    if (cmd_w_fd > 0)
        ret = write(cmd_w_fd, buffer, len);

    if (ret < 0)
        DMX_USB_DBG("send command error: ret = %d, [%d]%s", ret, -errno, strerror(errno));

#ifdef DEMUX_USB_MODULE_DEBUG
    if (ret > 0 && ret < 256)
    {
        for (i = 0; i < ret; i++)
            sprintf(buf + 3 * i, "%02x ", buffer[i]);
        DMX_USB_DBG("Write %d =========> %s", ret, buf);
    }
#endif

    return ret;
}

/**
 * \brief STB_CIUsbRead
 *        Read data from usbcam.
 * \param buffer, read buffer
 * \param len read len.
 * \return readlen.
 */
S32BIT STB_CIUsbRead(U8BIT *buffer, U32BIT len)
{
    int ret;

#ifdef DEMUX_USB_MODULE_DEBUG
    char buf[2048];
#endif
    int i;
    int read_len = 0;
    struct pollfd fds[1];
    int timeout_ms = 0;

    if (module_inserted == FALSE)
    {
        DMX_USB_DBG("module closed, exit.");
        return -1;
    }

    fds[0].fd = cmd_r_fd;
    fds[0].events = POLLIN | POLLERR;

    if (cmd_r_fd > 0) {
        ret = poll(fds, 1, timeout_ms);

        if ((ret == 1) && (fds[0].revents & POLLIN)) {
            ret = read(cmd_r_fd, buffer, len);
            if (ret < 0)
                DMX_USB_DBG("read command error: ret = %d, [%d]%s", ret, -errno, strerror(errno));
        } else if(fds[0].revents & POLLERR) {
            ret = -1;
        }
    }
    else
        ret = -1;

#ifdef DEMUX_USB_MODULE_DEBUG
        if (ret > 0 && ret <= 256)
        {
            for (i = 0; i < ret; i++)
                sprintf(buf + 3 * i, "%02x ", buffer[i]);
            DMX_USB_DBG("Read %d =========> %s", ret, buf);
        }
#endif

    return ret;
}

U8BIT STB_CIUsbCamTotal(void)
{
    return 1;
}

/**
 * \brief STB_DMXUsbGetTsDemux
 *        get the inject usb demux number, and set other
 *        demux source to DMA.
 * \return 0 if success
 */
int STB_DMXUsbGetTsDemux()
{
    return inj_dev_id;
}

/**
 * \brief STB_DMXUsbIsEnable
 *        if usb cam function is valid.
 * \return 0 if success
 */
BOOLEAN STB_DMXUsbIsEnable()
{
    return TRUE;
}

/**
 * \brief   When ts data route using usbcam, play/record etc demux
 *          need set source to usbcam demux.
 *          This function will return usbcam demux number.
 * \param live if requirement is called by live path.
 * \return  demux source in code.
 */
U8BIT STB_CIUsbGetDmxSource(BOOLEAN live)
{
    if (live)
        return inj_dev_id;
    else
        return inj_dev_id;
}

/**
 * \brief   Check if usbcam is plugged.
 * \return  TRUE if cam is inserted.
 */
BOOLEAN STB_CIUsbModuleInserted()
{
    return module_inserted;
}
