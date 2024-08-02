#ifndef _XDS_H_
#define _XDS_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    int sys;
    #define XDS_RATING_SYS_NONE     0
    #define XDS_RATING_SYS_MPAA     1
    #define XDS_RATING_SYS_TV_US    2
    #define XDS_RATING_SYS_TV_CA_EN 3
    #define XDS_RATING_SYS_TV_CA_FR 4

    /*The Rating*/
    int id;
    #define XDS_RATING_MPAA_NA    0
    #define XDS_RATING_MPAA_G     1
    #define XDS_RATING_MPAA_PG    2
    #define XDS_RATING_MPAA_PG_13 3
    #define XDS_RATING_MPAA_R     4
    #define XDS_RATING_MPAA_NC_17 5
    #define XDS_RATING_MPAA_X     6
    #define XDS_RATING_MPAA_NR    7

    #define XDS_RATING_TV_US_NONE  0
    #define XDS_RATING_TV_US_TV_Y  1
    #define XDS_RATING_TV_US_TV_Y7 2
    #define XDS_RATING_TV_US_TV_G  3
    #define XDS_RATING_TV_US_TV_PG 4
    #define XDS_RATING_TV_US_TV_14 5
    #define XDS_RATING_TV_US_TV_MA 6
    #define XDS_RATING_TV_US_NR    7

    #define XDS_RATING_TV_CA_FR_E  0
    #define XDS_RATING_TV_CA_FR_G  1
    #define XDS_RATING_TV_CA_FR_8  2
    #define XDS_RATING_TV_CA_FR_13 3
    #define XDS_RATING_TV_CA_FR_16 4
    #define XDS_RATING_TV_CA_FR_18 5

    #define XDS_RATING_TV_CA_EN_E  0
    #define XDS_RATING_TV_CA_EN_C  1
    #define XDS_RATING_TV_CA_EN_C8 2
    #define XDS_RATING_TV_CA_EN_G  3
    #define XDS_RATING_TV_CA_EN_PG 4
    #define XDS_RATING_TV_CA_EN_14 5
    #define XDS_RATING_TV_CA_EN_18 6

    /*Valid if XDS_RATING_SYS_TV_US*/
    int dlsv;
    #define XDS_RATING_D 0x8
    #define XDS_RATING_L 0x4
    #define XDS_RATING_S 0x2
    #define XDS_RATING_V 0x1
} xds_rating_t;

typedef struct {
    int ts_id;
} xds_network_t;

typedef void (*xds_callback_t)(int event, void *data, void *priv);

typedef struct {
    /*
     * Source of the XDS data,
     */
    int input;
    #define XDS_INPUT_VBI0       0x00
    #define XDS_INPUT_USERDATA0  0x10

    /*Valid only if input==XDS_INPUT_USERDATAx*/
    int vfmt;

    int events;
    #define XDS_EVT_RATING  0x1
    #define XDS_EVT_NETWORK 0x2

    /*Event callback*/
    xds_callback_t callback;
    void *callback_priv;

    char owner[32];
} xds_start_param_t;

struct xds_handle_s;

typedef struct xds_handle_s xds_handle_t;

/*start the xds decoder*/
xds_handle_t* xds_start(xds_start_param_t *param);
xds_handle_t* xds_start_ext(xds_start_param_t *param, void*(*connect)());

/*stop the xds decoder*/
int xds_stop(xds_handle_t *xds);

#ifdef __cplusplus
}
#endif

#endif
