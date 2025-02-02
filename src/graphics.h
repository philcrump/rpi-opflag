#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

void datetime_render(char *datetime_string);

void alarms_render(bool http_ok, alarm_t *alarms_ptr);

void flag_render(int32_t flag_status, bool acknowledged);

void connectionstatus_render(char *text_string, bool status_ok);

#endif /* __GRAPHICS_H__ */