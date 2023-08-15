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
#include "stbdpc.h"
#include "stb_utils.h"
#include "stbhwdemux_usb.h"
#include "usbcimoduleapi.h"
#include <Aml_MP/Aml_MP.h>

/*---constant definitions for this file--------------------------------------*/
// #define INJECT_FROM_FILE
// #define DEMUX_USB_MODULE_DEBUG
#define CIPLUS_USB_INDEX 1
#define DEMUX_USB_DEBUG 1
#ifdef DEMUX_USB_DEBUG
#define DMX_USB_DBG(x, ...) DTV_LOG(ANDROID_LOG_INFO, TAG, "CIP_USB %s:%d " x, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define DMX_USB_DBG(x, ...)
#endif

#define REC_BUFF_SIZE (USB_CIMODULE_MEDIA_MAX_SIZE * 120)

#define MEDIA_INPUT_ENABLE 1
#define MEDIA_OUTPUT_ENABLE 1
#define USBCAM_UNPLUG (-71)
// #define DMX_USB_TEST

static int rec_dev_id;
static int inj_dev_id;
static int rec_dvr_fd = -1;
static int rec_dmx_fd = -1;
static int inj_dvr_fd = -1;
static int ev_fd;
static BOOLEAN thread_running = FALSE;
static int cmd_rw_fd = -1;
static int media_read_fd = -1;
static int media_write_fd = -1;
static pthread_mutex_t gs_tMediaReadCondMut;
static pthread_mutex_t gs_tMediaWriteCondMut;
static unsigned char *g_pCmdWriteBuf = NULL;
static unsigned char *g_pCmdReadBuf = NULL;
static unsigned char *g_pbMediaReadBuf = NULL;
static unsigned char *g_pbMediaWriteBuf = NULL;
pthread_mutex_t cmd_read_mutex;
pthread_mutex_t resource_mutex;
static char ci_cmd_buffer[1024];
static int ci_cmd_len;
static BOOLEAN module_inserted = FALSE;
static BOOLEAN module_init = FALSE;
static BOOLEAN mutex_init = FALSE;

static DataBlock *data_block_head = NULL;

static pthread_t tMediaReadTaskId;
static pthread_t tMediaWriteTaskId;
static pthread_t tCmdReadTaskId;
#ifndef RDK_COMPILE
static void prepare_working_demuxes();
static void *cimodule_media_read_task(void *args);
static void *cimodule_media_write_task(void *args);
static void *cimodule_cmd_read_task(void *args);
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
        pthread_mutex_init(&gs_tMediaWriteCondMut, NULL);
        pthread_mutex_init(&gs_tMediaReadCondMut, NULL);
        mutex_init = TRUE;
    }
}

static int ci_media_read_open()
{
    int fd = -1;
    const char *read_node = "/dev/cimodule_media0";

    if (0 == access(read_node, F_OK))
    {
        fd = cimodule_media_intf_open(read_node, O_RDONLY);
        if (fd < 0)
            DMX_USB_DBG("open %s failed", read_node);
    }
    return fd;
}

static int ci_media_read_close(int fd)
{
    int ret;
    if (fd < 0)
    {
        DMX_USB_DBG("media interface close failed, fd = %d", fd);
        return -1;
    }

    ret = cimodule_media_intf_close(fd);
    DMX_USB_DBG("media interface close, fd = %d, ret = %d", fd, ret);

    return 0;
}

static int ci_media_write_open()
{
    int fd = -1;
    const char *write_node = "/dev/cimodule_media0";

    if (0 == access(write_node, F_OK))
    {
        fd = cimodule_media_intf_open(write_node, O_RDWR);
        if (fd < 0)
            DMX_USB_DBG("open %s failed", write_node);
    }
    return fd;
}

static int ci_media_write_close(int fd)
{
    int ret;
    if (fd < 0)
    {
        DMX_USB_DBG("media interface close failed, fd = %d", fd);
        return -1;
    }

    ret = cimodule_media_intf_close(fd);
    DMX_USB_DBG("media interface close, fd = %d", fd, ret);

    return 0;
}

