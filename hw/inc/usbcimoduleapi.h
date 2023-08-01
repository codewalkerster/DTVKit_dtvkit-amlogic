#ifndef __USBCIMODULEAPI_H
#define __USBCIMODULEAPI_H
#include <stdint.h>
#define USB_CIMODULE_COMMAND_MAX_SIZE    0x2000
#define USB_CIMODULE_MEDIA_MAX_SIZE      (47 * 1024)
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
   unsigned char m_bIsCI20Deteced;
   unsigned char m_arReserve[31];
};
int cimodule_cmd_intf_open(const char *i_pbDevpath, int i_nflags);
int cimodule_cmd_intf_close(int i_fd);
unsigned char *cimodule_cmd_intf_mmap_readbuf(int i_fd, unsigned int i_dwMaxCmdBufReadSize);
unsigned char *cimodule_cmd_intf_mmap_writebuf(int i_fd, unsigned int i_dwMaxCmdBufWriteSize);
void cimodule_cmd_intf_munmap_readbuf(unsigned char *i_pbBuf);
void cimodule_cmd_intf_munmap_writebuf(unsigned char *i_pbBuf);
int cimodule_cmd_intf_read(int i_fd, unsigned char *o_pbBuf, unsigned int i_dwRqstLen, unsigned int *o_pdwActualLen, unsigned int i_dwTimeout);
int cimodule_cmd_intf_write(int i_fd, unsigned char *i_pbBuf, unsigned int i_dwRqstLen, unsigned int *o_pdwActualLen, unsigned int i_dwTimeout);
int cimodule_media_intf_open(const char *i_pbDevpath, int i_nflags);
int cimodule_media_intf_close(int i_fd);
unsigned char *cimodule_media_intf_mmap_readbuf(int i_fd, unsigned int i_dwMaxMediaBufReadSize);
unsigned char *cimodule_media_intf_mmap_writebuf(int i_fd, unsigned int i_dwMaxMediaBufWriteSize);
void cimodule_media_intf_munmap_readbuf(unsigned char *i_pbBuf);
void cimodule_media_intf_munmap_writebuf(unsigned char *i_pbBuf);
int cimodule_media_intf_read(int i_fd, unsigned char *o_pbBuf, unsigned int i_dwRqstLen, unsigned int *o_pdwActualLen, unsigned int i_dwTimeout);
int cimodule_media_intf_write(int i_fd, unsigned char *i_pbBuf, unsigned int i_dwRqstLen, unsigned int *o_pdwActualLen, unsigned int i_dwTimeout);
int cimodule_get_driver_version(int i_fd, unsigned int *o_pdwDriverVersion);
int cimodule_get_usb_cimodule_info(int i_fd, struct usb_cimodule_info *o_ptUsbCiModuleInfo);
int cimodule_reset(int i_fd);
#endif
