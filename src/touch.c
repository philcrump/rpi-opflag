#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <linux/input.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include "main.h"
#include "util/timing.h"
#include "backlight.h"

static bool touch_detecthw(char **touchscreen_path_ptr)
{
  FILE *fp;
  char *line;
  size_t len;
  char handler_device[64];
  
  bool found_name;

  found_name = false;

  fp=fopen("/proc/bus/input/devices","r");

  while(getline(&line,&len,fp) != -1)
  {
    /* Look for Name */
    if(strncmp("N:", line, 2) == 0)
    {
      if(strstr(line,"raspberrypi-ts") != NULL)
      {
        found_name = true;
      }
      else
      {
        found_name = false;
      }
    }
    /* Look for Handler */
    else if(found_name && strncmp("H:", line, 2) == 0 && strstr(line,"mouse") != NULL)
    {
      sscanf(strstr(line,"event"), "%s", handler_device);
      int r = asprintf(touchscreen_path_ptr, "/dev/input/%s", handler_device);
      fclose(fp);
      free(line);
      if(r >= 0)
      {
        return true;
      }
      else
      {
        return false;
      }
    }   
  }

  /* Failed to find touchscreen device */
  fclose(fp);

  if(line != NULL)
  {
    free(line);
  }

  return false;
}

#define TOUCH_EVENT_START 1
#define TOUCH_EVENT_END   2
#define TOUCH_EVENT_MOVE   3

//Returns 0 if no touch available. 1 if Touch start. 2 if touch end. 3 if touch move
static void touch_readEvents(int touch_fd, void (*touch_callback)(int type, int x, int y, app_state_t *app_state_ptr), app_state_t *app_state_ptr)
{
  size_t i, rb;
  struct input_event ev[64];
  int retval;
  int touch_type = -1, touch_x = 0, touch_y = 0;

  fd_set touch_fdset;
  struct timeval timeout_tv;

  while(!app_state_ptr->app_exit)
  {
    FD_ZERO(&touch_fdset);
    FD_SET(touch_fd, &touch_fdset);

    timeout_tv.tv_sec = 0;
    timeout_tv.tv_usec = 50*1000;

    retval = select(touch_fd+1 , &touch_fdset, NULL, NULL, &timeout_tv);
    if(retval < 0 && !app_state_ptr->app_exit)
    {
        fprintf(stderr,"[touch] Error: Select failed (%s)\n", strerror(errno));
    }
    else if(FD_ISSET(touch_fd, &touch_fdset))
    {
      rb=read(touch_fd, ev, sizeof(struct input_event)*64);

      for(i = 0;  i <  (rb / sizeof(struct input_event)); i++)
      {
        if(ev[i].type == EV_SYN) 
        {
          if(touch_type == TOUCH_EVENT_START)
          {
            /* This concludes a START event, [START, X, Y, MOVE] */
            touch_callback(touch_type, touch_x, touch_y, app_state_ptr);
          }
          else if(touch_type == TOUCH_EVENT_END)
          {
            /* This concludes a END event, [END, MOVE] */
            touch_callback(touch_type, 0, 0, app_state_ptr);
          }
          else
          {
            /* Concludes a MOVE EVENT [<X>, <Y>, MOVE] */
            touch_type = TOUCH_EVENT_MOVE;
            touch_callback(touch_type, touch_x, touch_y, app_state_ptr);
          }
        }
        else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 1)
        {
          touch_type = TOUCH_EVENT_START;
        }
        else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 0)
        {
          touch_type = TOUCH_EVENT_END;
        }
        else if (ev[i].type == EV_ABS && ev[i].code == 0 && ev[i].value > 0)
        {
          touch_x = ev[i].value;
        }
        else if (ev[i].type == EV_ABS  && ev[i].code == 1 && ev[i].value > 0)
        {
          touch_y = ev[i].value;
        }
      }
    }
  }
}

#define INOTIFY_FD_BUFFER_LENGTH        (64 * (sizeof(struct inotify_event) + NAME_MAX + 1))

