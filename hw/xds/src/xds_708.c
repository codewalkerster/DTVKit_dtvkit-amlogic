#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "xds_608.h"
#include "xds_708.h"

#define xds_inf(_fmt, ...) printf("I 708: "_fmt"\n", ##__VA_ARGS__)
#define xds_err(_fmt, ...) printf("E 708: "_fmt"\n", ##__VA_ARGS__)

typedef struct AM_CCData_s AM_CCData;
struct AM_CCData_s {
    AM_CCData *next;
    uint8_t *buf;
    uint32_t pts;
    uint32_t duration;
    int pts_valid;
    int size;
    int cap;
    int poc;
};

typedef struct {
    int magic;
    #define XDS_708_MAGIC ((int)('x'<<24|'7'<<16|'0'<<8|'8'))

    xds_start_param_t param;

    void *xds608;

    /*sequencing*/
    AM_CCData *cc_list;
    AM_CCData *free_list;
    int cc_num;
    int curr_poc;
    uint32_t curr_pts;
    uint32_t curr_duration;
} xds_708_t;

static void dump(char *title, uint8_t *pd, size_t size)
{
    int i;
    for (i = 0; i < (size/16); pd+=16, i++) {
        xds_inf("%s: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                title,
                pd[0], pd[1], pd[2], pd[3], pd[4], pd[5], pd[6], pd[7],
                pd[8], pd[9], pd[10], pd[11], pd[12], pd[13], pd[14], pd[15]);
    }
}

void* xds_708_create(xds_start_param_t *param)
{
    xds_708_t *x708 = (xds_708_t *)calloc(1, sizeof(xds_708_t));
    if (!x708)
        return NULL;

    x708->magic = XDS_708_MAGIC;
    x708->param = *param;

    x708->xds608 = xds_608_create(&x708->param);

    x708->curr_poc = -1;

    return x708;
}

static void aml_free_cc_data (AM_CCData *cc)
{
    if (cc->buf)
        free(cc->buf);
    free(cc);
}

void xds_708_destroy(void *xds708)
{
    xds_708_t *x708 = (xds_708_t *)xds708;

    if (!x708 || x708->magic != XDS_708_MAGIC)
        return;

    if (x708->xds608)
        xds_608_destroy(x708->xds608);

    AM_CCData *cc, *cc_next;
    for (cc = x708->cc_list; cc; cc = cc_next) {
        cc_next = cc->next;
        aml_free_cc_data(cc);
    }
    x708->magic = 0;
    free(x708);
}

void xds_708_decode(void *xds708, const uint8_t *buf, size_t size)
{
    xds_708_t *x708 = (xds_708_t *)xds708;

    if (!x708 || x708->magic != XDS_708_MAGIC)
        return;

    int process_cc = buf[0] & 0x40;
    if (!process_cc)
        return;

    int cc_count = buf[0] & 0x1F;
    int max = (size - 2) / 3;
    if (cc_count > max)
        cc_count = max;

    uint8_t *pd = (uint8_t *)buf + 2;
    int i;
    for (i = 0; i < cc_count * 3; i += 3) {
        int cc_valid = pd[i] & 4;
        int cc_type = pd[i] & 3;

        if (!cc_valid)
            continue;

        if (cc_type == 1) {
            /*608 field 2*/
            xds_608_decode(x708->xds608, 284, pd[i + 1], pd[i + 2]);
        }
    }
}

void xds_708_decode_userdata(void *xds708, const uint8_t *buf, size_t size)
{
    xds_708_t *x708 = (xds_708_t *)xds708;

    if (!x708 || x708->magic != XDS_708_MAGIC)
        return;

    uint8_t *pd = (uint8_t *)buf;

#if 0//no pts
    /*pts*/
    pd += 4;
    size -= 4;
#endif

    if (size > 8
            && pd[0] == 0x47
            && pd[1] == 0x41
            && pd[2] == 0x39
            && pd[3] == 0x34) {
        if (pd[4] != 0x3)
            return;
        xds_708_decode(x708, pd + 5, size - 5);

    } else if (size > 4
            && pd[0] == 0xb5
            && pd[1] == 0
            && pd[2] == 0x2F) {
        /*directv*/
        if (pd[3] != 0x3)
            return;
        xds_708_decode(x708, pd + 5, size - 5);
    }
}

