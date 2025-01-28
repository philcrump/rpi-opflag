#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <math.h>

#include "main.h"
#include "clock.h"
#include "screen.h"
#include "graphics.h"
#include "font/font.h"

#include "util/timing.h"


/** Alarms **/

#define ALARM_LINE_WIDTH      350
#define ALARM_LINE_HEIGHT     300
#define ALARM_EACH_LINE_HEIGHT  50
//static screen_pixel_t alarm_line_buffer[ALARM_LINE_HEIGHT][ALARM_LINE_WIDTH] __attribute__ ((aligned (NEON_ALIGNMENT))) = { 0 };

#define ALARM_LINE_POS_X    25
#define ALARM_LINE_POS_Y    25

/** Connection Status **/

#define CONNECTIONSTATUS_WIDTH      300
#define CONNECTIONSTATUS_HEIGHT     25
static screen_pixel_t connectionstatus_buffer[CONNECTIONSTATUS_HEIGHT][CONNECTIONSTATUS_WIDTH] __attribute__ ((aligned (NEON_ALIGNMENT))) = { 0 };

#define CONNECTIONSTATUS_POS_X    10
#define CONNECTIONSTATUS_POS_Y    (SCREEN_SIZE_Y - 10 - CONNECTIONSTATUS_HEIGHT)

/** FLAG **/

#define FLAG_WIDTH      400
#define FLAG_HEIGHT     480
static screen_pixel_t flag_buffer[FLAG_HEIGHT][FLAG_WIDTH] __attribute__ ((aligned (NEON_ALIGNMENT))) = { 0 };

#define FLAG_POS_X    400
#define FLAG_POS_Y    0


const screen_pixel_t graphics_white_pixel =
{
  .Alpha = 0x80,
  .Red = 0xFF,
  .Green = 0xFF,
  .Blue = 0xFF
};

const screen_pixel_t graphics_black_pixel =
{
  .Alpha = 0x80,
  .Red = 0x00,
  .Green = 0x00,
  .Blue = 0x00
};

const screen_pixel_t graphics_subtlegrey_pixel =
{
  .Alpha = 0x80,
  .Red = 0x30,
  .Green = 0x30,
  .Blue = 0x30
};

const screen_pixel_t graphics_darkgrey_pixel =
{
  .Alpha = 0x80,
  .Red = 0x70,
  .Green = 0x70,
  .Blue = 0x70
};

const screen_pixel_t graphics_grey_pixel =
{
  .Alpha = 0x80,
  .Red = 0xA0,
  .Green = 0xA0,
  .Blue = 0xA0
};

const screen_pixel_t graphics_red_pixel =
{
  .Alpha = 0x80,
  .Red = 0xFF,
  .Green = 0x00,
  .Blue = 0x00
};

const screen_pixel_t graphics_yellow_pixel =
{
  .Alpha = 0x80,
  .Red = 0xFF,
  .Green = 0xFF,
  .Blue = 0x00
};

const screen_pixel_t graphics_blue_pixel =
{
  .Alpha = 0x80,
  .Red = 0x00,
  .Green = 0x00,
  .Blue = 0xFF
};

const screen_pixel_t graphics_green_pixel =
{
  .Alpha = 0x80,
  .Red = 0x00,
  .Green = 0xFF,
  .Blue = 0x00
};

const screen_pixel_t graphics_amber_pixel =
{
  .Alpha = 0x80,
  .Red = 0xFF,
  .Green = 0xBF,
  .Blue = 0x00
};

#if 0
static inline void event_line_render_font_cb(int x, int y, screen_pixel_t *pixel_ptr)
{
  memcpy(&(event_line_buffer[y][x]), pixel_ptr, sizeof(screen_pixel_t));
}

static void event_line_graphic_transition(int x, int y, int height, int width, const screen_pixel_t *pixel_start_ptr, const screen_pixel_t *pixel_end_ptr)
{
  /* For each row */
  for(int i = 0; i < height; i++)
  {
    /* For each pixel in row */
    for(int j = 0; j < width; j++)
    {
      if(((float)(j)/width) < ((float)(height-i)/height))
      {
        memcpy(&(event_line_buffer[i+y][j+x]), pixel_start_ptr, sizeof(screen_pixel_t));
      }
      else
      {
        memcpy(&(event_line_buffer[i+y][j+x]), pixel_end_ptr, sizeof(screen_pixel_t));
      }
    }
  }
}

