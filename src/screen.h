#ifndef __SCREEN_H__
#define __SCREEN_H__

typedef struct {
  uint8_t Blue;
  uint8_t Green;
  uint8_t Red;
  uint8_t Alpha; // 0x80
} __attribute__((__packed__)) screen_pixel_t;

bool screen_init(bool splash_enable, char *splash_lower_line);
void *screen_thread(void *arg);
void screen_deinit(void);

void screen_clear(void);
void screen_setPixel(int x, int y, screen_pixel_t *pixel_ptr);
void screen_setPixelLine(int x, int y, int length, screen_pixel_t *pixel_array_ptr);

void screen_drawHorizontalLine(int x, int y, int length, const screen_pixel_t *pixel_ptr);

#endif /* __SCREEN_H__ */