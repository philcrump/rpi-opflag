#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "main.h"
#include "alarms.h"

void alarms_append(alarm_t **alarms_ptr_ptr, char* alarm_description, int alarm_severity)
{
  alarm_t *new_alarm_ptr = (alarm_t *)malloc(sizeof(alarm_t));

  if(new_alarm_ptr == NULL)
  {
    fprintf(stderr, "[alarms_append] Error allocating memory for new alarm.\n");
    return;
  }

  new_alarm_ptr->description = strdup(alarm_description);
  new_alarm_ptr->severity = alarm_severity;
  new_alarm_ptr->next = NULL;
  if(*alarms_ptr_ptr == NULL)
  {
    /* First alarm */
    *alarms_ptr_ptr = new_alarm_ptr;
  }
  else
  {
    alarm_t *alarm_cursor;

    /* Find the end of the list */
    alarm_cursor = *alarms_ptr_ptr;
    while(alarm_cursor->next != NULL)
    {
      alarm_cursor = alarm_cursor->next;
    }

    /* Append the new alarm */
    alarm_cursor->next = new_alarm_ptr;
  }
}

void alarms_delete(alarm_t *alarms_ptr)
{
  if(alarms_ptr == NULL)
  {
    return;
  }

  alarm_t *alarm_cursor, *alarm_next;

  alarm_cursor = alarms_ptr;
  while(alarm_cursor != NULL)
  {
    alarm_next = alarm_cursor->next;

    free(alarm_cursor);

    alarm_cursor = alarm_next;
  }
}