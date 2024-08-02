
/*
 * Read cc608 raw data from stdin,
 * decode it then print xds info to stdout
 * */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "xds_608.h"

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


int main(int argc, char *argv[])
{
    xds_start_param_t param = {
        .input = XDS_INPUT_VBI0,
        .events = XDS_EVT_RATING | XDS_EVT_NETWORK,
        .callback = xds_callback,
        .callback_priv = "raw608",
        .owner = "raw608",
    };

    void *decoder = xds_608_create(&param);

    inf("decoder[%p] created", decoder);
    assert(decoder);

    unsigned char buf[3];

    int ret;

    do {
        ret = read(0, buf, 3);
        if (ret == 3) {

            //inf("%02X:%c%c:%02X", buf[0], buf[1] & 0x7f, buf[2] & 0x7f, buf[2]);

            int cc_valid = buf[0] & 4;
            int cc_type = buf[0] & 3;
            if (!cc_valid)
                continue;

            if (cc_type == 1)/*608 field 2*/
                xds_608_decode(decoder, 284, buf[1], buf[2]);
        }
    } while (ret == 3);

    xds_608_destroy(decoder);

    return 0;
}
