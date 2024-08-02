#ifndef _XDS_708_H_
#define _XDS_708_H_

#include "xds.h"
#include "ccdatabase/dataserver_data_userdata.h"

#ifdef __cplusplus
extern "C" {
#endif

void* xds_708_create(xds_start_param_t *param);

void xds_708_destroy(void *xds708);

void xds_708_decode(void *xds708, const uint8_t *buf, size_t size);

void xds_708_decode_userdata(void *xds708, const uint8_t *buf, size_t size);

void xds_708_sequence_userdata(void *xds708, ds_data_userdata_t *userdata);


#ifdef __cplusplus
}
#endif

#endif

