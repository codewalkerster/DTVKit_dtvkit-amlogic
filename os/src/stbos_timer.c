#include <string.h>
#include <signal.h>
#include <time.h>

#include "stbos_timer.h"

//新添加一个定时器
int STB_OSAddTimer(Timer_s *timer)
{
    struct itimerspec spec;

    if (NULL == timer || NULL == timer->expireCB)
    {
        return -1;
    }

    //创建定时器
    struct sigevent eventp;
    memset(&eventp, 0, sizeof(eventp));
    eventp.sigev_notify = SIGEV_THREAD;
    eventp.sigev_value.sival_ptr = timer->userptr;
    eventp.sigev_notify_function = (void*)timer->expireCB;
    eventp.sigev_notify_attributes = NULL;
    timer_create(CLOCK_MONOTONIC, &eventp, &timer->id);

    //启动定时器
    memset(&spec, 0, sizeof(struct itimerspec));
    if (TIMER_FLAG_ONESHOT != timer->flag)
    {
        spec.it_interval.tv_sec = timer->expire_value.tv_sec;
        spec.it_interval.tv_nsec = timer->expire_value.tv_usec*1000;
    }
    spec.it_value.tv_sec = timer->expire_value.tv_sec;
    spec.it_value.tv_nsec = timer->expire_value.tv_usec*1000;
    timer_settime(timer->id, 0, &spec, NULL);
    return 0;
}

//重置定时器
int STB_OSRestartTimer(Timer_s *timer)
{
    struct itimerspec spec;
    if (NULL == timer || NULL == timer->id)
    {
        return -1;
    }

    //启动定时器
    memset(&spec, 0, sizeof(struct itimerspec));
    if (TIMER_FLAG_ONESHOT != timer->flag)
    {
        spec.it_interval.tv_sec = timer->expire_value.tv_sec;
        spec.it_interval.tv_nsec = timer->expire_value.tv_usec*1000;
    }
    spec.it_value.tv_sec = timer->expire_value.tv_sec;
    spec.it_value.tv_nsec = timer->expire_value.tv_usec*1000;
    timer_settime(timer->id, 0, &spec, NULL);

    return 0;
}

//删除定时器
int STB_OSDeleteTimer(Timer_s *timer)
{

    if (NULL == timer || NULL == timer->id)
    {
        return -1;
    }

    timer_delete(timer->id);
    timer->id = NULL;

    return 0;
}

