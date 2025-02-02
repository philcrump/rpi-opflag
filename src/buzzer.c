#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include <lgpio.h>

#include "main.h"
#include "util/timing.h"

void *buzzer_thread(void *arg)
{
  app_state_t *app_state_ptr = (app_state_t *)arg;

  if(app_state_ptr->config.buzzer_enable == false)
  {
    pthread_exit(NULL);
  }

  int gpio_pin = app_state_ptr->config.buzzer_gpio;

  int gpio_buzzer_on = 1;
  int gpio_buzzer_off = 0;
  if(app_state_ptr->config.buzzer_active_high == false)
  {
    gpio_buzzer_on = 0;
    gpio_buzzer_off = 1;
  }

  int gpio_handle = lgGpiochipOpen(0);

  if (gpio_handle < 0)
  {
    printf("[buzzer_thread] ERROR: %s (%d)\n", lguErrorText(gpio_handle), gpio_handle);
    pthread_exit(NULL);
  }

  lgGpioWrite(gpio_handle, gpio_pin, gpio_buzzer_off);

  while(!app_state_ptr->app_exit)
  {
    if(app_state_ptr->http_ok
      && app_state_ptr->flag_severity > 0
      && !app_state_ptr->flag_acknowledged)
    {
      if(timestamp_ms() % 500 < 250)
      {
        lgGpioWrite(gpio_handle, gpio_pin, gpio_buzzer_on);
      }
      else
      {
        lgGpioWrite(gpio_handle, gpio_pin, gpio_buzzer_off);
      }
    }
    else
    {
      lgGpioWrite(gpio_handle, gpio_pin, gpio_buzzer_off);
    }
    sleep_ms_or_signal(50, &(app_state_ptr->app_exit));
  }

  lgGpioWrite(gpio_handle, gpio_pin, gpio_buzzer_off);

  lgGpiochipClose(0);

  pthread_exit(NULL);
}
