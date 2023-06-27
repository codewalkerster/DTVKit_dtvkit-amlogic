#ifndef __ATV_FEND_INTERNAL_H__
#define __ATV_FEND_INTERNAL_H__

#include <pthread.h>

/*add for config define for linux dvb *.h*/
//#include <linux/dvb/frontend.h>



#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#ifndef AM_SUCCESS
/**\brief Function result: Success*/
#define AM_SUCCESS     (0)
#endif

#ifndef AM_FAILURE
/**\brief Function result: Unknown error*/
#define AM_FAILURE     (-1)
#endif

#ifndef AM_TRUE
/**\brief Boolean value: true*/
#define AM_TRUE        (1)
#endif

#ifndef AM_FALSE
/**\brief Boolean value: false*/
#define AM_FALSE       (0)
#endif

#define FE_AUTO (-1)        /**< Do not care the mode*/
#define FE_UNKNOWN (-2)     /**< Set mode to unknown, something like reset*/

#define AM_ERROR_BASE(_mod)    ((_mod)<<24)
#define AM_MOD_FEND            (10)


/****************************************************************************
 * Type definitions
 ***************************************************************************/

enum AM_FEND_ErrorCode
{
    AM_FEND_ERROR_BASE=AM_ERROR_BASE(AM_MOD_FEND),
    AM_FEND_ERR_NO_MEM,                   /**< Not enough memory*/
    AM_FEND_ERR_BUSY,                     /**< The device has already been opened*/
    AM_FEND_ERR_INVALID_DEV_NO,           /**< Invalid device numbe*/
    AM_FEND_ERR_NOT_OPENED,               /**< The device not open*/
    AM_FEND_ERR_CANNOT_CREATE_THREAD,     /**< Cannot create new thread*/
    AM_FEND_ERR_NOT_SUPPORTED,            /**< Not supported*/
    AM_FEND_ERR_CANNOT_OPEN,              /**< Cannot open device*/
    AM_FEND_ERR_TIMEOUT,                  /**< Timeout*/
    AM_FEND_ERR_INVOKE_IN_CB,             /**< Invoke in callback function*/
    AM_FEND_ERR_IO,                       /**< IO error*/
    AM_FEND_ERR_BLINDSCAN,                /**< Blindscan error*/
    AM_FEND_ERR_BLINDSCAN_INRUNNING,      /**< In Blindscan*/
    AM_FEND_ERR_END
};

typedef struct atv_status_s atv_status_t;
typedef struct tuner_param_s tuner_param_t;

typedef int AM_ErrorCode_t;
typedef uint8_t AM_Bool_t;

/**\brief frontend open parameter*/
typedef struct
{
    int mode;
} AM_FEND_OpenPara_t;

/**\brief frontend callback function*/
typedef void (*AM_FEND_Callback_t)(int dev_no, struct dvb_frontend_event *evt, void *user_data);



/**\brief 前端设备*/
typedef struct AM_FEND_Device AM_FEND_Device_t;

/**\brief 前端设备驱动*/
typedef struct
{
    AM_ErrorCode_t (*open) (AM_FEND_Device_t *dev, const AM_FEND_OpenPara_t *para);
    AM_ErrorCode_t (*close) (AM_FEND_Device_t *dev);

    AM_ErrorCode_t (*set_mode) (AM_FEND_Device_t *dev, int mode);

    AM_ErrorCode_t (*set_para) (AM_FEND_Device_t *dev, const struct dvb_frontend_parameters *para);
    AM_ErrorCode_t (*get_para) (AM_FEND_Device_t *dev, struct dvb_frontend_parameters *para);
    AM_ErrorCode_t (*set_prop) (AM_FEND_Device_t *dev, const struct dtv_properties *prop);
    AM_ErrorCode_t (*get_prop) (AM_FEND_Device_t *dev, struct dtv_properties *prop);

    AM_ErrorCode_t (*get_status) (AM_FEND_Device_t *dev, fe_status_t *status);
    AM_ErrorCode_t (*get_atv_status)(AM_FEND_Device_t *dev, atv_status_t *atv_status);
    AM_ErrorCode_t (*wait_event) (AM_FEND_Device_t *dev, struct dvb_frontend_event *evt, int timeout);

    AM_ErrorCode_t (*get_snr) (AM_FEND_Device_t *dev, int *snr);
    AM_ErrorCode_t (*get_ber) (AM_FEND_Device_t *dev, int *ber);
    AM_ErrorCode_t (*get_strength) (AM_FEND_Device_t *dev, int *strength);


    AM_ErrorCode_t (*fine_tune)(AM_FEND_Device_t *dev, unsigned int freq);
    AM_ErrorCode_t (*set_cvbs_amp_out)(AM_FEND_Device_t *dev, tuner_param_t *tuner_para);
    AM_ErrorCode_t (*set_afc)(AM_FEND_Device_t *dev, unsigned int afc);

} AM_FEND_Driver_t;


/**\brief 前端设备*/
struct AM_FEND_Device
{
    int                dev_no;        /**< 设备号*/
    const AM_FEND_Driver_t  *drv;     /**< 设备驱动*/
    void              *drv_data;      /**< 驱动私有数据*/
    int                open_count;    /**< 设备打开次数计数*/
    AM_Bool_t          enable_thread; /**< 状态监控线程是否运行*/
    AM_Bool_t          active_thread; /**< 状态监控线程是否运行*/
    pthread_t          thread;        /**< 状态监控线程*/
    pthread_mutex_t    lock;          /**< 设备数据保护互斥体*/
    pthread_cond_t     cond;          /**< 状态监控线程控制条件变量*/
    int                flags;         /**< 状态监控线程标志*/
    AM_FEND_Callback_t cb;            /**< 状态监控回调函数*/
    int                curr_mode;     /**< 当前解调模式*/
    void              *user_data;     /**< 回调函数参数*/
    int                user_data_len; /**< 回调函数参数长度*/
    AM_Bool_t          enable_cb;     /**< 允许或者禁止状态监控回调函数*/
    fe_status_t        status;        /**< device status*/
};






/****************************************************************************
 * Function prototypes
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