void event_line_render(event_t *events_ptr, bool show_previous_expired_event)
{
  /* Clear all lines buffer */
  for(uint32_t i = 0; i < EVENT_LINE_HEIGHT; i++)
  {
    for(uint32_t j = 0; j < EVENT_LINE_WIDTH; j++)
    {
      memcpy(&(event_line_buffer[i][j]), &graphics_black_pixel, sizeof(screen_pixel_t));
    }
  }

  event_t *event_cursor;
  event_t *event_previous_cursor;

  /* Find the next event */

  event_cursor = events_ptr;
  event_previous_cursor = NULL;

  while(event_cursor != NULL
    && event_cursor->next != NULL
    && event_cursor->countdown_int > 0)
  {
    event_previous_cursor = event_cursor;
    event_cursor = event_cursor->next;
  }

  if(event_cursor == NULL || event_cursor->countdown_int >= 60)
  {
    /* No events or newest event is more than 1 minute ago */

    /* Draw message, render early, and exit */
    font_render_string_with_callback(
      0, // Align left
      10, // Top
      &font_dejavu_sans_36,
      "No Events scheduled",
      event_line_render_font_cb
    );
    for(uint32_t i = 0; i < EVENT_LINE_HEIGHT; i++)
    {
      screen_setPixelLine(EVENT_LINE_POS_X, EVENT_LINE_POS_Y + i, EVENT_LINE_WIDTH, event_line_buffer[i]);
    }
    return;
  }
  else if(show_previous_expired_event
    && event_previous_cursor != NULL)
  {
    /* If a previous event exists, roll the cursor back one */
    event_cursor = event_previous_cursor;
  }

  /* Render event list */
  int events_displayed = 0;
  screen_pixel_t *text_colour_ptr;
  while(event_cursor != NULL && events_displayed <= 6)
  {
    /* This is an ugly thing to blank the top line when the next line is == 0, so just about to tick over */
    if(false == (
         show_previous_expired_event
      && events_displayed == 0
      && event_cursor->next != NULL
      && event_cursor->next->countdown_int == 0))
    {
      /* Get text colour from urgency */
      if(event_cursor->countdown_int > 0)
      {
        text_colour_ptr = (screen_pixel_t *)&graphics_darkgrey_pixel;
      }
      else if(event_cursor->countdown_int == 0)
      {
        text_colour_ptr = (screen_pixel_t *)&graphics_red_pixel;
      }
      else if(event_cursor->countdown_int > -60)
      {
        text_colour_ptr = (screen_pixel_t *)&graphics_amber_pixel;
      }
      else
      {
        text_colour_ptr = (screen_pixel_t *)&graphics_white_pixel;
      }

      /* Render description text */
      font_render_colour_string_with_callback(
        0, // Align left
        (events_displayed * EVENT_EACH_LINE_HEIGHT),
        &font_dejavu_sans_36, &graphics_black_pixel, text_colour_ptr,
        event_cursor->description, event_line_render_font_cb
      );

      /* Draw graphic for all events now or in the future */
      if(event_cursor->countdown_int <= 0)
      {
        if(event_cursor->type == 1)
        {
          // AoS
          event_line_graphic_transition(410, (events_displayed * EVENT_EACH_LINE_HEIGHT)+5, 30, 100, &graphics_black_pixel, &graphics_blue_pixel);
        }
        else if(event_cursor->type == 2)
        {
          // LoS
          event_line_graphic_transition(410, (events_displayed * EVENT_EACH_LINE_HEIGHT)+5, 30, 100, &graphics_blue_pixel, &graphics_black_pixel);
        }
        else if(event_cursor->type == 3)
        {
          // Uplink Start
          event_line_graphic_transition(410, (events_displayed * EVENT_EACH_LINE_HEIGHT)+5, 30, 100, &graphics_black_pixel, &graphics_yellow_pixel);
        }
        else if(event_cursor->type == 4)
        {
          // Uplink Stop
          event_line_graphic_transition(410, (events_displayed * EVENT_EACH_LINE_HEIGHT)+5, 30, 100, &graphics_yellow_pixel, &graphics_black_pixel);
        }
      }

      /* Draw countdown timer string */
      font_render_colour_string_with_callback(
        EVENT_LINE_WIDTH - font_width_string(&font_dejavu_sans_36, event_cursor->countdown_string), // Align right
        (events_displayed * EVENT_EACH_LINE_HEIGHT),
        &font_dejavu_sans_36, &graphics_black_pixel, text_colour_ptr,
        event_cursor->countdown_string, event_line_render_font_cb
      );
    }

    /* Draw separation line */
    if(events_displayed != 0)
    {
      for(int i = 2; i < (EVENT_LINE_WIDTH-2); i++)
      {
        memcpy(&event_line_buffer[(events_displayed * EVENT_EACH_LINE_HEIGHT)-3][i], &graphics_subtlegrey_pixel, sizeof(screen_pixel_t));
      }
    }

    event_cursor = event_cursor->next;
    events_displayed++;
  }

  for(uint32_t i = 0; i < EVENT_LINE_HEIGHT; i++)
  {
    screen_setPixelLine(EVENT_LINE_POS_X, EVENT_LINE_POS_Y + i, EVENT_LINE_WIDTH, event_line_buffer[i]);
  }
}
#endif

