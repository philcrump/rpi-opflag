#include <time.h>
#include <string.h>
#include <errno.h>

#include "timing.h"

uint64_t monotonic_ms(void)
{
    struct timespec tp;

    if(clock_gettime(CLOCK_MONOTONIC, &tp) != 0)
    {
        return 0;
    }

    return (uint64_t) tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}

uint64_t timestamp_ms(void)
{
    struct timespec tp;

    if(clock_gettime(CLOCK_REALTIME, &tp) != 0)
    {
        return 0;
    }

    return (uint64_t) tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}

void sleep_ms(uint32_t _duration)
{
    struct timespec req, rem;
    req.tv_sec = _duration / 1000;
    req.tv_nsec = (_duration - (req.tv_sec*1000))*1000*1000;

    while(nanosleep(&req, &rem) != 0 && errno == EINTR)
    {
        /* Interrupted by signal, shallow copy remaining time into request, and resume */
        req = rem;
    }
}

void sleep_ms_or_signal(uint32_t _duration, bool *app_exit_ptr)
{
    struct timespec req, rem;
    req.tv_sec = _duration / 1000;
    req.tv_nsec = (_duration - (req.tv_sec*1000))*1000*1000;

    while(nanosleep(&req, &rem) != 0 && errno == EINTR && *app_exit_ptr == false)
    {
        /* Interrupted by signal, shallow copy remaining time into request, and resume */
        req = rem;
    }
}

uint64_t timestamp_no_ms_from_rfc8601(const char *time_string)
{
    struct tm ctime;

    memset(&ctime, 0, sizeof(struct tm));

    strptime(time_string, "%FT%T%z", &ctime);

    return (((uint64_t)mktime(&ctime)) * 1000);
}