static void swap_data(uint8_t *user_data, int ud_size)
{
    int swap_blocks, i, j, k, m;
    unsigned char c_temp;

    /* swap byte order */
    swap_blocks = ud_size >> 3;
    for (i = 0; i < swap_blocks; i ++) {
        j = i << 3;
        k = j + 7;
        for (m=0; m<4; m++) {
            c_temp = user_data[j];
            user_data[j++] = user_data[k];
            user_data[k--] = c_temp;
        }
    }
}

#define CC_TYPE_INVALID 0
#define CC_TYPE_MPEG    1

typedef struct {
    uint32_t picture_structure:16;
    uint32_t temporal_reference:10;
    uint32_t picture_coding_type:3;
    uint32_t reserved:3;
    uint32_t index:16;
    uint32_t offset:16;
    uint8_t  atsc_flag[4];
    uint8_t  cc_data_start[4];
} aml_ud_header_t;

#define IS_ATSC(p)  ((p[0] == 0x47) && (p[1] == 0x41) && (p[2] == 0x39) && (p[3] == 0x34)/* && (p[4] == 0x3)*/)

static int check_type(uint8_t *data, int size, int vfmt)
{
    /*vfmt: 0-mpeg, 2-h264, 7-avs*/
    switch (vfmt) {
        case 0:
            if (size >= (int)sizeof(aml_ud_header_t)) {
                aml_ud_header_t *h = (aml_ud_header_t *)data;
                if (IS_ATSC(h->atsc_flag)) {
                    xds_inf("CC_TYPE_MPEG");
                    return CC_TYPE_MPEG;
                }
            }
            break;
        default:
            break;
    }
    return 0;
}

static void aml_write_userdata(xds_708_t *x708, uint8_t *buffer, int buffer_len,
        uint32_t pts, int pts_valid, uint32_t duration)
{
    if (pts_valid == 0)
        pts = x708->curr_pts + x708->curr_duration;

    xds_708_decode_userdata(x708, buffer, buffer_len);
    xds_inf("decode 708: pts:%x, size:%d", pts, buffer_len);

    x708->curr_pts = pts;
    x708->curr_duration = duration;
}

static void aml_flush_cc_data(xds_708_t *x708)
{
    AM_CCData *cc, *ncc;

    for (cc = x708->cc_list; cc; cc = ncc) {
        ncc = cc->next;

        aml_write_userdata(x708, cc->buf, cc->size, cc->pts, cc->pts_valid, cc->duration);

        cc->next = x708->free_list;
        x708->free_list = cc;
    }

    x708->cc_list  = NULL;
    x708->cc_num   = 0;
    x708->curr_poc = -1;
}

#define MAX_CC_NUM 64

static void aml_add_cc_data(xds_708_t *x708, int poc, int type,
        uint8_t *p, int len,
        uint32_t pts, int pts_valid, uint32_t duration)
{
    AM_CCData **pcc, *cc;

    if (x708->cc_num >= MAX_CC_NUM) {
        cc = x708->cc_list;
        aml_write_userdata(x708, cc->buf, cc->size, cc->pts, cc->pts_valid, cc->duration);

        x708->cc_list = cc->next;
        cc->next = x708->free_list;
        x708->free_list = cc;
        x708->cc_num --;
    }

    pcc = &x708->cc_list;
    if (*pcc && poc < ((*pcc)->poc - 30))
        aml_flush_cc_data(x708);

    while ((cc = *pcc)) {
        if (cc->pts == pts)
        {
            if (cc->poc > poc)
                break;
        }
        else if (cc->pts > pts) {
            break;
        }

        pcc = &cc->next;
    }

    if (x708->free_list) {
        cc = x708->free_list;
        x708->free_list = cc->next;
    } else {
        cc = malloc(sizeof(AM_CCData));
        cc->buf  = NULL;
        cc->size = 0;
        cc->cap  = 0;
        cc->poc  = 0;
    }

    if (cc->cap < len) {
        cc->buf = realloc(cc->buf, len);
        cc->cap = len;
    }

    memcpy(cc->buf, p, len);

    cc->size = len;
    cc->poc  = poc;
    cc->pts = pts;
    cc->pts_valid = pts_valid;
    cc->duration = duration;
    cc->next = *pcc;
    *pcc = cc;

    x708->cc_num ++;
}

