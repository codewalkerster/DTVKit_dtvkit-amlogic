#include <time.h>
#include "sys/time.h"

#include "techtype.h"

#ifndef __STBOS_TIMER_H__
#define __STBOS_TIMER_H__

#define TIMER_FLAG_ONESHOT      0
#define TIMER_FLAG_REPEAT       1

typedef enum TimerMode
{
    EN_TIMERMODE_ONESHOT    = TIMER_FLAG_ONESHOT,
    EN_TIMERMODE_REPEAT     = TIMER_FLAG_REPEAT
} ENUM_TIMERMODE;

typedef struct Timer
{
    timer_t         id;
    int             flag;
    struct timeval  expire_value;
    void (*expireCB)(void *arg);
    void            *userptr;
}Timer_s;


//新添加一个定时器
int STB_OSAddTimer(Timer_s *timer);

//重置定时器
int STB_OSRestartTimer(Timer_s *timer);

//删除定时器
int STB_OSDeleteTimer(Timer_s *timer);

#endif
