
/*
 * Read data from dataserver,
 * decode it and print xds info to stdout
 * */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "ccdatabase/dataserver_client.h"
#include "xds.h"

#define inf(_fmt, ...) printf("I "_fmt"\n", ##__VA_ARGS__)
#define err(_fmt, ...) printf("E "_fmt"\n", ##__VA_ARGS__)

static void xds_callback(int event, void *data, void *priv)
{
    switch (event) {
        case XDS_EVT_RATING:
            {
                xds_rating_t *rating = (xds_rating_t *)data;

                inf("Rating cb: sys[%d] id[%d] dlsv[%#x] priv[%s]",
                        rating->sys,
                        rating->id,
                        rating->dlsv,
                        (char*)priv);
            } break;

        case XDS_EVT_NETWORK:
            {
                xds_network_t *net = (xds_network_t *)data;

                inf("Network cb: tsid[%d] priv[%s]", net->ts_id, (char *)priv);
            } break;

        default:
            err("unknown evt[%d]", event);
            break;
    }
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static struct in_addr addr;
static uint32_t port;

static void* das_connect(void)
{
    return dataserver_connect_ip(addr.s_addr, port);
}

int main(int argc, char *argv[])
{
    int vbi = 0;

    if (strstr(argv[0], "vbi"))
        vbi = 1;

    xds_start_param_t param = {
        .input = vbi? XDS_INPUT_VBI0 : XDS_INPUT_USERDATA0,
        .events = XDS_EVT_RATING | XDS_EVT_NETWORK,
        .callback = xds_callback,
        .callback_priv = "testxds",
        .owner = "testxds",
    };

    int is_ip = 0;

    if (argc > 1) {
        if (inet_aton(argv[1], &addr) != 0
                && sscanf(argv[2], "%i", &port) == 1)
            is_ip = 1;
    }

    void *decoder = is_ip?
        xds_start_ext(&param, das_connect)
        : xds_start(&param);

    inf("decoder[%p] created", decoder);
    assert(decoder);

    for (;;)
        sleep(1);

    return 0;
}
