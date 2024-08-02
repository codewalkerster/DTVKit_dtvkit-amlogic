#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "xds_608.h"

#define xds_dbg(_fmt, ...) printf("D 608: "_fmt"\n", ##__VA_ARGS__)
#define xds_inf(_fmt, ...) printf("I 608: "_fmt"\n", ##__VA_ARGS__)
#define xds_err(_fmt, ...) printf("E 608: "_fmt"\n", ##__VA_ARGS__)

#define MAX_PKTS 4
#define MAX_BYTES (32 + 2 + 2)

typedef struct {
    int class;
    int type;

    int cnt;
    uint8_t buf[MAX_BYTES];
} xds_pkt_t;

typedef struct {
    int magic;
    #define XDS_608_MAGIC ((int)('x'<<24|'6'<<16|'0'<<8|'8'))

    xds_start_param_t param;

    /*pkts interleaved*/
    xds_pkt_t pkts[MAX_PKTS];

    /*current processing pkt*/
    xds_pkt_t *cur_pkt;

    /*xds processing*/
    int in_xds;
} xds_608_t;

static void decode_xds_current(xds_608_t *x608)
{
    #define XDS_TYPE_CONTENT_ADVISORY 5

    switch (x608->cur_pkt->type) {
        case XDS_TYPE_CONTENT_ADVISORY: {
            int r;
            int g;

            if (x608->cur_pkt->cnt < (2 + 2 + 2))
                return;

            uint8_t c1 = x608->cur_pkt->buf[2];
            uint8_t c2 = x608->cur_pkt->buf[3];

            xds_dbg("XDS: [RATING] %02X:%02X", c1, c2);

            if (!(c1 & 0x40) || !(c2 & 0x40))
                return;

            r = c1 & 0x7;
            g = c2 & 0x7;

            xds_rating_t rating;

            rating.dlsv = 0;

            if (c1 & 0x20)
                rating.dlsv |= XDS_RATING_D;
            if (c2 & 0x08)
                rating.dlsv |= XDS_RATING_L;
            if (c2 & 0x10)
                rating.dlsv |= XDS_RATING_S;
            if (c2 & 0x20)
                rating.dlsv |= XDS_RATING_V;

            if (!(c1 & 0x8)) {
                rating.sys = XDS_RATING_SYS_MPAA;
                rating.id = r;
                rating.dlsv = 0;
            } else if (!(c1 & 0x10)) {
                rating.sys = XDS_RATING_SYS_TV_US;
                rating.id = g;
            } else if (!(c2 & 0x8)) {
                if (c1 & 0x20) {
                    rating.sys = XDS_RATING_SYS_TV_CA_FR;
                    rating.id = g;
                    rating.dlsv = 0;

                    if (rating.id > 5)
                        return;
                } else {
                    rating.sys = XDS_RATING_SYS_TV_CA_EN;
                    rating.id = g;
                    rating.dlsv = 0;

                    if (rating.id > 6)
                        return;
                }
            } else {
                return;
            }

            xds_inf("XDS: rating s/r/dlsv[%d/%d/%d]",
                    rating.sys, rating.id, rating.dlsv);

            if (x608->param.events & XDS_EVT_RATING)
                x608->param.callback(XDS_EVT_RATING, &rating, x608->param.callback_priv);
            } break;
        default:
            break;
    }
}

static void decode_xds_channel(xds_608_t *x608)
{
    #define XDS_TYPE_TSID 4

    switch (x608->cur_pkt->type) {
        case XDS_TYPE_TSID:{
            if (x608->cur_pkt->cnt < (4 + 2 + 2))
                return;

            xds_network_t net = {
                .ts_id = (x608->cur_pkt->buf[5] & 0xF) << 12
                         | (x608->cur_pkt->buf[4] & 0xF) << 8
                         | (x608->cur_pkt->buf[3] & 0xF) << 4
                         | (x608->cur_pkt->buf[2] & 0xF),
            };

            xds_inf("XDS: tsid [%d/%#x]", net.ts_id, net.ts_id);

            if (x608->param.events & XDS_EVT_NETWORK)
                x608->param.callback(XDS_EVT_NETWORK, &net, x608->param.callback_priv);
            } break;
        default:
            break;
    }
}

