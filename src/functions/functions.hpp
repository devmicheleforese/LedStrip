#pragma once
#include <Adafruit_NeoPixel.h>

void rainbow(Adafruit_NeoPixel & strip);

struct RGB_Color {
  unsigned int r;
  unsigned int g;
  unsigned int b;
};

void fixed_color(Adafruit_NeoPixel & strip, int color, unsigned int brightness);

RGB_Color hex_to_rgb(int hex);