static void reset_resource()
{
    DMX_USB_DBG("enter");
    if (cmd_rw_fd > 0)
    {
        DMX_USB_DBG("close cmd_rw_fd");
        cimodule_cmd_intf_close(cmd_rw_fd);
        cmd_rw_fd = -1;
    }
    if (g_pCmdReadBuf)
    {
        DMX_USB_DBG("close cmdReadBuf");
        cimodule_cmd_intf_munmap_readbuf(g_pCmdReadBuf);
        g_pCmdReadBuf = NULL;
    }
    if (g_pCmdWriteBuf)
    {
        DMX_USB_DBG("close cmdWriteBuf");
        cimodule_cmd_intf_munmap_writebuf(g_pCmdWriteBuf);
        g_pCmdWriteBuf = NULL;
    }
    if (g_pbMediaReadBuf)
    {
        DMX_USB_DBG("close g_pbMediaReadBuf");
        cimodule_media_intf_munmap_readbuf(g_pbMediaReadBuf);
        g_pbMediaReadBuf = NULL;
    }
    if (g_pbMediaWriteBuf)
    {
        DMX_USB_DBG("close g_pbMediaWriteBuf");
        cimodule_media_intf_munmap_writebuf(g_pbMediaWriteBuf);
        g_pbMediaWriteBuf = NULL;
    }
    if (media_read_fd > 0)
    {
        DMX_USB_DBG("close media_read_fd");
        ci_media_read_close(media_read_fd);
        media_read_fd = -1;
    }
    if (media_write_fd > 0)
    {
        DMX_USB_DBG("close media_write_fd");
        ci_media_write_close(media_write_fd);
        media_write_fd = -1;
    }
    if (rec_dmx_fd > 0)
    {
        DMX_USB_DBG("close rec_dmx_fd");
        ioctl(rec_dmx_fd, DMX_STOP, 0);
        close(rec_dmx_fd);
        rec_dmx_fd = -1;
    }
    if (rec_dvr_fd > 0)
    {
        DMX_USB_DBG("close rec_dvr_fd");
        close(rec_dvr_fd);
        rec_dvr_fd = -1;
    }
    if (inj_dvr_fd > 0)
    {
        DMX_USB_DBG("close inj_dvr_fd");
        ioctl(inj_dvr_fd, DMX_SET_INPUT, INPUT_DEMOD);
        close(inj_dvr_fd);
        inj_dvr_fd = -1;
    }
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

    fcntl(rec_dmx_fd, F_SETFL, O_NONBLOCK);
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

static void *cimodule_media_read_task(void *args)
{
    int ret, read_len, inj_len, usbdata_len = 0;
    int save_fd = -1;
    struct usb_cimodule_info tUsbCiModuleInfo = {0};
    unsigned char bMediaOutputCtrl = 0;
    char *usbdata_buf;

    usbdata_buf = STB_MEMGetSysRAM(USB_CIMODULE_MEDIA_MAX_SIZE * 100);
    if (!usbdata_buf)
    {
        DMX_USB_DBG("no mem to alloc usbdata_buf");
        return NULL;
    }

    DMX_USB_DBG("entry");

    ret = cimodule_get_usb_cimodule_info(media_read_fd, &tUsbCiModuleInfo);
    if (ret < 0)
    {
        DMX_USB_DBG("(handle: %d),get device info error,error code:%d", media_read_fd, ret);
        return NULL;
    }

    if (!tUsbCiModuleInfo.m_bIsCI20Deteced)
    {
        // refer to CI_OVER_USB_1.0 SPEC
        bMediaOutputCtrl = (unsigned char)((tUsbCiModuleInfo.m_dwCiCompatibility >> 7) & 0x01);
        if (MEDIA_OUTPUT_ENABLE != bMediaOutputCtrl)
        {
            DMX_USB_DBG("can not read media from usb ci module in this mode");
            return NULL;
        }
    }

    while (thread_running)
    {
        ret = cimodule_media_intf_read(media_read_fd, g_pbMediaReadBuf, USB_CIMODULE_MEDIA_MAX_SIZE, &read_len, -1);
        if (ret == USBCAM_UNPLUG)
        {
            goto EXIT;
        }
        // DMX_USB_DBG("read ret %d", ret);
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
                    // available data + short packet, so if short packet comes first, drop it.
                    if (usbdata_len == 0)
                        continue;
                }
                memcpy(usbdata_buf + usbdata_len, g_pbMediaReadBuf, read_len);
                usbdata_len += read_len;
                if (usbdata_len >= USB_CIMODULE_MEDIA_MAX_SIZE)
                {
                    inj_len = inject_usbcam_source_demux(usbdata_buf, usbdata_len);
                    usbdata_len -= inj_len;
                    if (usbdata_len > 0)
                        memmove(usbdata_buf, usbdata_buf + inj_len, usbdata_len);
                }
                // DMX_USB_DBG("read %d, inject %d", read_len, inj_len);
#ifdef DMX_USB_TEST
                if (save_fd < 0)
                    save_fd = open("/data/w.ts", O_RDWR);
                write(save_fd, g_pbMediaReadBuf, read_len);
#endif
            }
        }
        else
        {
            // DMX_USB_DBG("read len %d ret %d", read_len, ret);
            // sleep(1);
        }
    }