static void decode_xds(xds_608_t *x608, unsigned char chksum)
{
    if (!x608->cur_pkt) {
        xds_err("no pkt to decode");
        return;
    }

    x608->cur_pkt->buf[x608->cur_pkt->cnt++] = 0xF;/*END*/
    x608->cur_pkt->buf[x608->cur_pkt->cnt++] = chksum;

    xds_dbg("XDS: %02x:%02x", 0xF, chksum);

    xds_dbg("XDS: [End] class[%d] type[%d] size:%d",
            x608->cur_pkt->class,
            x608->cur_pkt->type,
            x608->cur_pkt->cnt);

    int sum = 0;
    int i;
    for (i = 0; i < x608->cur_pkt->cnt; i++)
        sum += x608->cur_pkt->buf[i];

    if (sum & 0x7F) {
        xds_err("ignore pkt c/t[%d/%d], checksum fail",
                x608->cur_pkt->class,
                x608->cur_pkt->type);
        return;
    }

    #define X608_CLASS_CURRENT 0
    #define X608_CLASS_CHANNEL 2

    switch (x608->cur_pkt->class) {
        case X608_CLASS_CURRENT:
            decode_xds_current(x608);
            break;
        case X608_CLASS_CHANNEL:
            decode_xds_channel(x608);
            break;
        default:
            break;
    }

    x608->cur_pkt->class = -1;
    x608->cur_pkt->cnt = 0;
    x608->cur_pkt = NULL;
}

static int process_xds(xds_608_t *x608, unsigned char b1, unsigned char b2)
{
    if (b1 < 0xF) {//start or continue
        int is_new = b1 % 2;
        int class = (b1 - 1) / 2;

        int i;
        int match = -1;
        int first_free = -1;
        for (i = 0; i < MAX_PKTS; i++) {

            if (first_free == -1 && x608->pkts[i].class == -1)
                first_free = i;

            if (x608->pkts[i].class == class
                    && x608->pkts[i].type == b2)
                match = i;
        }

        xds_dbg("XDS: [Start] %02x:%02x new[%d] class[%d] type[%d], match[%d] firstfree[%d]",
                b1, b2, is_new, class, b2, match, first_free);

        if (is_new) {//start

            if (match != -1) {
                xds_err("new start c/t[%d/%d], but old found, drop and restart",
                        class, b2);

                x608->cur_pkt = &x608->pkts[match];
                x608->cur_pkt->cnt = 0;

            } else if (first_free == -1) {
                xds_err("pkts overflow, data dropped");
                return -1;

            } else {
                x608->cur_pkt = &x608->pkts[first_free];
                x608->cur_pkt->cnt = 0;

                x608->cur_pkt->class = class;
                x608->cur_pkt->type = b2;
            }

        } else {//continue

            if (match != -1)
                x608->cur_pkt = &x608->pkts[match];

            if (!x608->cur_pkt) {
                xds_err("continue c/t[%d/%d], but pkt not found, data dropped",
                        class, b2);
                return -1;
            }
        }

    } else {

        /*xds data*/
        if (!x608->cur_pkt) {
            xds_err("xds data, but pkt not found, data dropped");
            return -1;
        }
    }

    xds_dbg("XDS: %02x:%02x", b1, b2);

    if (x608->cur_pkt->cnt <= (MAX_BYTES - 2)) {
        x608->cur_pkt->buf[x608->cur_pkt->cnt++] = b1;
        x608->cur_pkt->buf[x608->cur_pkt->cnt++] = b2;
    }

    return 0;
}


void xds_608_decode(void *xds608, int line, unsigned char b1, unsigned char b2)
{
    xds_608_t *x608 = (xds_608_t *)xds608;

    if (!x608 || x608->magic != XDS_608_MAGIC)
        return;

    /*xds is in field 2*/
    if (line != 284)
        return;

    b1 &= 0x7F;
    b2 &= 0x7F;

    if (b1 == 0 && b2 == 0)
        return;

    if (b1 < 0xF) {
        x608->in_xds = (process_xds(x608, b1, b2) == 0)? 1 : 0;
    } else if (b1 == 0xF) {
        decode_xds(x608, b2);
        x608->in_xds = 0;
    } else if (b1 <= 0x1F) {
        x608->in_xds = 0;
    } else if (x608->in_xds) {
        x608->in_xds = (process_xds(x608, b1, b2) == 0)? 1 : 0;
    }
}

void* xds_608_create(xds_start_param_t *param)
{
    xds_608_t *x608 = (xds_608_t *)calloc(1, sizeof(xds_608_t));
    if (!x608)
        return NULL;

    x608->magic = XDS_608_MAGIC;
    x608->param = *param;

    int i;
    for (i = 0; i < MAX_PKTS; i++)
        x608->pkts[i].class = -1;

    return x608;
}

void xds_608_destroy(void *xds608)
{
    xds_608_t *x608 = (xds_608_t *)xds608;

    if (!x608 || x608->magic != XDS_608_MAGIC)
        return;

    x608->magic = 0;
    free(x608);
}
