#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include "main.h"
#include "backlight.h"
#include "screen.h"
#include "graphics.h"

int main(int argc, char* argv[])
{
  (void) argc;
  (void) argv;

  /* Switch on screen */
  backlight_power(true);

  /* Initialise screen and splash */
  if(!screen_init(true, "Waiting for network.."))
  {
    fprintf(stderr, "Error initialising screen!\n");
    return 1;
  }

  screen_deinit();

  return 0;
}
