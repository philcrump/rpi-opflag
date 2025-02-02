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


/** DATE **/

#define DATE_WIDTH      350
#define DATE_HEIGHT     55
static screen_pixel_t date_buffer[DATE_HEIGHT][DATE_WIDTH] __attribute__ ((aligned (NEON_ALIGNMENT))) = { 0 };

#define DATE_POS_X    15
#define DATE_POS_Y    5

/** Alarms **/

#define ALARM_LINE_WIDTH      370
#define ALARM_LINE_HEIGHT     300
#define ALARM_EACH_LINE_HEIGHT  30
static screen_pixel_t alarm_line_buffer[ALARM_LINE_HEIGHT][ALARM_LINE_WIDTH] __attribute__ ((aligned (NEON_ALIGNMENT))) = { 0 };

#define ALARM_LINE_POS_X    15
#define ALARM_LINE_POS_Y    65

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

const screen_pixel_t graphics_purple_pixel =
{
  .Alpha = 0x80,
  .Red = 0xAA,
  .Green = 0x44,
  .Blue = 0xFF
};

static inline void datetime_render_font_cb(int x, int y, screen_pixel_t *pixel_ptr)
{
  memcpy(&(date_buffer[y][x]), pixel_ptr, sizeof(screen_pixel_t));
}

void datetime_render(char *datetime_string)
{
  int32_t i, j;

  /* Clear buffer */
  for(i = 0; i < DATE_HEIGHT; i++)
  {
    for(j = 0; j <DATE_WIDTH; j++)
    {
      memcpy(&(date_buffer[i][j]), &graphics_black_pixel, sizeof(screen_pixel_t));
    }
  }

  font_render_string_with_callback(
    0, // Align left
    (DATE_HEIGHT - font_dejavu_sans_36.height) / 2,
    &font_dejavu_sans_36,
    datetime_string,
    datetime_render_font_cb
  );

  for(i = 0; i < DATE_HEIGHT; i++)
  {
    screen_setPixelLine(DATE_POS_X, DATE_POS_Y + i, DATE_WIDTH, date_buffer[i]);
  }
}

static inline void alarm_line_render_font_cb(int x, int y, screen_pixel_t *pixel_ptr)
{
  memcpy(&(alarm_line_buffer[y][x]), pixel_ptr, sizeof(screen_pixel_t));
}

void alarms_render(bool http_ok, alarm_t *alarms_ptr)
{
  int32_t i, j;

  /* Clear all lines buffer */
  for(i = 0; i < ALARM_LINE_HEIGHT; i++)
  {
    for(j = 0; j < ALARM_LINE_WIDTH; j++)
    {
      memcpy(&(alarm_line_buffer[i][j]), &graphics_black_pixel, sizeof(screen_pixel_t));
    }
  }

  alarm_t *alarm_cursor = alarms_ptr;

  if(http_ok == false)
  {
    /* Connection failed, render blank */

    for(i = 0; i < ALARM_LINE_HEIGHT; i++)
    {
      screen_setPixelLine(ALARM_LINE_POS_X, ALARM_LINE_POS_Y + i, ALARM_LINE_WIDTH, alarm_line_buffer[i]);
    }
    return;
  }

  if(alarm_cursor == NULL)
  {
    /* No alarms */

    /* Draw message, render early, and exit */
    font_render_colour_string_with_callback(
      0, // Align left
      0, // Top
      &font_dejavu_sans_16,
      &graphics_black_pixel, &graphics_green_pixel,
      "No Alarms",
      alarm_line_render_font_cb
    );

    for(i = 0; i < ALARM_LINE_HEIGHT; i++)
    {
      screen_setPixelLine(ALARM_LINE_POS_X, ALARM_LINE_POS_Y + i, ALARM_LINE_WIDTH, alarm_line_buffer[i]);
    }
    return;
  }

  /* Render event list */
  int alarms_displayed = 0;
  screen_pixel_t *text_colour_ptr;
  char alarm_display_description[46];
  while(alarm_cursor != NULL && alarms_displayed <= 10)
  {
    /* Get text colour from severity */
    if(alarm_cursor->severity == 1) // Warning
    {
      text_colour_ptr = (screen_pixel_t *)&graphics_amber_pixel;
    }
    else if(alarm_cursor->severity == 2) // Critical
    {
      text_colour_ptr = (screen_pixel_t *)&graphics_red_pixel;
    }
    else // 3 = Unknown
    {
      text_colour_ptr = (screen_pixel_t *)&graphics_purple_pixel;
    }

    /* Limit description length */
    snprintf(alarm_display_description, sizeof(alarm_display_description)-1, "%s", alarm_cursor->description);

    /* Render description text */
    font_render_colour_string_with_callback(
      0, // Align left
      (alarms_displayed * ALARM_EACH_LINE_HEIGHT),
      &font_dejavu_sans_16, &graphics_black_pixel, text_colour_ptr,
      alarm_display_description, alarm_line_render_font_cb
    );

    alarm_cursor = alarm_cursor->next;
    alarms_displayed++;
  }

  for(i = 0; i < ALARM_LINE_HEIGHT; i++)
  {
    screen_setPixelLine(ALARM_LINE_POS_X, ALARM_LINE_POS_Y + i, ALARM_LINE_WIDTH, alarm_line_buffer[i]);
  }
}

