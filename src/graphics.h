#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

void date_render(char *date_string);

void time_render(char *time_string);

void alarm_line_render(event_t *events_ptr);

void flag_render(int32_t flag_status, bool acknowledged);

void connectionstatus_render(char *text_string, bool status_ok);

#endif /* __GRAPHICS_H__ */