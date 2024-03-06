#ifndef _AFD_CTRL_H_
#define _AFD_CTRL_H_

#include "afd_datatype.h"

#define AFD_DEV "/dev/aml_afd"

extern void afd_create_context(int path, int instance_id);
extern void afd_release_context(int path);
extern void afd_setMhegScalling(int path, int x, int y, int w, int h, int res_x, int res_y);
extern void afd_setAppScalling(int path, int x, int y, int w, int h, int res_x, int res_y);
extern void afd_disableScalling(int path);
extern int  afd_getContextNum(void);

#endif /*_AFD_CTRL_H_*/