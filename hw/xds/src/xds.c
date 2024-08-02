#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "ccdatabase/dataserver_client.h"
#include "ccdatabase/dataserver_message.h"
#include "ccdatabase/dataserver_cmd_resp.h"
#include "ccdatabase/dataserver_data_vbi.h"
#include "ccdatabase/dataserver_data_userdata.h"

#include "xds.h"
#include "xds_608.h"
#include "xds_708.h"

#define xds_inf(_fmt_, ...) printf("I "_fmt_"\n", ##__VA_ARGS__)
#define xds_err(_fmt_, ...) printf("E "_fmt_"\n", ##__VA_ARGS__)

typedef struct {
    xds_start_param_t param;

    ds_conn_t *das;
    ds_conn_t *(*das_connect)();

    pthread_t thread;
    int running;

    void *decoder;
} xds_ctx_t;

#define DAS_RET_CHK() \
        if (ret == 0) {\
            continue;\
        } else if (ret == -1) {\
            xds_err("bad parameter");\
            continue;\
        } else if (ret <= -3) {\
            xds_err("dataserver disconnect, retry connect...");\
            ctx->das = NULL;\
            stage = STAGE_INIT;\
            continue;\
        }


static void *xds_vbi_task(void *args)
{
    enum {
        STAGE_INIT,
        STAGE_PAR = STAGE_INIT,
        STAGE_SRC,
        STAGE_DAT,
    } stage = STAGE_INIT;

    xds_ctx_t *ctx = (xds_ctx_t *)args;

    xds_inf("xds vbi task started.");

    int buf_size = sizeof(ds_data_vbi_t) * 50;

    uint8_t *buf = (uint8_t *)malloc(buf_size);

    if (!buf) {
        xds_err("memory fail");
        goto exit;
    }

    ctx->decoder = xds_608_create(&ctx->param);

    if (!ctx->decoder) {
        xds_err("failed to create xds x608 decoder");
        goto exit;
    }

    ctx->das = ctx->das_connect();

    int ret, last_ret = -100;
    while (ctx->running) {

        if (!ctx->das) {
            xds_err("dataserver not connected, try reconnect...");

            sleep(1);
            ctx->das = ctx->das_connect();

            continue;
        }

        /*prepare for the data*/
        uint8_t msg[sizeof(ds_msg_head_t) + 64];
        ds_msg_head_t *pmsg = (ds_msg_head_t *)msg;
        pmsg->magic = DS_MAGIC;
        pmsg->type = DS_T_CNTL;

        switch (stage) {
            case STAGE_PAR:
                pmsg->size = snprintf((char *)pmsg->body, 64, ds_cmd_set_source_para_vbi_type, 1/*USCC*/);
                pmsg->size++;
                ret = dataserver_write_message(ctx->das, (uint8_t *)pmsg, sizeof(ds_msg_head_t) + pmsg->size);
                DAS_RET_CHK();
            break;

            case STAGE_SRC:
                pmsg->size = snprintf((char *)pmsg->body, 64, ds_cmd_set_source, "/dev/vbi", ctx->param.owner);
                pmsg->size++;
                ret = dataserver_write_message(ctx->das, pmsg, sizeof(ds_msg_head_t) + pmsg->size);
                DAS_RET_CHK();
            break;

            default:
            break;
        }

        ret = dataserver_read_message(ctx->das, buf, buf_size, 1000);

        if (ret == -2) {
            if (ret != last_ret) {
                xds_inf("timeout");
            }
            last_ret = ret;
            continue;
        }

        last_ret = ret;

        DAS_RET_CHK();

        pmsg = (ds_msg_head_t *)buf;

        xds_inf("msg:%d, size(%d)", pmsg->type, pmsg->size);

        switch (stage) {
            case STAGE_SRC:
            case STAGE_PAR:

                if (pmsg->type == DS_T_RESP) {
                    if (strncmp((char *)pmsg->body, "ok", 2) == 0) {
                        /*next stage*/
                        stage = (stage == STAGE_PAR)? STAGE_SRC : STAGE_DAT;
                        break;
                    }
                }

                /*stage fail, should exit and wait for next start*/
                xds_err("fail in stage %d", stage);
                dataserver_disconnect(ctx->das);
                ctx->das = NULL;
                stage = STAGE_INIT;
                goto exit;

                break;

            case STAGE_DAT:
            default:
                if (pmsg->type == DS_T_DATA) {
                    ds_data_vbi_t *vbi = (ds_data_vbi_t *)pmsg->body;
                    int left = pmsg->size;

                    while (left >= sizeof(ds_data_vbi_t)) {

                        xds_608_decode(ctx->decoder, vbi->line_num, vbi->b[0], vbi->b[1]);

                        vbi++;
                        left -= sizeof(ds_data_vbi_t);
                    }

                } else if (pmsg->type == DS_T_EVNT) {

                    xds_err("data overflow, restart data pipe");

                    dataserver_disconnect(ctx->das);
                    ctx->das = NULL;
                    stage = STAGE_INIT;
                    continue;
                }
                break;
        }
    }

exit:
    if (ctx->das)
        dataserver_disconnect(ctx->das);

    if (ctx->decoder)
        xds_608_destroy(ctx->decoder);

    if (buf)
        free(buf);

    xds_inf("xds vbi task exit.");
    return NULL;
}