static inline void connectionstatus_render_font_cb(int x, int y, screen_pixel_t *pixel_ptr)
{
  memcpy(&(connectionstatus_buffer[y][x]), pixel_ptr, sizeof(screen_pixel_t));
}

void connectionstatus_render(char *text_string, bool status_ok)
{
  /* Clear buffer */
  for(uint32_t i = 0; i < CONNECTIONSTATUS_HEIGHT; i++)
  {
    for(uint32_t j = 0; j < CONNECTIONSTATUS_WIDTH; j++)
    {
      memcpy(&(connectionstatus_buffer[i][j]), &graphics_black_pixel, sizeof(screen_pixel_t));
    }
  }

  screen_pixel_t *text_colour_ptr;

  if(status_ok)
  {
    text_colour_ptr = (screen_pixel_t *)&graphics_darkgrey_pixel;
  }
  else
  {
    text_colour_ptr = (screen_pixel_t *)&graphics_red_pixel;
  }

  font_render_colour_string_with_callback(
    0, // Align left
    (CONNECTIONSTATUS_HEIGHT - font_dejavu_sans_16.height) / 2,
    &font_dejavu_sans_16,
    &graphics_black_pixel, text_colour_ptr,
    text_string,
    connectionstatus_render_font_cb
  );

  for(uint32_t i = 0; i < CONNECTIONSTATUS_HEIGHT; i++)
  {
    screen_setPixelLine(CONNECTIONSTATUS_POS_X, CONNECTIONSTATUS_POS_Y + i, CONNECTIONSTATUS_WIDTH, connectionstatus_buffer[i]);
  }
}

void flag_render(int32_t flag_status, bool acknowledged)
{
  screen_pixel_t *flag_colour_ptr;

  switch(flag_status)
  {
    case 0:
      flag_colour_ptr = (screen_pixel_t *)&graphics_green_pixel;
      break;
    case 1:
      flag_colour_ptr = (screen_pixel_t *)&graphics_amber_pixel;
      break;
    case 2:
      flag_colour_ptr = (screen_pixel_t *)&graphics_red_pixel;
      break;
    default:
      flag_colour_ptr = (screen_pixel_t *)&graphics_darkgrey_pixel;
      break;
  }

  if(acknowledged)
  {
    // Solid colour
    for(uint32_t i = 0; i < FLAG_HEIGHT; i++)
    {
      for(uint32_t j = 0; j < FLAG_WIDTH; j++)
      {
        memcpy(&(flag_buffer[i][j]), flag_colour_ptr, sizeof(screen_pixel_t));
      }
    }
  }
  else
  {
    // Alternate flashing panels
    screen_pixel_t *upper_colour_ptr;
    screen_pixel_t *lower_colour_ptr;
    if(timestamp_ms() % 1000 < 500)
    {
      upper_colour_ptr = flag_colour_ptr;
      lower_colour_ptr = (screen_pixel_t *)&graphics_black_pixel;
    }
    else
    {
      upper_colour_ptr = (screen_pixel_t *)&graphics_black_pixel;
      lower_colour_ptr = flag_colour_ptr;
    }

    for(uint32_t i = 0; i < FLAG_HEIGHT/2; i++)
    {
      for(uint32_t j = 0; j < FLAG_WIDTH; j++)
      {
        memcpy(&(flag_buffer[i][j]), upper_colour_ptr, sizeof(screen_pixel_t));
      }
    }

    for(uint32_t i = FLAG_HEIGHT/2; i < FLAG_HEIGHT; i++)
    {
      for(uint32_t j = 0; j < FLAG_WIDTH; j++)
      {
        memcpy(&(flag_buffer[i][j]), lower_colour_ptr, sizeof(screen_pixel_t));
      }
    }


  }



  for(uint32_t i = 0; i < FLAG_HEIGHT; i++)
  {
    screen_setPixelLine(FLAG_POS_X, FLAG_POS_Y + i, FLAG_WIDTH, flag_buffer[i]);
  }
}