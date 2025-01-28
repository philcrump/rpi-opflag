#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
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

#define SCREEN_RENDER_INTERVAL_MS   (10)

static char *screen_fb_ptr = 0;
static int screen_fb_fd = 0;

pthread_mutex_t screen_backbuffer_mutex = PTHREAD_MUTEX_INITIALIZER;
screen_pixel_t screen_backbuffer[SCREEN_PIXEL_COUNT] __attribute__ ((aligned (NEON_ALIGNMENT)));

static screen_pixel_t screen_pixel_blank_array[SCREEN_PIXEL_COUNT] __attribute__ ((aligned (NEON_ALIGNMENT))) = {
  {
    .Green = 0x00,
    .Red = 0x00,
    .Blue = 0x00,
    .Alpha = 0x80
  }
};

void screen_setPixel(int x, int y, screen_pixel_t *pixel_ptr)
{
  pthread_mutex_lock(&screen_backbuffer_mutex);

  memcpy(&screen_backbuffer[x + (y * SCREEN_SIZE_X)], (void *)pixel_ptr, sizeof(screen_pixel_t));

  pthread_mutex_unlock(&screen_backbuffer_mutex);
}

void screen_setPixelLine(int x, int y, int length, screen_pixel_t *pixel_array_ptr)
{
  pthread_mutex_lock(&screen_backbuffer_mutex);

  memcpy(&screen_backbuffer[x + (y * SCREEN_SIZE_X)], (void *)pixel_array_ptr, length * sizeof(screen_pixel_t));

  pthread_mutex_unlock(&screen_backbuffer_mutex);
}

void screen_drawHorizontalLine(int x, int y, int length, const screen_pixel_t *pixel_ptr)
{
  pthread_mutex_lock(&screen_backbuffer_mutex);

  for(int i = 0; i < length; i++)
  {
    memcpy(&screen_backbuffer[i + x + (y * SCREEN_SIZE_X)], (void *)pixel_ptr, sizeof(screen_pixel_t));
  }

  pthread_mutex_unlock(&screen_backbuffer_mutex);
}

void screen_clear(void)
{
  pthread_mutex_lock(&screen_backbuffer_mutex);
  
  memcpy(screen_backbuffer, (void *)screen_pixel_blank_array, SCREEN_PIXEL_COUNT * sizeof(screen_pixel_t));

  pthread_mutex_unlock(&screen_backbuffer_mutex);
}

void screen_forcerender(void)
{
  pthread_mutex_lock(&screen_backbuffer_mutex);

  memcpy(screen_fb_ptr, (void *)&screen_backbuffer, SCREEN_PIXEL_COUNT * sizeof(screen_pixel_t));

  pthread_mutex_unlock(&screen_backbuffer_mutex);
}

void screen_tryrender(int signum)
{
  (void)signum;

  if(0 == pthread_mutex_trylock(&screen_backbuffer_mutex))
  {
    memcpy(screen_fb_ptr, (void *)&screen_backbuffer, SCREEN_PIXEL_COUNT * sizeof(screen_pixel_t));

    pthread_mutex_unlock(&screen_backbuffer_mutex);
  }
}

static void screen_splash_font_cb(int x, int y, screen_pixel_t *pixel_ptr)
{
  memcpy(&(screen_backbuffer[x + (y * SCREEN_SIZE_X)]), pixel_ptr, sizeof(screen_pixel_t));
}

void screen_splash(char *lower_line)
{
  char *splash_string;

  int r = asprintf(&splash_string, "OpClock");
  if(r < 0)
  {
    // asprintf allocation of string failed
    return;
  }

  pthread_mutex_lock(&screen_backbuffer_mutex);

  font_render_string_with_callback(
    (SCREEN_SIZE_X- font_width_string(&font_dejavu_sans_72, splash_string)) / 2, // Align center
    100,
    &font_dejavu_sans_72,
    splash_string,
    screen_splash_font_cb
  );

  if(lower_line != NULL)
  {
    font_render_string_with_callback(
      (SCREEN_SIZE_X- font_width_string(&font_dejavu_sans_36, lower_line)) / 2, // Align center
      300,
      &font_dejavu_sans_36,
      lower_line,
      screen_splash_font_cb
    );
  }

  pthread_mutex_unlock(&screen_backbuffer_mutex);

  free(splash_string);
}

bool screen_init(bool splash_enable, char *splash_lower_line)
{ 
  screen_fb_fd = open("/dev/fb0", O_RDWR);

  if(!screen_fb_fd) 
  {
    printf("Error: cannot open framebuffer device.\n");
    return false;
  }

  screen_fb_ptr = (char*)mmap(0, SCREEN_PIXEL_COUNT * sizeof(screen_pixel_t), PROT_READ | PROT_WRITE, MAP_SHARED, screen_fb_fd, 0);
                    
  if((int64_t)screen_fb_ptr == -1)
  {
    printf("Error: Invalid framebuffer pointer.\n");
    return false;
  }

  /* Clear Screen */
  screen_clear();

  /* Run optional splash-screen function */
  if(splash_enable)
  {
    screen_splash(splash_lower_line);
  }

  screen_forcerender();

  return true;
}

void screen_deinit(void)
{
  munmap(screen_fb_ptr, SCREEN_PIXEL_COUNT * sizeof(screen_pixel_t));
  close(screen_fb_fd);
}

void *screen_thread(void *arg)
{
  bool *app_exit_ptr = (bool *)arg;

  /* Set up signal handler for timer */
  struct sigaction sa;
  memset(&sa, 0x00, sizeof(sa));
  sa.sa_handler = &screen_tryrender;
  sigaction(SIGALRM, &sa, NULL);

  /* Set up timer, queue for immediate first render */
  struct itimerval timer;
  /* Initial delay: */
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 1; // 1 microsecond ~= immediate
  /* Subsequent interval: */
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = SCREEN_RENDER_INTERVAL_MS*1000;
  setitimer(ITIMER_REAL, &timer, NULL);

  while(!*app_exit_ptr)
  {
    sleep_ms_or_signal(1000, app_exit_ptr);
  }

  /* De-initialise timer */
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 0;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL, &timer, NULL);

  return NULL;
}