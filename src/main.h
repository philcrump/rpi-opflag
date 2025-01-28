#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

/* Only supporting the Official 7" touchscreen */
#define SCREEN_SIZE_X   800
#define SCREEN_SIZE_Y   480
#define SCREEN_PIXEL_COUNT  (SCREEN_SIZE_X*SCREEN_SIZE_Y)

#define NEON_ALIGNMENT (4*4*2) // 128b

typedef struct eventt {
  char *description;
  time_t event_time;
  int countdown_int;
  char *countdown_string;
  int type; // 0=None, 1=AoS, 2=Los, 3=Uplink Start, 4=Uplink Stop
  struct eventt *next;
} event_t;

typedef struct {
  int32_t backlight_level;

  char *event_source_file_filepath;
  char *event_source_http_url;

  bool display_show_previous_expired_event;

} app_config_t;

typedef struct {
  event_t *events;
  pthread_mutex_t events_mutex;
  bool events_source_ok;

  bool flag_acknowledged;

  app_config_t config;
  bool app_exit;
} app_state_t;

#endif /* __MAIN_H__ */