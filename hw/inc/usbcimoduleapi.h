#ifndef __USBCIMODULEAPI_H
#define __USBCIMODULEAPI_H
#include <stdint.h>
#include <stdbool.h>
#define USB_CIMODULE_COMMAND_MAX_SIZE    0x2000
#define USB_CIMODULE_MEDIA_MAX_SIZE      (47 * 1024 )
#define ERROR_USB_REMOVE          -19
#define ERROR_INVALID_ARGUMENT    -22
#define ERROR_PROTOCOL_ERROR      -71
#define ERROR_ENDPOINT_SHUTDOWN   -108
#define ERROR_INVALID_HANDLE      -200
#define ERROR_USB_PARAM           -201
struct usb_cimodule_info
{
   unsigned short vendor_id;
   unsigned short product_id;
   uint32_t ci_compatibility;
   unsigned char is_ci20_detected;
   unsigned char arreserve[31];
};

enum aml_usbcam_device_state {
	DEVICE_CONNECT = 0,
	DEVICE_DISCONNECT = 1
};

#define USBCAM_MAX_NAME_LEN 30
typedef struct
{
  char    ci_manufacturer_name[USBCAM_MAX_NAME_LEN];
  char    ci_product_name[USBCAM_MAX_NAME_LEN];
  bool    ci_plus_supported;
  bool    op_profile_supported;
} usbci_module_capabilities_t;

//ci module ioctl command
#define AML_USBCAM_IOC_MAGIC                  'c'
#define AML_USBCAM_IOC_GET_DRIVER_VERSION     _IOR(AML_USBCAM_IOC_MAGIC, 0, uint32_t)
#define AML_USBCAM_IOC_GET_INFO               _IOR(AML_USBCAM_IOC_MAGIC, 1, struct usb_cimodule_info)
#define AML_USBCAM_IOC_RESET                  _IO(AML_USBCAM_IOC_MAGIC, 2)
#define AML_USBCAM_IOC_CANCEL_TRANSFER        _IO(AML_USBCAM_IOC_MAGIC, 3)
#define AML_USBCAM_IOC_MODULE_CAPABILITIES    _IOR(AML_USBCAM_IOC_MAGIC, 4, usbci_module_capabilities_t)
#define AML_USBCAM_IOC_GET_MODULE_STATE       _IOR(AML_USBCAM_IOC_MAGIC, 5, uint32_t)
#define AML_USBCAM_IOC_SET_MODULE_STATE       _IOR(AML_USBCAM_IOC_MAGIC, 6, uint32_t)


#endif
