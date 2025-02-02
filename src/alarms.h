#ifndef __ALARMS_H__
#define __ALARMS_H__

void alarms_append(alarm_t **alarms_ptr_ptr, char* alarm_description, int alarm_type);
void alarms_delete(alarm_t *alarms_ptr);

void *alarms_http_thread(void *arg);

#endif /* __ALARMS_H__ */