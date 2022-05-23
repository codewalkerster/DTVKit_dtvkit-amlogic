#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <poll.h>

#include "stbhwdmx.h"

#include "dmx.h"
#include "emu_internal.h"


#define AMSTREAM_IOC_MAGIC  'S'
#define AMSTREAM_IOC_PORT_INIT   _IO(AMSTREAM_IOC_MAGIC, 0x11)

static int dmx_driver_ver;


int EmuFileEcho(const char *name, const char *cmd)
{
  int fd, len, ret;
  if (name == NULL || cmd == NULL) {
    return -1;
  }

  fd = open(name, O_WRONLY);
  if (fd == -1)
  {
    EMU_DBG("cannot open file \"%s\"", name);
    return -1;
  }

  len = strlen(cmd);
  ret = write(fd, cmd, len);
  if (ret != len)
  {
    EMU_DBG("write failed file:\"%s\" cmd:\"%s\" error:\"%s\"", name, cmd, strerror(errno));
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}


int EmuFileRead(const char *name, char *buf, int len)
{
  FILE *fp;
  char *ret;

  if (name == NULL || buf == NULL) {
    EMU_DBG("read error param is NULL");
    return -1;
  }

  fp = fopen(name, "r");
  if (!fp)
  {
    EMU_DBG("cannot open file \"%s\"", name);
    return -1;
  }

  ret = fgets(buf, len, fp);
  if (!ret)
  {
    EMU_DBG("read the file:\"%s\" error:\"%s\" failed", name, strerror(errno));
  }

  fclose(fp);
  return ret ? 0 : -1;
}


static int X2DmxGetInput(int dmx_no)
{
    char buf[64];
    char node[64];

    memset(buf, 0, sizeof(buf));
    EmuFileRead("/sys/class/stb/source", buf, sizeof(buf));
    if(memcmp(buf, "hiu", 3))
    {
        return DMX_TUNER;
    }

    snprintf(node, sizeof(node), "/sys/class/stb/demux%d_source", dmx_no);
    memset(buf, 0, sizeof(buf));
    EmuFileRead(node, buf, sizeof(buf));
    if(memcmp(buf, "hiu", 3))
    {
        return DMX_TUNER;
    }

    return DMX_MEMORY;
}

static int X2DmxSetInput(int dmx_no, int input)
{
    char node[64];

    EmuFileEcho("/sys/class/stb/source", "hiu");

    snprintf(node, sizeof(node), "/sys/class/stb/demux%d_source", dmx_no);
    return EmuFileEcho(node, "hiu");
}

static int X2DmxOpen(int dmx_no, int search)
{
    int fd = 0;
    int ret;

    fd = open("/dev/amstream_mpts_sched", O_WRONLY);
    if (fd == -1)
    {
        EMU_DBG("cannot open amstream_mpts_sched (%s)", strerror(errno));
        return -1;
    }

    if (!search)
    {
        ioctl(fd, AMSTREAM_IOC_PORT_INIT, 0xff88);
    }

    EmuFileEcho("/sys/class/stb/source", "hiu");
    return fd;
}

static int X2DmxClose(int handle)
{
    if (handle >= 0)
    {
        close(handle);
    }
    return 0;
}

static int X2DmxInjectData(int handle, unsigned char *buf, int size, int timeout)
{
    return size;
}

static int X4DmxGetInput(int dmx_no)
{
    char buf[256];
    char dmx[32];
    int  input = DMX_TUNER;

    snprintf(dmx, sizeof(dmx), "dmx%d source:", dmx_no);
    FILE* fd = fopen("/sys/class/dmx/dmx_source", "r");
    if (fd == NULL)
    {
        //EMU_DBG("open input failed");
        return input;
    }

    memset(buf, 0, sizeof(buf));
    while (fgets(buf, sizeof(buf), fd) != NULL)
    {
        if (memcmp(buf, dmx, strlen(dmx)) == 0)
        {
            char *pbuf = &buf[strlen(dmx)];
            if (memcmp(pbuf, "input_local", strlen("input_local")) == 0)
            {
                input = DMX_MEMORY;
            }
            else if (memcmp(pbuf, "input_local_sec", strlen("input_local_sec")) == 0)
            {
                input = DMX_MEMORY;
            }

            break;
        }
    }

    fclose(fd);
    return input;
}


static int X4DmxSetInput(int dmx_no, int input)
{
    char dev_name[32];
    int fd;
    int ret;

    snprintf(dev_name, sizeof(dev_name), "/dev/dvb0.demux%d", dmx_no);
    fd = open(dev_name, O_RDWR);
    if(fd == -1)
    {
        EMU_DBG("cannot open for set input \"%s\" (%s)", dev_name, strerror(errno));
        return -1;
    }

    if (input == DMX_MEMORY)
    {
        ret = ioctl(fd, DMX_SET_INPUT, INPUT_LOCAL);
        //tsplay set input,but not set hws, we set another hws,force update hw source
        ret = ioctl(fd, DMX_SET_HW_SOURCE, DMA_6);
        ret = ioctl(fd, DMX_SET_HW_SOURCE, DMA_0 + dmx_no);
    }

    close(fd);
    return 0;
}


static int X4DmxOpen(int dmx_no)
{
    char dev_name[32];
    int fd;
    int ret;

    snprintf(dev_name, sizeof(dev_name), "/dev/dvb0.dvr%d", dmx_no);
    fd = open(dev_name, O_WRONLY);
    if (fd == -1)
    {
        EMU_DBG("cannot open \"%s\" (%s)", dev_name, strerror(errno));
        return -1;
    }

    ret = ioctl(fd, DMX_SET_INPUT, INPUT_LOCAL);
    ret = ioctl(fd, DMX_SET_BUFFER_SIZE, 5*1024*1024);

    return fd;
}


static int X4DmxClose(int handle)
{
    if (handle >= 0)
    {
        close(handle);
    }
    return 0;
}


static int X4DmxInjectData(int handle, unsigned char *buf, int size, unsigned int timeout)
{
    int ret;
    int left = size;
    uint8_t *p = buf;
    struct timeval begin_tv,now_tv;

    if (handle < 0)
    {
        return 0;
    }

    if (timeout >= 0)
    {
        struct pollfd pfd;

        pfd.fd = handle;
        pfd.events = POLLOUT;

        ret = poll(&pfd, 1, timeout);
        if (ret != 1)
            return 0;
    }

    gettimeofday(&begin_tv, NULL);
    while (left > 0)
    {
        ret = write(handle, p, left);
        if (ret == -1)
        {
            if (errno != EINTR)
            {
                EMU_DBG("Write data failed: %s", strerror(errno));
                break;
            }
            ret = 0;
        }
        else
        {
            //printf("%s write cnt:%d\n",__FUNCTION__,ret);
        }

        gettimeofday(&now_tv, NULL);
        unsigned int diff = DiffTimeval(&begin_tv, &now_tv);
        if (diff > timeout)
        {
            EMU_DBG("Write dmx timeout");
            break;
        }

        left -= ret;
        p += ret;
    }

    return (size - left);
}

static int DMXCheckVersion()
{
    struct stat st;
    int r;

    r = stat("/sys/class/stb/demux0_source", &st);
    if (r == -1)
    {
        return DMX_X4;
    }
    return DMX_X2;
}

int EmuDmxInit()
{
    dmx_driver_ver = DMXCheckVersion();
    EMU_DBG("dmx ver:%d" , dmx_driver_ver);
    return 0;
}

int EmuDmxOpen(int dmx_no, int search)
{
    if (dmx_driver_ver == DMX_X4)
    {
        return X4DmxOpen(dmx_no);
    }
    else if (dmx_driver_ver == DMX_X2)
    {
        return X2DmxOpen(dmx_no, search);
    }

    return 0;
}

int EmuDmxClose(int handle)
{
    if (dmx_driver_ver == DMX_X4)
    {
        return X4DmxClose(handle);
    }
    else if (dmx_driver_ver == DMX_X2)
    {
        return X2DmxClose(handle);
    }

    return 0;
}

int EmuDmxInjectData(int handle, unsigned char *buf, int size, unsigned int timeout)
{
    if (dmx_driver_ver == DMX_X4)
    {
        return X4DmxInjectData(handle, buf, size, timeout);
    }
    else if (dmx_driver_ver == DMX_X2)
    {
        return X4DmxInjectData(handle, buf, size, timeout);
    }

    return 0;
}

int EmuDmxSetInput(int dmx_no, int input)
{
    if (dmx_driver_ver == DMX_X4)
    {
        return X4DmxSetInput(dmx_no, input);
    }
    else if (dmx_driver_ver == DMX_X2)
    {
        return X2DmxSetInput(dmx_no, input);
    }

    return 0;
}


int EmuDmxGetInput(int dmx_no)
{
    if (dmx_driver_ver == DMX_X4)
    {
        return X4DmxGetInput(dmx_no);
    }
    else if (dmx_driver_ver == DMX_X2)
    {
        return X2DmxGetInput(dmx_no);
    }

    return 0;
}