#define PIC_I 1
#define PIC_P 2
#define PIC_B 3
#define PIC_D 4

static void aml_mpeg_userdata_package(xds_708_t *x708, int poc, int type,
        uint8_t *p, int len,
        uint32_t pts, int pts_valid, uint32_t duration)
{
    if (len < 5)
        return;

    if (p[4] != 3)
        return;

    if (type == PIC_I)
        aml_flush_cc_data(x708);

    if (poc == x708->curr_poc + 1) {
        AM_CCData *cc, **pcc;

        aml_write_userdata(x708, p, len, pts, pts_valid, duration);
        x708->curr_poc ++;

        pcc = &x708->cc_list;
        while ((cc = *pcc)) {
            if (x708->curr_poc + 1 != cc->poc)
                break;

            aml_write_userdata(x708, cc->buf, cc->size, cc->pts, cc->pts_valid, cc->duration);
            *pcc = cc->next;
            x708->curr_poc ++;

            cc->next = x708->free_list;
            x708->free_list = cc;
        }

        return;
    }

    aml_add_cc_data(x708, poc, type, p, len, pts, pts_valid, duration);
}


static int sequence_mpeg_userdata(xds_708_t *x708, uint8_t *data, int size, struct userdata_meta_info_t *meta)
{
    uint8_t *pd = data;
    int left = size;
    int r = 0;

    xds_inf("poc:%d, flags:%x, vpts:%x, pts_valid:%d",
            meta->poc_number, meta->flags, meta->vpts, meta->vpts_valid);

    while (left >= (int)sizeof(aml_ud_header_t)) {
        aml_ud_header_t *hdr = (aml_ud_header_t*)pd;

        dump("udp", pd, 32);

        if (IS_ATSC(hdr->atsc_flag)) {
            uint32_t v = (pd[4] << 24) | (pd[5] << 16) | (pd[6] << 8) | pd[7];
            int ref   = (v >> 16) & 0x3ff;
            int ptype = (v >> 26) & 7;
            /* We read one packet in one time, so treat entire buffer */
            xds_inf("ref:%d p:%d", ref, ptype);
            if (!(meta->flags & (1 << 2)))
                return size;
            aml_mpeg_userdata_package(x708, ref, ptype, hdr->atsc_flag, size - r - 8, meta->vpts,
                meta->vpts_valid, meta->duration);
            return size;
            break;

        } else {
            pd   += 8;
            left -= 8;
            r   += 8;
        }
    }

    return r;
}


void xds_708_sequence_userdata(void *xds708, ds_data_userdata_t *ud)
{
    xds_708_t *x708 = (xds_708_t *)xds708;

    if (!x708 || x708->magic != XDS_708_MAGIC)
        return;

    uint8_t *pd = ud->data;
    int left = ud->data_size;

    xds_inf("seq ud: %zd+%d", sizeof(ds_data_userdata_t), ud->data_size);

    swap_data(ud->data, ud->data_size);

    int type = CC_TYPE_INVALID;
    while (type == CC_TYPE_INVALID) {
        if (left < 8)
            break;

        type = check_type(pd, left, x708->param.vfmt);
        if (type != CC_TYPE_INVALID)
            break;

        pd += 8;
        left -= 8;
    }

    if (type == CC_TYPE_MPEG)
        sequence_mpeg_userdata(x708, pd, left, &ud->meta_info);
}


