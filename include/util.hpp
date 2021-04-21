#include "SPIFFS.h"
#include <ArduinoJson.h>


void listAllFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file) {
    Serial.println("File: " + String(file.name()));
    file = root.openNextFile();
  }
  root.close();
  file.close();
}

void readDatafromFile(const char* filename) {
  File file = SPIFFS.open(filename);
  if(file) {
    DynamicJsonDocument sett(1024);
    DeserializationError error = deserializeJson(sett, file);
    if(error) Serial.println("Error readDatafromFile");
    Serial.println(
      // TODO: Add ["color"] between call.
      String((int)sett["FixedColorData"]["color"][0]) + "," +
      String((int)sett["FixedColorData"]["color"][1]) + "," +
      String((int)sett["FixedColorData"]["color"][2])
    );
  }
  file.close();
}


struct Color_RGB {
  byte r;
  byte g;
  byte b;
};


Color_RGB hex_to_rgb(int hex) {
  byte red = hex >> 16;
  byte green = (hex & 0x00ff00) >> 8;
  byte blue = (hex & 0x0000ff);
  return Color_RGB{red, green, blue};
}

uint32_t hex2int(const char *hex) {
  uint32_t val = 0;
  while (*hex) {
    // get current character then increment
    uint8_t byte = *hex++;
    // transform hex character to the 4bit equivalent number, using the ascii table indexes
    if (byte >= '0' && byte <= '9') byte = byte - '0';
    else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
    else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;
    // shift 4 to make space for new digit, and add the 4 bits of the new digit
    val = (val << 4) | (byte & 0xF);
  }
  return val;
}