#include "functions.hpp"
#include <Adafruit_NeoPixel.h>


void rainbow(Adafruit_NeoPixel& strip) {

  for (int i = 0; i < strip.numPixels(); ++i)
  {
    strip.setPixelColor(i, strip.Color(0,0,255));
  }
  unsigned int brightness;
  brightness = ((sin(millis()/2000.0 * 2.0 * PI) +1 ) * 255)/2;
  strip.setBrightness(brightness);
  // Serial.println(brightness);
}

void fixed_color(Adafruit_NeoPixel & strip, int color, unsigned int brightness = 100) {

  for (int i = 0; i < strip.numPixels(); ++i)
  {
    strip.setPixelColor(i, color);
  }
  strip.setBrightness(brightness);
}


RGB_Color hex_to_rgb(int hex) {
  byte red = hex >> 16;
  byte green = (hex & 0x00ff00) >> 8;
  byte blue = (hex & 0x0000ff);
  return RGB_Color{red, green, blue};
}