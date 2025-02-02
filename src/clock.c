#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>

#include "main.h"
#include "clock.h"
#include "screen.h"
#include "graphics.h"
#include "font/font.h"
#include "util/timing.h"

void *clock_thread(void *arg)
{
  app_state_t *app_state_ptr = (app_state_t *)arg;

  time_t rawtime;
  struct tm utctime;
  char buffer[80];

  while(!app_state_ptr->app_exit)
  {
    /** Date & Time **/
    time(&rawtime);
    gmtime_r(&rawtime, &utctime);

    strftime(buffer,80,"%Y-%j %H:%M:%S", &utctime);
    datetime_render(buffer);

    sleep_ms_or_signal(10, &app_state_ptr->app_exit);
  }

  return NULL;
}