EXIT:
    DMX_USB_DBG("usbcam unplug, media read task exit.");
    module_inserted = FALSE;
    if (usbdata_buf)
        free(usbdata_buf);
    return NULL;
}

static void *cimodule_media_write_task(void *args)
{
    int ret;
    int fdMedia = -1;
    struct usb_cimodule_info tUsbCiModuleInfo;
    unsigned int dwCiCompatibility;
    unsigned char bMediaInputCtrl;
    unsigned char *buffer;
    int write_len = 0;
    int rec_len = 0;
    int threshold = 0;
    BOOLEAN first_run = TRUE;
    unsigned char arDummyTsHdr[10] = {0x00, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    buffer = STB_MEMGetSysRAM(REC_BUFF_SIZE);

    DMX_USB_DBG("open usb cimodlue media interface succeessfully,write handle:%d", media_write_fd);

    ret = cimodule_get_usb_cimodule_info(media_write_fd, &tUsbCiModuleInfo);
    if (ret < 0)
    {
        DMX_USB_DBG("(handle: %d),get device info error,error code:%d", media_write_fd, ret);
        return NULL;
    }
    DMX_USB_DBG("get cimodule info ok");

    if (!tUsbCiModuleInfo.m_bIsCI20Deteced)
    {
        // refer to CI_OVER_USB_1.0 SPEC
        bMediaInputCtrl = (unsigned char)((tUsbCiModuleInfo.m_dwCiCompatibility >> 6) & 0x01);
        if (MEDIA_INPUT_ENABLE != bMediaInputCtrl)
        {
            DMX_USB_DBG("can not write media to usb ci module in this mode");
            return NULL;
        }
    }

    DMX_USB_DBG("ci20 detected ok. ready to inject ts, buf %p", g_pbMediaWriteBuf);

    while (thread_running)
    {
#ifndef INJECT_FROM_FILE
        rec_len = record_from_tsin(buffer + threshold, USB_CIMODULE_MEDIA_MAX_SIZE*10);
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
                DMX_USB_DBG("first run ,%x buf %d rec %d", buffer[0], threshold, rec_len);
#endif
                continue;
            }
        }

        while ((threshold > USB_CIMODULE_MEDIA_MAX_SIZE) && thread_running)
        {
            memcpy(g_pbMediaWriteBuf, arDummyTsHdr, 10);
            if (first_run)
            {
                first_run = FALSE;
                g_pbMediaWriteBuf[3] |= (1<<7);
            }
            ret = cimodule_media_intf_write(media_write_fd, g_pbMediaWriteBuf, 10, &write_len, -1);
#ifdef DEMUX_USB_MODULE_DEBUG
            DMX_USB_DBG("write dummy len %d ret %d", write_len, ret);
#endif
            memcpy(g_pbMediaWriteBuf, buffer, USB_CIMODULE_MEDIA_MAX_SIZE);
            if (ret == 0)
            {
                ret = cimodule_media_intf_write(media_write_fd, g_pbMediaWriteBuf, USB_CIMODULE_MEDIA_MAX_SIZE, &write_len, -1);
                if (ret != 0)
                {
                    DMX_USB_DBG("write usb cam failed!!!! %d", ret);
                    goto EXIT;
                }
#ifdef DEMUX_USB_MODULE_DEBUG
                DMX_USB_DBG("write ts len %d ret %d", write_len, ret);
#endif
                if (threshold - write_len > 0)
                {
                    memmove(buffer, buffer + write_len, threshold - write_len);
                }
                if (threshold - write_len < 0)
                    DMX_USB_DBG("write data error occur");
                threshold -= write_len;
            }
            else if (ret == USBCAM_UNPLUG)
            {
                goto EXIT;
            }
        }
    }
EXIT:
    DMX_USB_DBG("usbcam unplug, media write task exit.");
    if (buffer > 0)
        free(buffer);
    module_inserted = FALSE;
    return NULL;
}

