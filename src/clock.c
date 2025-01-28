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

extern const screen_pixel_t graphics_white_pixel;

#if 0
void *clock_thread(void *arg)
{
  app_state_t *app_state_ptr = (app_state_t *)arg;

  double event_diff;
  int abs_event_diff;
  event_t *event_cursor;

  time_t rawtime;
  struct tm utctime;
  char buffer[80];

  /** Seperator line **/
  screen_drawHorizontalLine(5, 105, (800-10), &graphics_white_pixel);

  while(!app_state_ptr->app_exit)
  {
    /** Date & Time **/
    time(&rawtime);
    gmtime_r( &rawtime, &utctime );

    strftime(buffer,80,"%Y-%j", &utctime);
    date_render(buffer);

    strftime(buffer,80,"%H:%M:%S", &utctime);
    time_render(buffer);

    /** Events **/
    /* For each event, recalculate countdown and format the string */
    pthread_mutex_lock(&app_state_ptr->events_mutex);

    event_cursor = app_state_ptr->events;
    int r;
    while(event_cursor != NULL)
    {
      event_diff = difftime(rawtime, event_cursor->event_time);
      event_cursor->countdown_int = (int)event_diff;
      abs_event_diff = abs(event_cursor->countdown_int);
      if(event_cursor->countdown_string != NULL)
      {
        free(event_cursor->countdown_string);
      }
      r = asprintf(&event_cursor->countdown_string, "%c%02d:%02d:%02d",
        event_diff == 0 ? ' ' : (event_diff < 0 ? '-' : '+'),
        (abs_event_diff / 3600),
        ((abs_event_diff % 3600) / 60),
        (abs_event_diff % 60)
      );
      if(r < 0)
      {
        printf("[clock] Error allocating countdown string for event\n");
      }

      event_cursor = event_cursor->next;
    }

    event_line_render(app_state_ptr->events, app_state_ptr->config.display_show_previous_expired_event);

    pthread_mutex_unlock(&app_state_ptr->events_mutex);

    sleep_ms_or_signal(10, &app_state_ptr->app_exit);
  }

  return NULL;
}
#endif