static void *xds_userdata_task(void *args)
{
    enum {
        STAGE_INIT,
        STAGE_RB = STAGE_INIT,
        STAGE_SRC,
        STAGE_DAT,
    } stage = STAGE_INIT;

    xds_ctx_t *ctx = (xds_ctx_t *)args;

    xds_inf("xds userdata task started.");

    int buf_size = 1024 * 10;

    uint8_t *buf = (uint8_t *)malloc(buf_size);

    if (!buf) {
        xds_err("memory fail");
        goto exit;
    }

    ctx->decoder = xds_708_create(&ctx->param);

    if (!ctx->decoder) {
        xds_err("failed to create xds 708 decoder");
        goto exit;
    }

    ctx->das = ctx->das_connect();

    int ret, last_ret = -100;
    while (ctx->running) {

        if (!ctx->das) {
            xds_err("dataserver not connected, try reconnect...");

            sleep(1);
            ctx->das = ctx->das_connect();

            continue;
        }

        /*prepare for the data*/
        uint8_t msg[sizeof(ds_msg_head_t) + 64];
        ds_msg_head_t *pmsg = (ds_msg_head_t *)msg;
        pmsg->magic = DS_MAGIC;
        pmsg->type = DS_T_CNTL;

        switch (stage) {
            case STAGE_RB:
                pmsg->size = snprintf((char *)pmsg->body, 64, ds_cmd_enable_ringbuffer, 1024 * 512);
                pmsg->size++;
                ret = dataserver_write_message(ctx->das, pmsg, sizeof(ds_msg_head_t) + pmsg->size);
                DAS_RET_CHK();
            break;

            case STAGE_SRC:
                pmsg->size = snprintf((char *)pmsg->body, 64, ds_cmd_set_source, "/dev/amstream_userdata", ctx->param.owner);
                pmsg->size++;
                ret = dataserver_write_message(ctx->das, pmsg, sizeof(ds_msg_head_t) + pmsg->size);
                DAS_RET_CHK();
            break;

            default:
            break;
        }

        ret = dataserver_read_message(ctx->das, buf, buf_size, 1000);

        if (ret == -2) {
            if (ret != last_ret) {
                xds_inf("timeout");
            }
            last_ret = ret;
            continue;
        }

        last_ret = ret;

        DAS_RET_CHK();

        pmsg = (ds_msg_head_t *)buf;

        xds_inf("msg:%d, size(%d)", pmsg->type, pmsg->size);

        switch (stage) {
            case STAGE_RB:
            case STAGE_SRC:

                if (pmsg->type == DS_T_RESP) {
                    if (strncmp((char *)pmsg->body, "ok", 2) == 0) {
                        /*next stage*/
                        stage = (stage == STAGE_RB)? STAGE_SRC : STAGE_DAT;
                        break;
                    }
                }

                /*stage fail, should exit and wait for next start*/
                xds_err("fail in stage %d", stage);
                dataserver_disconnect(ctx->das);
                ctx->das = NULL;
                stage = STAGE_INIT;
                goto exit;

                break;

            case STAGE_DAT:
            default:
                if (pmsg->type == DS_T_DATA) {
                    int left = pmsg->size;

                    ds_data_userdata_t *ud = (ds_data_userdata_t *)pmsg->body;

                    while (left >= sizeof(ds_data_userdata_t)) {

                        /*queue the ud*/
                        xds_708_sequence_userdata(ctx->decoder, ud);


                        int udsize = sizeof(ds_data_userdata_t) + ud->data_size;
                        left -= udsize;
                        ud = (ds_data_userdata_t *)(pmsg->body + udsize);
                    }

                } else if (pmsg->type == DS_T_EVNT) {

                    xds_err("data overflow, restart data pipe");

                    dataserver_disconnect(ctx->das);
                    ctx->das = NULL;
                    stage = STAGE_INIT;
                    continue;
                }
                break;
        }
    }

exit:
    if (ctx->das)
        dataserver_disconnect(ctx->das);

    if (ctx->decoder)
        xds_708_destroy(ctx->decoder);

    if (buf)
        free(buf);

    xds_inf("xds userdata task exit.");
    return NULL;
}

/*start the xds decoder*/
xds_handle_t* xds_start_ext(xds_start_param_t *param,  void*(*connect)())
{
    xds_ctx_t *ctx = (xds_ctx_t *)calloc(1, sizeof(xds_ctx_t));
    if (!ctx) {
        xds_err("memory fail.");
        return NULL;
    }

    ctx->param = *param;
    ctx->das_connect = (ds_conn_t *(*)())connect;

    xds_inf("start xds...");
    ctx->running = 1;
    pthread_create(&ctx->thread, NULL,
            (ctx->param.input == XDS_INPUT_VBI0)? xds_vbi_task : xds_userdata_task,
            ctx);

    return (xds_handle_t *)ctx;
}

xds_handle_t* xds_start(xds_start_param_t *param)
{
    return xds_start_ext(param, (void*(*)())dataserver_connect);
}

/*stop the xds decoder*/
int xds_stop(xds_handle_t *xds)
{
    if (!xds) {
        xds_err("bad xds handle.");
        return -1;
    }

    xds_ctx_t *ctx = (xds_ctx_t *)xds;

    xds_inf("stop xds...");
    ctx->running = 0;
    pthread_join(ctx->thread, NULL);
    xds_inf("xds stopped.");

    return 0;
}


