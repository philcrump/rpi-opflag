#include <stdlib.h>
#include <string.h>

#include "main.h"

int config_handler(void* user, const char* section, const char* name, const char* value)
{
  #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  
  app_config_t *app_config_ptr = (app_config_t *)user;

  if      (MATCH("backlight", "level"))
  {
    app_config_ptr->backlight_level = atoi(value);
  }
  else if (MATCH("http", "url"))
  {
    app_config_ptr->http_url = strdup(value);
  }
  else if (MATCH("http", "username"))
  {
    app_config_ptr->http_username = strdup(value);
  }
  else if (MATCH("http", "password"))
  {
    app_config_ptr->http_password = strdup(value);
  }
  else if (MATCH("acknowledge", "single_touch"))
  {
    if(strcasecmp("true", value) == 0 || strcasecmp("yes", value) == 0)
    {
      app_config_ptr->acknowledge_single_touch = true;
    }
    else // if(strcasecmp("false", value) == 0 || strcasecmp("no", value) == 0)
    {
      app_config_ptr->acknowledge_single_touch = false;
    }
  }
  else if (MATCH("acknowledge", "double_touch"))
  {
    if(strcasecmp("true", value) == 0 || strcasecmp("yes", value) == 0)
    {
      app_config_ptr->acknowledge_double_touch = true;
    }
    else // if(strcasecmp("false", value) == 0 || strcasecmp("no", value) == 0)
    {
      app_config_ptr->acknowledge_double_touch = false;
    }
  }
  else if (MATCH("buzzer", "enable"))
  {
    if(strcasecmp("true", value) == 0 || strcasecmp("yes", value) == 0)
    {
      app_config_ptr->buzzer_enable = true;
    }
    else // if(strcasecmp("false", value) == 0 || strcasecmp("no", value) == 0)
    {
      app_config_ptr->buzzer_enable = false;
    }
  }
  else if (MATCH("buzzer", "gpio"))
  {
    app_config_ptr->buzzer_gpio = atoi(value);
  }
  else if (MATCH("buzzer", "active_high"))
  {
    if(strcasecmp("true", value) == 0 || strcasecmp("yes", value) == 0)
    {
      app_config_ptr->buzzer_active_high = true;
    }
    else // if(strcasecmp("false", value) == 0 || strcasecmp("no", value) == 0)
    {
      app_config_ptr->buzzer_active_high = false;
    }
  }
  else
  {
      return 0;  /* unknown section/name, error */
  }
  return 1;
}