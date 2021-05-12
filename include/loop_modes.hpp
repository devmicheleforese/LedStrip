#ifndef LOOP_MODES_HPP
#define LOOP_MODES_HPP

#include "settings.h"
#include <Adafruit_NeoPixel.h>

// Loop Functions
#pragma region LoopFunctions
void fixed_color(DeviceInfo &dev) {
  dev.strip.fill(Adafruit_NeoPixel::Color(dev.fixedColorData.color.r,
                                          dev.fixedColorData.color.g,
                                          dev.fixedColorData.color.b),
                 0, dev.defaultData.ledLenght);
  dev.strip.setBrightness(dev.defaultData.brightness);
  dev.strip.show();
}

void rainbow(DeviceInfo &dev) {
  static long firstPixelHue = 0;
  long hueDifference        = 65536L / dev.strip.numPixels();

  // millis() % 1000 Transorms the internal clock in the claped version of
  // single second Then the result is translated into double with (result /
  // 1000.0) Now we have a function that returns a value between 0 and 1 so we *
  // 65535L The Max value for a HUE Color.
  //
  // 4 * rainbowData.velocity / 100
  // (rainbowData.velocity / 100) limits the velocity between 0 and 1
  // the the "3" means: We do the full walk of the HUE that times a second.
  firstPixelHue = int(
      ((int(millis() * 3 * dev.rainbowData.velocity / 100) % 1000) / 1000.0) *
      65535L);

  for (int i = 0; i < dev.strip.numPixels(); i++) {
    int pixelHue = firstPixelHue + (i * hueDifference);
    if (pixelHue > 65536) {
      pixelHue -= 65535;
    }
    dev.strip.setPixelColor(i, dev.strip.gamma32(dev.strip.ColorHSV(
                                   pixelHue, 255, dev.defaultData.brightness)));
  }
  dev.strip.show();
}

void color_split(DeviceInfo &dev) {
  // Set First Color
  dev.strip.fill(Adafruit_NeoPixel::Color(dev.colorSplitData.color1.r,
                                          dev.colorSplitData.color1.g,
                                          dev.colorSplitData.color1.b),
                 0, dev.colorSplitData.endFirstLedSplit);

  // Set Second Color
  dev.strip.fill(Adafruit_NeoPixel::Color(dev.colorSplitData.color2.r,
                                          dev.colorSplitData.color2.g,
                                          dev.colorSplitData.color2.b),
                 dev.colorSplitData.endFirstLedSplit);
  dev.strip.show();
}
#pragma endregion LoopFuctions

#endif // LOOP_MODES_HPP