#ifndef _AFD_DATATYPE_H_
#define _AFD_DATATYPE_H_

struct afd_ctl_create_t {
    int path;
    int instance_id;
};

struct afd_rect_t {
    int a;
    int b;
    int c;
    int d;
};

typedef enum {
    SCALING_NONE,
    SCALING_APP,
    SCALING_MHEG,
    SCALING_HBBTV
} E_APP_SCALING_TYPE;

struct afd_scaling_t {
    unsigned int type;//enum E_APP_SCALING_TYPE
    struct afd_rect_t scaling_rect;
    int resolution_width;
    int resolution_height;
};

struct afd_ctl_scaling_t {
    int path;
    struct afd_scaling_t scaling;
};

struct afd_recv_list_t {
    int list[10];
    unsigned char size;
};

struct afd_ctl_overscan_t {
    struct afd_rect_t uhd_overscan;
    struct afd_rect_t fhd_overscan;
    struct afd_rect_t hd_overscan;
    struct afd_rect_t sd_overscan;
};

struct afd_recv_state_t {
    unsigned int valid;
    int path;
    int instance_id;
    unsigned char enable;
    int aspect;
    int video_aspect;
    unsigned int afd_value;
    struct afd_scaling_t scaling;
    unsigned int screen_width;
    unsigned int screen_height;
    unsigned int video_width;
    unsigned int video_height;
    struct afd_rect_t video_out;
    struct afd_rect_t display_out;
};

#define AFD_IOC_MAGIC 'F'
#define AFD_IOCTl_CREATE_CONTEXT  _IOW(AFD_IOC_MAGIC, 0x00, struct afd_ctl_create_t)
#define AFD_IOCTL_RELEASE_CONTEXT _IO(AFD_IOC_MAGIC, 0x01)
#define AFD_IOCTL_SET_ASPECT      _IO(AFD_IOC_MAGIC, 0x02)
#define AFD_IOCTL_SET_SCALE_TYPE  _IOW(AFD_IOC_MAGIC, 0x03, struct afd_ctl_scaling_t)
#define AFD_IOCTL_GET_PATHS       _IOR(AFD_IOC_MAGIC, 0x04, struct afd_recv_list_t)
#define AFD_IOCTL_SET_OVERSCAN    _IOW(AFD_IOC_MAGIC, 0x05, struct afd_ctl_overscan_t)
#define AFD_IOCTL_GET_STATE       _IOR(AFD_IOC_MAGIC, 0x06, struct afd_recv_state_t)


#endif //#define _AFD_DATATYPE_H_