static void *cimodule_cmd_read_task(void *args)
{
    unsigned int len = 0;
    DMX_USB_DBG("command thread begin running");
    int ret;
#ifdef DEMUX_USB_MODULE_DEBUG
    int i;
    char buffer[512];
#endif

    while (thread_running)
    {
        ret = cimodule_cmd_intf_read(cmd_rw_fd, g_pCmdReadBuf, USB_CIMODULE_COMMAND_MAX_SIZE, &len, -1);
#ifdef DEMUX_USB_MODULE_DEBUG
        for (i=0;i<16;i++)
            sprintf(buffer+3*i, "%2x ", g_pCmdReadBuf[i]);
        DMX_USB_DBG("====> %s", buffer);
#endif

        if (ret != 0)
        {
            // if (ret == USBCAM_UNPLUG)
            if (ret < 0)
            {
                DMX_USB_DBG("cmd_read: usbcam unplug detect, exit");
                module_inserted = FALSE;
                break;
            }
        }
        else
        {
            pthread_mutex_lock(&cmd_read_mutex);
            DataBlock *db = (DataBlock *)STB_MEMGetSysRAM(sizeof(DataBlock));
            db->next = NULL;
            memcpy(db->data, g_pCmdReadBuf, len);
            db->left = len;
            db->start = 0;
            if (data_block_head == NULL)
                data_block_head = db;
            else
            {
                DataBlock *t = data_block_head;
                while (t->next != NULL)
                    t = t->next;
                t->next = db;
            }
            pthread_mutex_unlock(&cmd_read_mutex);
        }
    }
    DMX_USB_DBG("usbcam unplug, cmd read task exit.");
    return NULL;
}

static void prepare_working_demuxes()
{
    int ret;
    char inj_dvr_path[64];
    char rec_dvr_path[64];

    ev_fd = eventfd(0, 0);
    snprintf(inj_dvr_path, sizeof(inj_dvr_path), "/dev/dvb0.dvr%d", inj_dev_id);
    if (inj_dvr_fd < 0)
        inj_dvr_fd = open(inj_dvr_path, O_WRONLY);
    ioctl(inj_dvr_fd, DMX_SET_INPUT, INPUT_LOCAL);
    snprintf(rec_dvr_path, sizeof(rec_dvr_path), "/dev/dvb0.dvr%d", rec_dev_id);
    if (rec_dvr_fd < 0)
        rec_dvr_fd = open(rec_dvr_path, O_RDONLY);

    fcntl(rec_dvr_fd, F_SETFL, fcntl(rec_dvr_fd, F_GETFL, 0) | O_NONBLOCK, 0);
    ioctl(rec_dvr_fd, DMX_SET_BUFFER_SIZE, REC_BUFF_SIZE);

    set_usbcam_recording_demux(aml_hw_cfg.tuners[aml_hw_cfg.tuner_num - 1].ts_input_idx);
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
    int ret = 0;
    unsigned int dwDriverVersion = 0;
    struct usb_cimodule_info tUsbCiModuleInfo;
    const char *pbFileName = "/dev/cimodule_command0";

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

    if (0 == access(pbFileName, F_OK))
    {
        cmd_rw_fd = cimodule_cmd_intf_open(pbFileName, O_RDWR);

        if (cmd_rw_fd < 0)
        {
            DMX_USB_DBG("cimodule cmd interface open failed");
            goto ERR;
        }
    }
    else
    {
        DMX_USB_DBG("access %s failed", pbFileName);
        goto ERR;
    }
    cimodule_get_driver_version(cmd_rw_fd, &dwDriverVersion);
    DMX_USB_DBG("usbcimodule driver version: %d.%d.%d.%d",
                (dwDriverVersion & 0xFF000000) >> 24,
                (dwDriverVersion & 0x00FF0000) >> 16,
                (dwDriverVersion & 0x0000FF00) >> 8,
                dwDriverVersion & 0x000000FF);

    cimodule_get_usb_cimodule_info(cmd_rw_fd, &tUsbCiModuleInfo);

    DMX_USB_DBG("Show the usb cimodule Info: ");
    DMX_USB_DBG("Vendor Id: 0x%x", tUsbCiModuleInfo.m_wVendorId);
    DMX_USB_DBG("Product Id: 0x%x", tUsbCiModuleInfo.m_wProductId);

    if (media_read_fd < 0)
        media_read_fd = ci_media_read_open();
    if (media_write_fd < 0)
        media_write_fd = ci_media_write_open();
    if ((media_read_fd < 0) || (media_write_fd < 0))
    {
        DMX_USB_DBG("Failed to open media device, read fd: %d write: %d", media_read_fd, media_write_fd);
        goto ERR;
    }

    g_pCmdWriteBuf = cimodule_cmd_intf_mmap_writebuf(cmd_rw_fd, USB_CIMODULE_COMMAND_MAX_SIZE);
    g_pCmdReadBuf = cimodule_cmd_intf_mmap_readbuf(cmd_rw_fd, USB_CIMODULE_COMMAND_MAX_SIZE);
    g_pbMediaReadBuf = cimodule_media_intf_mmap_readbuf(media_read_fd, USB_CIMODULE_MEDIA_MAX_SIZE);
    g_pbMediaWriteBuf = cimodule_media_intf_mmap_writebuf(media_write_fd, USB_CIMODULE_MEDIA_MAX_SIZE);

    prepare_working_demuxes();
    module_init = TRUE;
    module_inserted = TRUE;
    thread_running = TRUE;

    pthread_mutex_unlock(&resource_mutex);

    ret = pthread_create(&tMediaReadTaskId, NULL, (void *)cimodule_media_read_task, NULL);
    if (ret != 0)
        goto ERR;
    ret = pthread_create(&tMediaWriteTaskId, NULL, (void *)cimodule_media_write_task, NULL);
    if (ret != 0)
        goto ERR;
    ret = pthread_create(&tCmdReadTaskId, NULL, (void *)cimodule_cmd_read_task, NULL);
    if (ret != 0)
        goto ERR;


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

    if (pthread_join(tMediaReadTaskId, &status) != 0)
    {
        DMX_USB_DBG("media read task join failed");
    }
    if (pthread_join(tMediaWriteTaskId, &status) != 0)
    {
        DMX_USB_DBG("media write task join failed");
    }
    if (pthread_join(tCmdReadTaskId, &status) != 0)
    {
        DMX_USB_DBG("cmd read task join failed");
    }

    reset_resource();
    module_init = FALSE;

    pthread_mutex_unlock(&resource_mutex);
    return 0;
}

