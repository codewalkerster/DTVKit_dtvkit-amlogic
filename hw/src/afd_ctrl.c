#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "afd_ctrl.h"
#include "dtv_log.h"
#include "stb_utils.h"

#define TAG  "AFD"
#define AFD_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

static int afd_dev = -1;

static int afd_open_device()
{
    if (afd_dev == -1)
        afd_dev = open(AFD_DEV, O_RDWR | O_NONBLOCK);

    if (afd_dev == -1)
        AFD_DBG("can not open afd device(%s)", strerror(errno));

    return afd_dev;
}

void afd_create_context(int path, int instance_id)
{
    AFD_DBG("enable afd for (%d-%d)", path, instance_id);
#ifdef USE_AFD_DEVICE
    if (afd_open_device() != -1)
    {
        struct afd_ctl_create_t iop;
        iop.path = path;
        iop.instance_id = instance_id;
        if (ioctl(afd_dev, AFD_IOCTl_CREATE_CONTEXT, &iop) != 0)
            AFD_DBG("ioctl failed: %s", strerror(errno));
    }
#else
    char afd_cmd[64];
    snprintf(afd_cmd, sizeof(afd_cmd), "%d %d 1", path, instance_id);
    if (!STB_File_Echo("/sys/class/afd_module/enable", afd_cmd))
        AFD_DBG("enable afd failed");
#endif
}

void afd_release_context(int path)
{
    AFD_DBG("disable afd of path (%d)", path);
#ifdef USE_AFD_DEVICE
    if (afd_dev != -1)
    {
        if (ioctl(afd_dev, AFD_IOCTL_RELEASE_CONTEXT, &path) != 0)
            AFD_DBG("ioctl failed: %s", strerror(errno));
        if (afd_getContextNum() == 0)
        {
            close(afd_dev);
            afd_dev = -1;
        }
    }
    else
        AFD_DBG("No afd context, skip release");
#else
    char afd_cmd[16];
    snprintf(afd_cmd, sizeof(afd_cmd), "%d 0 0", path);
    if (!STB_File_Echo("/sys/class/afd_module/enable", afd_cmd))
       AFD_DBG("disable afd failed");
#endif
}

void afd_setMhegScalling(int path, int x, int y, int w, int h, int res_x, int res_y)
{
    AFD_DBG("enable afd mheg scalling %d - (%d,%d,%d,%d,(%d,%d))",
        path, x, y, w, h, res_x, res_y);
#ifdef USE_AFD_DEVICE
    if (afd_open_device() != -1)
    {
        struct afd_ctl_scaling_t st;
        st.path = path;
        st.scaling.type = SCALING_MHEG;
        st.scaling.scaling_rect.a = x;
        st.scaling.scaling_rect.b = y;
        st.scaling.scaling_rect.c = w;
        st.scaling.scaling_rect.d = h;
        st.scaling.resolution_width = res_x;
        st.scaling.resolution_height = res_y;
        if (ioctl(afd_dev, AFD_IOCTL_SET_SCALE_TYPE, &st) != 0)
            AFD_DBG("ioctl failed: %s", strerror(errno));
    }
#else
    char afd_cmd[64];
    snprintf(afd_cmd, sizeof(afd_cmd), "%d 2 %d %d %d %d %d %d",
        path, x, y, w, h, res_x, res_y);
    if (!STB_File_Echo("/sys/class/afd_module/scaling", afd_cmd))
       AFD_DBG("enable afd mheg scalling failed");

#endif
}

void afd_setAppScalling(int path, int x, int y, int w, int h, int res_x, int res_y)
{
    AFD_DBG("enable afd app scalling %d - (%d,%d,%d,%d,(%d,%d))",
        path, x, y, w, h, res_x, res_y);
#ifdef USE_AFD_DEVICE
    if (afd_open_device() != -1)
    {
        struct afd_ctl_scaling_t st;
        st.path = path;
        st.scaling.type = SCALING_APP;
        st.scaling.scaling_rect.a = x;
        st.scaling.scaling_rect.b = y;
        st.scaling.scaling_rect.c = w;
        st.scaling.scaling_rect.d = h;
        st.scaling.resolution_width = res_x;
        st.scaling.resolution_height = res_y;
        if (ioctl(afd_dev, AFD_IOCTL_SET_SCALE_TYPE, &st) != 0)
            AFD_DBG("ioctl failed: %s", strerror(errno));
    }
#else
    char afd_cmd[64];
    snprintf(afd_cmd, sizeof(afd_cmd), "%d 1 %d %d %d %d %d %d",
        path, x, y, w, h, res_x, res_y);
    if (!STB_File_Echo("/sys/class/afd_module/scaling", afd_cmd))
       AFD_DBG("enable afd app scalling failed");

#endif
}

void afd_disableScalling(int path)
{
    AFD_DBG("disable afd scalling (%d)", path);
#ifdef USE_AFD_DEVICE
    if (afd_open_device() != -1)
    {
        struct afd_ctl_scaling_t st;
        st.path = path;
        st.scaling.type = SCALING_NONE;
        if (ioctl(afd_dev, AFD_IOCTL_SET_SCALE_TYPE, &st) != 0)
            AFD_DBG("ioctl failed: %s", strerror(errno));
    }
#else
    char afd_cmd[16];
    snprintf(afd_cmd, sizeof(afd_cmd), "%d 0", path);
    STB_File_Echo("/sys/class/afd_module/scaling", afd_cmd);

#endif
}

int afd_getContextNum()
{
    int ret = 0;
#ifdef USE_AFD_DEVICE
    struct afd_recv_list_t l;
    if (afd_open_device() != -1)
    {
        if (ioctl(afd_dev, AFD_IOCTL_GET_PATHS, &l) == 0)
        {
            ret = l.size;
            AFD_DBG("AFD context number: %d", ret);
        }
        else
            AFD_DBG("ioctl failed: %s", strerror(errno));
    }
#endif
    return ret;
}
