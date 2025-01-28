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
  else if (MATCH("events", "file_filepath"))
  {
    app_config_ptr->event_source_file_filepath = strdup(value);
  }
  else if (MATCH("events", "http_url"))
  {
    app_config_ptr->event_source_http_url = strdup(value);
  }
  else if (MATCH("display", "show_last_expired_event"))
  {
    if(strcasecmp("true", value) == 0 || strcasecmp("yes", value) == 0)
    {
      app_config_ptr->display_show_previous_expired_event = true;
    }
    else // if(strcasecmp("false", value) == 0 || strcasecmp("no", value) == 0)
    {
      app_config_ptr->display_show_previous_expired_event = false;
    }
  }
  else
  {
      return 0;  /* unknown section/name, error */
  }
  return 1;
}