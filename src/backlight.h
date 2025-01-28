#ifndef __BACKLIGHT_H__
#define __BACKLIGHT_H__

#include <stdint.h>
#include <stdbool.h>

void backlight_level(int32_t level_value);

void backlight_power(bool on);

#endif /* __BACKLIGHT_H__ */