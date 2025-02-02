#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <asm/errno.h>
#include <errno.h>
#include <pthread.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <json-c/json.h>

#include "main.h"
#include "alarms.h"
#include "util/timing.h"

#define HTTP_UPDATE_INTERVAL_S  (1)
#define HTTP_RETRY_PERIOD_S     (3)

#define BUF_SZ                  (1024*1024)

static char curl_data[BUF_SZ];
static int32_t curl_data_written = 0;

static int curl_receive(void* buffer, size_t length, size_t size, void* data)
{
  size_t l = length * size;
  if(l > 0)
  {
    if(curl_data_written + l >= BUF_SZ)
    {
      fprintf(stderr, "[alarms-http] curl buffer size exceeded.\n");
      return 0;
    }
    memcpy(&((char*)data)[curl_data_written], buffer, l);
    curl_data_written += l;
  }
  return l;
}

void *alarms_http_thread(void *arg)
{
  app_state_t *app_state_ptr = (app_state_t *)arg;

  int r;
  struct json_object *packet_obj, *alarm_obj;
  struct json_object *alarm_hostname_obj, *alarm_servicename_obj, *alarm_output_obj, *alarm_state_obj, *alarm_laststatechange_obj;
  char *alarm_hostname, *alarm_servicename, *alarm_output;
  int alarm_state, alarm_laststatechange;
  int max_alarm_laststatechange = 0;
  int max_alarm_severity = 0;

  char alarm_description[64];
  alarm_t *alarms_downloaded_list = NULL;
  alarm_t *alarms_old_ptr;

  /* Check URL is valid */
  if(app_state_ptr->config.http_url == NULL
    || strlen(app_state_ptr->config.http_url) < 1)
  {
    fprintf(stderr, "[alarms-http] HTTP URL is not valid\n");
    app_state_ptr->http_ok = false;
    return NULL;
  }

  curl_global_init(CURL_GLOBAL_NOTHING);
  CURL *c;

  while(!app_state_ptr->app_exit)
  {
    /* Clear data and data cursor */
    memset(curl_data, 0x00, BUF_SZ);
    curl_data_written = 0;

    max_alarm_laststatechange = 0;
    max_alarm_severity = 0;

    /* Initialise curl */
    c = curl_easy_init();
    if(c == NULL)
    {
      sleep_ms_or_signal(100, &app_state_ptr->app_exit);
      app_state_ptr->http_ok = false;
      continue;
    }

    curl_easy_setopt(c, CURLOPT_URL, app_state_ptr->config.http_url);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curl_receive);
    curl_easy_setopt(c, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(c, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, curl_data);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 5);

    /* Authentication */
    curl_easy_setopt(c, CURLOPT_LOGIN_OPTIONS, "AUTH=*");
    curl_easy_setopt(c, CURLOPT_USERNAME, app_state_ptr->config.http_username);
    curl_easy_setopt(c, CURLOPT_PASSWORD, app_state_ptr->config.http_password);

    /* Don't validate SSL certificate */
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYSTATUS, 0);

    struct curl_slist *curl_headers = NULL;
    curl_headers = curl_slist_append(curl_headers, "Accept: application/json");
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, curl_headers);

    /* Perform the request */
    r = curl_easy_perform(c);
    curl_easy_cleanup(c);

    if(r != 0)
    {
      fprintf(stderr, "[alarms-http] curl error: %s (%d)\n", curl_easy_strerror(r), r);
      app_state_ptr->http_ok = false;
      sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
      continue;
    }

    packet_obj = json_tokener_parse(curl_data);
    if(packet_obj == NULL)
    {
      fprintf(stderr, "[alarms-http] Error: Failed to parse JSON\n");
      app_state_ptr->http_ok = false;
      sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
      continue;
    }

    for (size_t i = 0; i < json_object_array_length(packet_obj); i++)
    {
      alarm_obj = json_object_array_get_idx(packet_obj, i);

      if(!json_object_object_get_ex(alarm_obj, "host_display_name", &alarm_hostname_obj))
      {
        fprintf(stderr, "[alarms-http] Error: alarm doesn't contain host_display_name\n");
        //goto discard_json;
        app_state_ptr->http_ok = false;
        sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
        continue;
      }
      alarm_hostname = strdup(json_object_get_string(alarm_hostname_obj));

      if(!json_object_object_get_ex(alarm_obj, "service_display_name", &alarm_servicename_obj))
      {
        fprintf(stderr, "[alarms-http] Error: alarm doesn't contain service_display_name\n");
        //goto discard_json;
        app_state_ptr->http_ok = false;
        sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
        continue;
      }
      alarm_servicename = strdup(json_object_get_string(alarm_servicename_obj));

      if(!json_object_object_get_ex(alarm_obj, "service_output", &alarm_output_obj))
      {
        fprintf(stderr, "[alarms-http] Error: alarm doesn't contain service_output\n");
        //goto discard_json;
        app_state_ptr->http_ok = false;
        sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
        continue;
      }
      alarm_output = strdup(json_object_get_string(alarm_output_obj));

      if(!json_object_object_get_ex(alarm_obj, "service_state", &alarm_state_obj))
      {
        fprintf(stderr, "[alarms-http] Error: alarm doesn't contain service_state\n");
        //goto discard_json;
        app_state_ptr->http_ok = false;
        sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
        continue;
      }
      alarm_state = json_object_get_int(alarm_state_obj);

      if(!json_object_object_get_ex(alarm_obj, "service_last_state_change", &alarm_laststatechange_obj))
      {
        fprintf(stderr, "[alarms-http] Error: alarm doesn't contain service_last_state_change\n");
        //goto discard_json;
        app_state_ptr->http_ok = false;
        sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
        continue;
      }
      alarm_laststatechange = json_object_get_int(alarm_laststatechange_obj);

      snprintf(alarm_description, sizeof(alarm_description)-1,
        "%s - %s - %s",
        alarm_hostname,
        alarm_servicename,
        alarm_output
      );

      if(alarm_state == 1 || alarm_state == 2)
      {
        alarms_append(&alarms_downloaded_list, alarm_description, alarm_state);
        if(alarm_laststatechange > max_alarm_laststatechange)
        {
          max_alarm_laststatechange = alarm_laststatechange;
        }
        if(alarm_state > max_alarm_severity)
        {
          max_alarm_severity = alarm_state;
        }
      }

    }

    pthread_mutex_lock(&app_state_ptr->alarms_mutex);

    alarms_old_ptr = app_state_ptr->alarms;
    app_state_ptr->alarms = alarms_downloaded_list;
    alarms_delete(alarms_old_ptr);
    alarms_downloaded_list = NULL;

    app_state_ptr->flag_severity = max_alarm_severity;
    if(max_alarm_laststatechange > app_state_ptr->flag_laststatechange)
    {
      app_state_ptr->flag_acknowledged = false;
      app_state_ptr->flag_laststatechange = max_alarm_laststatechange;
    }

    pthread_mutex_unlock(&app_state_ptr->alarms_mutex);

    app_state_ptr->http_ok = true;

    sleep_ms_or_signal(HTTP_UPDATE_INTERVAL_S * 1000, &app_state_ptr->app_exit);
  }

  curl_global_cleanup();

  pthread_exit(NULL);
}
