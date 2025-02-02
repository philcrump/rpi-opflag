#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>

/* Only supporting the Official 7" touchscreen */
#define SCREEN_SIZE_X   800
#define SCREEN_SIZE_Y   480
#define SCREEN_PIXEL_COUNT  (SCREEN_SIZE_X*SCREEN_SIZE_Y)

#define NEON_ALIGNMENT (4*4*2) // 128b

typedef struct alarm_t {
  char *description;
  time_t alarm_time;
  int severity; // 0=None, 1=AoS, 2=Los, 3=Uplink Start, 4=Uplink Stop
  struct alarm_t *next;
} alarm_t;

typedef struct {
  int32_t backlight_level;

  char *http_url;
  char *http_username;
  char *http_password;

  bool acknowledge_single_touch;
  bool acknowledge_double_touch;

  bool buzzer_enable;
  int32_t buzzer_gpio;
  bool buzzer_active_high;
} app_config_t;

typedef struct {
  alarm_t *alarms;
  pthread_mutex_t alarms_mutex;

  atomic_bool http_ok;

  atomic_bool flag_acknowledged;
  atomic_int flag_severity;
  atomic_int flag_laststatechange;

  app_config_t config;
  atomic_bool app_exit;
} app_state_t;

#endif /* __MAIN_H__ */