static void touch_run(char *touch_path, void (*touch_callback)(int type, int x, int y, app_state_t *app_state_ptr), app_state_t *app_state_ptr)
{
  int inotify_fd, r;
  int touch_fd;

  /* Check that file_path exists */
  struct stat file_st;
  if(stat(touch_path, &file_st) < 0)
  {
    fprintf(stderr, "inotify: File path does not exist: %s\n", touch_path);
    return;
  }

  /* Set up inotify */
  inotify_fd = inotify_init();
  if(inotify_fd == -1)
  {
    fprintf(stderr, "inotify: inotify_init() returned error: %s\n", strerror(inotify_fd));
    return;
  }

  /* Start watching file for changes */
  r = inotify_add_watch(inotify_fd, touch_path, IN_ACCESS);
  if(r < 0)
  {
    fprintf(stderr, "inotify: inotify_add_watch() returned error: %s\n", strerror(r));
    return;
  }

  /* Open touch file for data read */
  touch_fd = open(touch_path, O_RDONLY);
  if(touch_fd < 0)
  {
    fprintf(stderr, "inotify: open() touch file returned error: %s\n", strerror(touch_fd));
    return;
  }

  char *p;
  ssize_t pending_length;
  struct inotify_event *event;
  char buf[INOTIFY_FD_BUFFER_LENGTH] __attribute__ ((aligned(8)));

  fd_set inotify_fdset;
  struct timeval timeout_tv;

  while (!app_state_ptr->app_exit)
  {
    FD_ZERO(&inotify_fdset);
    FD_SET(inotify_fd, &inotify_fdset);

    timeout_tv.tv_sec = 0;
    timeout_tv.tv_usec = 50*1000;

    r = select(inotify_fd+1 , &inotify_fdset, NULL, NULL, &timeout_tv);
    if(r < 0)
    {
        fprintf(stderr,"[touch] Error: Select failed (%s)\n", strerror(errno));
    }
    else if(FD_ISSET(inotify_fd, &inotify_fdset))
    {
      /* Wait for new events */
      pending_length = read(inotify_fd, buf, INOTIFY_FD_BUFFER_LENGTH);
      if (pending_length <= 0)
      {
        continue;
      }

      /* Process buffer of new events */
      for (p = buf; p < buf + pending_length; )
      {
        event = (struct inotify_event *) p;

        /* Read data from touch file, and pass touch event to supplied callback */
        touch_readEvents(touch_fd, touch_callback, app_state_ptr);

        /* Iterate onwards */
        p += sizeof(struct inotify_event) + event->len;
      }
    }
  }

  close(touch_fd);
  close(inotify_fd);
}

#if 0
#define BUTTON_X_POS  10
#define BUTTON_Y_POS  10
#define BUTTON_WIDTH  200
#define BUTTON_HEIGHT  100

#define areaTouched(ax, aw, ay, ah)     ((touch_x > ax) && (touch_x < (ax + aw)) && (touch_y > ay) && (touch_y < (ay + ah)))
#define xTouched(ax, aw)                ((touch_x > ax) && (touch_x < (ax + aw)))
#endif

static bool touch_backlight_ison = true;
static uint64_t last_touch_start_time = 0;
static uint64_t last_firsttouch_start_time = 0;

static void touch_process(int touch_type, int touch_x, int touch_y, app_state_t *app_state_ptr)
{
  (void)touch_x;
  (void)touch_y;

  if(touch_type == TOUCH_EVENT_START)
  {
    uint64_t current_time = monotonic_ms();

    /* A repeat touch within 20ms is assumed to be a glitch */
    if(current_time < (last_touch_start_time+20))
    {
      return;
    }

    if(touch_backlight_ison)
    {
      if(current_time < (last_firsttouch_start_time+500))
      {
        /* Double-tap ! */
        app_state_ptr->flag_acknowledged = true;
      }
      else
      {
        last_firsttouch_start_time = current_time;
      }
    }

    last_touch_start_time = current_time;

#if 0
    if(areaTouched(BUTTON_X_POS, BUTTON_WIDTH, BUTTON_Y_POS, BUTTON_HEIGHT))
    {
        button_pressed = true;
    }
#endif
  }
#if 0
  else if(touch_type == TOUCH_EVENT_END)
  {
    button_pressed = false;
  }
#endif
}

void *touch_thread(void *arg)
{
  app_state_t *app_state_ptr = (app_state_t *)arg;

  char *touchscreen_path;

  if(!touch_detecthw(&touchscreen_path))
  {
    fprintf(stderr, "Error initialising touch!\n");
    return NULL;
  }
  
  touch_run(touchscreen_path, &touch_process, app_state_ptr);

  return NULL;
}