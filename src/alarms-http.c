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

#include "main.h"
#include "alarms.h"
#include "util/timing.h"
#include "util/json.h"

#define HTTP_UPDATE_INTERVAL_S  (30)
#define HTTP_RETRY_PERIOD_S     (10)

#define BUF_SZ 1024*1024

static char curl_data[ BUF_SZ ];
static int32_t curl_data_written = 0;

static int curl_receive(void* buffer, size_t length, size_t size, void* data)
{
  size_t l = length * size;

  if(l > 0)
  {
    if(curl_data_written + l >= BUF_SZ)
    {
      fprintf(stderr, "[events-http] curl buffer size exceeded.\n");
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
  JsonNode *json_node, *event_node, *var_node;
  char json_errormsg[256];

  time_t time_now;
  char *event_description;
  time_t event_time_unix;
  int event_type;

  event_t *events_downloaded_list;
  event_t *events_old_ptr;

  /* Check URL is valid */
  if(app_state_ptr->config.event_source_http_url == NULL
    || strlen(app_state_ptr->config.event_source_http_url) < 1)
  {
    fprintf(stderr, "[events-http] HTTP URL is not valid\n");
    app_state_ptr->events_source_ok = false;
    return NULL;
  }

  curl_global_init(CURL_GLOBAL_NOTHING);
  CURL *c;

  while(!app_state_ptr->app_exit)
  {
    /* Clear data and data cursor */
    memset(curl_data, 0x00, BUF_SZ);
    curl_data_written = 0;

    /* Initialise curl */
    c = curl_easy_init();
    if(c == NULL)
    {
      sleep_ms_or_signal(100, &app_state_ptr->app_exit);
      app_state_ptr->events_source_ok = false;
      continue;
    }

    curl_easy_setopt(c, CURLOPT_URL, app_state_ptr->config.event_source_http_url);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curl_receive);
    curl_easy_setopt(c, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(c, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, curl_data);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 5);

    /* Don't validate SSL certificate */
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYSTATUS, 0);

    /* Perform the request */
    r = curl_easy_perform(c);
    curl_easy_cleanup(c);

    if(r != 0)
    {
      fprintf(stderr, "[events-http] curl error: %s (%d)\n", curl_easy_strerror(r), r);
      app_state_ptr->events_source_ok = false;
      sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
      continue;
    }

    if(json_validate(curl_data) != true)
    {
      fprintf(stderr, "[events-http] JSON Validation Error\n");
      app_state_ptr->events_source_ok = false;
      sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
      continue;
    }

    json_node = json_decode(curl_data);
    if(json_node == NULL)
    {
      fprintf(stderr, "[events-http] JSON Decode Error\n");
      app_state_ptr->events_source_ok = false;
      sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
      continue;
    }

    if(json_check(json_node, json_errormsg) == false)
    {
      fprintf(stderr, "[events-http] JSON Tree Error: %s\n", json_errormsg);
      app_state_ptr->events_source_ok = false;
      json_delete(json_node);
      sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
      continue;
    }

    time(&time_now);

    /* Loop through array for each event */
    json_foreach(event_node, json_node)
    {
      /* Description */
      var_node = json_find_member(event_node, "description");
      if(var_node == NULL)
      {
        fprintf(stderr, "[events-http] JSON - description not found\n");
        app_state_ptr->events_source_ok = false;
        json_delete(json_node);
        json_node = NULL;
        break;
      }
      if(var_node->tag != JSON_STRING)
      {
        fprintf(stderr, "[events-http] JSON - description is not a string\n");
        app_state_ptr->events_source_ok = false;
        json_delete(json_node);
        json_node = NULL;
        break;
      }
      event_description = strdup(var_node->string_);

      /* Auto Description (optional) */
      var_node = json_find_member(event_node, "autodescription");
      if(var_node != NULL && var_node->tag == JSON_STRING)
      {
        free(event_description);
        event_description = strdup(var_node->string_);
      }

      /* Target (optional) */
      var_node = json_find_member(event_node, "target");
      if(var_node != NULL && var_node->tag == JSON_STRING)
      {
        char *_event_description = event_description;

        if(asprintf(&event_description, "%s - %s", var_node->string_, _event_description) < 0)
        {
          fprintf(stderr, "[events-http] JSON - target failed to add to description string\n");
          app_state_ptr->events_source_ok = false;
          json_delete(json_node);
          json_node = NULL;
          break;
        }

        free(_event_description);
      }

      /* Catch errors from loop */
      if(json_node == NULL)
      {
        sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
        continue;
      }

      /* Type */
#if 0
      var_node = json_find_member(event_node, "type");
      if(var_node == NULL || var_node->tag != JSON_NUMBER)
      {
        fprintf(stderr, "[events-http] JSON - type not found\n");
        app_state_ptr->events_source_ok = false;
        json_delete(json_node);
        sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
        continue;
      }
      event_type = (int)var_node->number_;
#else
      event_type = (int)0;
#endif
      /* Time */
#if 1
      var_node = json_find_member(event_node, "time");
      if(var_node == NULL || var_node->tag != JSON_STRING)
      {
        fprintf(stderr, "[events-http] JSON - time not found\n");
        app_state_ptr->events_source_ok = false;
        json_delete(json_node);
        sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
        continue;
      }
      event_time_unix = (int)(timestamp_no_ms_from_rfc8601(var_node->string_) / 1000);
#else
      var_node = json_find_member(event_node, "time_unix");
      if(var_node == NULL || var_node->tag != JSON_NUMBER)
      {
        fprintf(stderr, "[events-http] JSON - type not found\n");
        app_state_ptr->events_source_ok = false;
        json_delete(json_node);
        sleep_ms_or_signal(HTTP_RETRY_PERIOD_S * 1000, &app_state_ptr->app_exit);
        continue;
      }
      event_time_unix = (int)var_node->number_;

#endif

      /* For anything newer than a minute ago, append to temporary list */
      if(event_time_unix > (time_now - 60))
      {
        events_append(&events_downloaded_list, event_time_unix, event_description, event_type);
      }
    }
    
    json_delete(json_node);

    if(events_downloaded_list != NULL)
    {
      /* Delete old events list and link pointer to new one */
      pthread_mutex_lock(&app_state_ptr->events_mutex);

      events_old_ptr = app_state_ptr->events;
      app_state_ptr->events = events_downloaded_list;
      events_delete(events_old_ptr);
      events_downloaded_list = NULL;

      pthread_mutex_unlock(&app_state_ptr->events_mutex);
    }

    app_state_ptr->events_source_ok = true;
    sleep_ms_or_signal(HTTP_UPDATE_INTERVAL_S * 1000, &app_state_ptr->app_exit);
  }

  curl_global_cleanup();

  pthread_exit(NULL);
}