S32BIT STB_CIUsbWrite(U8BIT *buffer, U32BIT len)
{
    int ret;
    unsigned int dwActualSendLen = 0;
#ifdef DEMUX_USB_MODULE_DEBUG
    char buf[2048];
#endif
    unsigned int i;

    if (len > USB_CIMODULE_COMMAND_MAX_SIZE)
    {
        DMX_USB_DBG("write data is longer than buffer size, failed");
        return -1;
    }
    memcpy(g_pCmdWriteBuf, buffer, len);
    if (cmd_rw_fd > 0)
        ret = cimodule_cmd_intf_write(cmd_rw_fd, g_pCmdWriteBuf, len, &dwActualSendLen, -1);
    else
    {
        DMX_USB_DBG("cmd fd is closed, exit.");
        return -1;
    }
    // DMX_USB_DBG("todo buffer len %d, write len %d, must equal", len, dwActualSendLen);
#ifdef DEMUX_USB_MODULE_DEBUG
    if (dwActualSendLen < 256)
    {
        for (i = 0; i < dwActualSendLen; i++)
            sprintf(buf + 3 * i, "%02x ", g_pCmdWriteBuf[i]);
        DMX_USB_DBG("Write %d =========> %s", dwActualSendLen, buf);
    }
#endif
    return dwActualSendLen;
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
    unsigned int dwActualRecvLen = 0;
#ifdef DEMUX_USB_MODULE_DEBUG
    char buf[2048];
#endif
    int i;
    int read_len = 0;

    if (module_inserted == FALSE)
    {
        DMX_USB_DBG("module closed, exit.");
        return -1;
    }

    pthread_mutex_lock(&cmd_read_mutex);
    if (data_block_head)
    {
        read_len = data_block_head->left > len ? len : data_block_head->left;
        memcpy(buffer, data_block_head->data + data_block_head->start, read_len);
        data_block_head->start += read_len;
        data_block_head->left -= read_len;
        if (data_block_head->left == 0)
        {
            DataBlock *head = data_block_head->next;
            STB_MEMFreeSysRAM(data_block_head);
            data_block_head = head;
        }
    }
    pthread_mutex_unlock(&cmd_read_mutex);

#ifdef DEMUX_USB_MODULE_DEBUG
    if (read_len > 0 && read_len <= 256)
    {
        for (i = 0; i < read_len; i++)
            sprintf(buf + 3 * i, "%02x ", buffer[i]);
        DMX_USB_DBG("Read %d =========> %s", read_len, buf);
    }
#endif
    return read_len;
}

U8BIT STB_CIUsbCamTotal(void)
{
    return 1;
}
#endif
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
