#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "main.h"
#include "backlight.h"
#include "touch.h"
#include "clock.h"
#include "screen.h"
#include "graphics.h"
#include "events.h"
#include "util/ini.h"
#include "util/timing.h"

#define CONFIG_PATH "config.ini"

/* From config.c */
int config_handler(void* user, const char* section, const char* name, const char* value);

static app_state_t app_state = {
  .events = NULL,
  .events_mutex = PTHREAD_MUTEX_INITIALIZER,

  .events_source_ok = false,

  .flag_acknowledged = false,

  .config = {
    .backlight_level = 180,
    .display_show_previous_expired_event = false
  },
  .app_exit = false
};

void sigint_handler(int sig)
{
    (void)sig;
    app_state.app_exit = true;
}

void _print_usage(void)
{
    printf(
        "\n"
        "Usage: opclock [options]\n"
    );
}

static pthread_t screen_thread_obj;
static pthread_t touch_thread_obj;
static pthread_t clock_thread_obj;
static pthread_t events_source_thread_obj;

int main(int argc, char* argv[])
{
  (void) argc;
  (void) argv;

  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigint_handler);

  printf("==============================\n");
  printf("OpFlag - Phil Crump M0DNY\n");
  printf("* version: " BUILD_VERSION "\n");
  printf("* build:   " BUILD_DATE "\n");
  printf("------------------------------\n");

  if(ini_parse(CONFIG_PATH, config_handler, &app_state.config) < 0)
  {
    fprintf(stderr, "Error loading " CONFIG_PATH ", aborting!\n");
    return 1;
  }

  printf(" * Backlight Level: %d\n", app_state.config.backlight_level);

  printf("------------------------------\n");

  /* Switch on screen */
  backlight_power(true);

  /* Set to configured backlight level */
  backlight_level(app_state.config.backlight_level);

  /* Initialise screen and splash */
  if(!screen_init(false, NULL))
  {
    fprintf(stderr, "Error initialising screen!\n");
    return 1;
  }

  /* Screen Render (backbuffer -> screen) Thread */
  if(pthread_create(&screen_thread_obj, NULL, screen_thread, &app_state.app_exit))
  {
      fprintf(stderr, "Error creating %s pthread\n", "Screen");
      return 1;
  }
  pthread_setname_np(screen_thread_obj, "Screen");

  /* Touch (screen on/off) Thread */
  if(pthread_create(&touch_thread_obj, NULL, touch_thread, &app_state))
  {
      fprintf(stderr, "Error creating %s pthread\n", "Touch");
      return 1;
  }
  pthread_setname_np(touch_thread_obj, "Touch");

#if 0
  /* Clock (time/date -> backbuffer) Thread */
  if(pthread_create(&clock_thread_obj, NULL, clock_thread, &app_state))
  {
      fprintf(stderr, "Error creating %s pthread\n", "Clock");
      return 1;
  }
  pthread_setname_np(clock_thread_obj, "Clock");
#endif

#if 0
  /* Event Source Thread */
  if(pthread_create(&events_source_thread_obj, NULL, events_http_thread, &app_state))
  {
      fprintf(stderr, "Error creating %s pthread\n", "Events HTTP");
      return 1;
  }
  pthread_setname_np(events_source_thread_obj, "Events HTTP");
#endif

  while(!app_state.app_exit)
  {
    connectionstatus_render("Connection: Failure", false);

    flag_render(2, app_state.flag_acknowledged);

    sleep_ms(100);
  }

  printf("Got SIGTERM/INT..\n");
  app_state.app_exit = true;

  pthread_kill(events_source_thread_obj, SIGINT);
  pthread_kill(clock_thread_obj, SIGINT);
  pthread_kill(touch_thread_obj, SIGINT);
  pthread_kill(screen_thread_obj, SIGINT);

  pthread_join(events_source_thread_obj, NULL);
  pthread_join(clock_thread_obj, NULL);
  pthread_join(touch_thread_obj, NULL);
  pthread_join(screen_thread_obj, NULL);

  screen_deinit();

  printf("All threads caught, exiting..\n");
}
