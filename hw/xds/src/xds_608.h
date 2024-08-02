#ifndef _XDS_608_H_
#define _XDS_608_H_

#include "xds.h"

#ifdef __cplusplus
extern "C" {
#endif

void* xds_608_create(xds_start_param_t *param);

void xds_608_decode(void *xds608, int line, unsigned char b1, unsigned char b2);

void xds_608_destroy(void *xds608);

#ifdef __cplusplus
}
#endif

#endif