static inline void connectionstatus_render_font_cb(int x, int y, screen_pixel_t *pixel_ptr)
{
  memcpy(&(connectionstatus_buffer[y][x]), pixel_ptr, sizeof(screen_pixel_t));
}

void connectionstatus_render(char *text_string, bool status_ok)
{
  int32_t i, j;

  /* Clear buffer */
  for(i = 0; i < CONNECTIONSTATUS_HEIGHT; i++)
  {
    for(j = 0; j < CONNECTIONSTATUS_WIDTH; j++)
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

  for(i = 0; i < CONNECTIONSTATUS_HEIGHT; i++)
  {
    screen_setPixelLine(CONNECTIONSTATUS_POS_X, CONNECTIONSTATUS_POS_Y + i, CONNECTIONSTATUS_WIDTH, connectionstatus_buffer[i]);
  }
}

void flag_render(int32_t flag_status, bool acknowledged)
{
  int32_t i, j;
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
    case -1:
    default:
      flag_colour_ptr = (screen_pixel_t *)&graphics_darkgrey_pixel;
      break;
  }

  if (flag_status == -1)
  {
    // Striped grey
    for(i = 0; i < FLAG_HEIGHT; i++)
    {
      for(j = 0; j < FLAG_WIDTH; j++)
      {
        //if(((float)(j)/FLAG_WIDTH) < ((float)(FLAG_HEIGHT-i)/FLAG_HEIGHT))
        if(((i+j) % 200) > 100)
        {
          memcpy(&(flag_buffer[i][j]), &graphics_darkgrey_pixel, sizeof(screen_pixel_t));
        }
        else
        {
          memcpy(&(flag_buffer[i][j]), &graphics_subtlegrey_pixel, sizeof(screen_pixel_t));
        }
      }
    }
  }
  else if(flag_status == 0 || acknowledged)
  {
    // Solid colour
    for(i = 0; i < FLAG_HEIGHT; i++)
    {
      for(j = 0; j < FLAG_WIDTH; j++)
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

    for(i = 0; i < FLAG_HEIGHT/2; i++)
    {
      for(j = 0; j < FLAG_WIDTH; j++)
      {
        memcpy(&(flag_buffer[i][j]), upper_colour_ptr, sizeof(screen_pixel_t));
      }
    }

    for(i = FLAG_HEIGHT/2; i < FLAG_HEIGHT; i++)
    {
      for(j = 0; j < FLAG_WIDTH; j++)
      {
        memcpy(&(flag_buffer[i][j]), lower_colour_ptr, sizeof(screen_pixel_t));
      }
    }
  }

  for(i = 0; i < FLAG_HEIGHT; i++)
  {
    screen_setPixelLine(FLAG_POS_X, FLAG_POS_Y + i, FLAG_WIDTH, flag_buffer[i]);
  }
}
