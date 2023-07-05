#ifndef __USBCIMODULEAPI_H
#define __USBCIMODULEAPI_H
#include <stdint.h>
#include <stdbool.h>
#define USB_CIMODULE_COMMAND_MAX_SIZE    0x2000
#define USB_CIMODULE_MEDIA_MAX_SIZE      (47 * 1024 )
#define ERROR_USB_REMOVE			-19
#define ERROR_INVALID_ARGUMENT		-22
#define ERROR_PROTOCOL_ERROR		-71
#define ERROR_ENDPOINT_SHUTDOWN		-108
#define ERROR_INVALID_HANDLE		-200
#define ERROR_USB_PARAM				-201
struct usb_cimodule_info
{
   unsigned short m_wVendorId;
   unsigned short m_wProductId;
   uint32_t m_dwCiCompatibility;
   unsigned char m_bIsCI20Detected;
   unsigned char m_arReserve[31];
};

#define USBCAM_MAX_NAME_LEN 30
typedef struct
{
  char   ci_manufacturer_name[USBCAM_MAX_NAME_LEN];
  char   ci_product_name[USBCAM_MAX_NAME_LEN];
  bool ci_plus_supported;
  bool op_profile_supported;
} usbci_module_capabilities_t;

//ci module ioctl command
#define AML_USBCAM_IOC_MAGIC						'c'
#define AML_USBCAM_IOC_GET_DRIVER_VERSION			_IOR(AML_USBCAM_IOC_MAGIC, 0, uint32_t)
#define AML_USBCAM_IOC_GET_INFO					_IOR(AML_USBCAM_IOC_MAGIC, 1, struct usb_cimodule_info)
#define AML_USBCAM_IOC_RESET						_IO(AML_USBCAM_IOC_MAGIC, 2)
#define AML_USBCAM_IOC_CANCEL_TRANSFER				_IO(AML_USBCAM_IOC_MAGIC, 3)
#define AML_USBCAM_IOC_MODULE_CAPABILITIES			_IOR(AML_USBCAM_IOC_MAGIC, 4, usbci_module_capabilities_t)

// int cimodule_cmd_intf_open(const char *dev_path, int flag);
// int cimodule_cmd_intf_close(int fd);
// unsigned char *cimodule_cmd_intf_readbuf(int fd, unsigned int cmd_readbuf_size);
// unsigned char *cimodule_cmd_intf_writebuf(int fd, unsigned int cmd_writebuf_size);
// void cimodule_cmd_intf_free_readbuf(unsigned char *buf);
// void cimodule_cmd_intf_free_writebuf(unsigned char *buf);
// int cimodule_cmd_intf_read(int fd, unsigned char *buf, unsigned int len, unsigned int *actual_len, unsigned int timeout);
// int cimodule_cmd_intf_write(int fd, unsigned char *buf, unsigned int len, unsigned int *actual_len, unsigned int timeout);
// int cimodule_media_intf_open(const char *dev_path, int flag);
// int cimodule_media_intf_close(int fd);
// unsigned char *cimodule_media_intf_readbuf(int fd, unsigned int media_readbuf_size);
// unsigned char *cimodule_media_intf_writebuf(int fd, unsigned int media_writebuf_size);
// void cimodule_media_intf_free_readbuf(unsigned char *buf);
// void cimodule_media_intf_free_writebuf(unsigned char *buf);
// int cimodule_media_intf_read(int fd, unsigned char *buf, unsigned int len, unsigned int *actual_len, unsigned int timeout);
// int cimodule_media_intf_write(int fd, unsigned char *buf, unsigned int len, unsigned int *actual_len, unsigned int timeout);
// int cimodule_get_driver_version(int fd, unsigned int *driver_version);
// int cimodule_get_usb_cimodule_info(int fd, struct usb_cimodule_info *usbcam_info);
// int aml_get_usbcam_module_capabilities(int fd, usbci_module_capabilities_t *usb_cimodule_info);
// int cimodule_reset(int fd);

#endif
