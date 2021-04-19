/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "Arduino.h"
#include <BLEUUID.h>
#include <string>
#include <iostream>

#include <Adafruit_NeoPixel.h>



BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "f4923e28-b468-46f7-bba8-b752fe1e14b8"
#define CHARACTERISTIC_UUID "a48a5e2d-e5e2-4252-8ba2-21435ec2399e"

enum class Board_Pin
{
  led = 13,
  strip = 14,
  push_button = 16,
};

Adafruit_NeoPixel led(1, int(Board_Pin::led), NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip(30, int(Board_Pin::strip), NEO_GRB + NEO_KHZ800);


const String ID_DEVICE = "StripLed_v1";


enum class Mode_Type : byte{
  fixed_color = 1,
  rainbow = 2,
  color_split = 3,
} mode;

const int DEVICE_MODES_LENGHT = 3;

byte DEVICE_MODES[DEVICE_MODES_LENGHT] = {
  (int)Mode_Type::fixed_color,
  (int)Mode_Type::rainbow,
  (int)Mode_Type::color_split,
};

struct DefaultData {
  int ledLenght;
  int brightness;
} defaultData;
struct FixedColorData {
  uint32_t color;
} fixedColorData;

struct RainbowData {
  double velocity;
} rainbowData;
struct ColorSplitData {
  int endFirstLedSplit;
  uint32_t color1;
  uint32_t color2;
} colorSplitData;

struct RGB_Color {
  unsigned int r;
  unsigned int g;
  unsigned int b;
};


long firstPixelHue = 0;

void fixed_color(Adafruit_NeoPixel & strip, DefaultData dData, FixedColorData& data) {
  strip.fill(data.color, 0, dData.ledLenght);
  // for (int i = 0; i < strip.numPixels(); ++i)
  // {
  //   strip.setPixelColor(i, data.color);
  // }
  strip.setBrightness(dData.brightness);
}

void rainbow(Adafruit_NeoPixel& strip, DefaultData dData, RainbowData& data) {
  // firstPixelHue += 256;
  // if (firstPixelHue >= 3 * 65536)
  // {
  //   firstPixelHue = 0;
  // }
  // for (int i = 0; i < strip.numPixels(); i++)
  // {
  //   int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
  //   strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue, 255,defaultData.brightness)));
  // }
  firstPixelHue += (rainbowData.velocity /100.0)* 2000;
  if (firstPixelHue >= 3 * 65536)
  {
    firstPixelHue = 0;
  }
  for (int i = 0; i < strip.numPixels(); i++)
  {
    int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue, 255,defaultData.brightness)));
  }
}

void color_split(Adafruit_NeoPixel & strip, DefaultData dData, ColorSplitData& data) {
  for (int i = 0; i < data.endFirstLedSplit; ++i)
  {
    strip.setPixelColor(i, data.color1);
  }
  for(int i = data.endFirstLedSplit; i < dData.ledLenght; ++i) {
    strip.setPixelColor(i, data.color2);
  }
  strip.setBrightness(dData.brightness);
}


RGB_Color hex_to_rgb(int hex) {
  byte red = hex >> 16;
  byte green = (hex & 0x00ff00) >> 8;
  byte blue = (hex & 0x0000ff);
  return RGB_Color{red, green, blue};
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



class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

void setDefaultSettings(std::string &val, int firstIndex){
  int ledLenghtIndex = firstIndex;
  defaultData.ledLenght = (int)val[ledLenghtIndex];

  Serial.print("Cleared.");
  //strip.clear();
  strip.fill(0,0,30);
  strip.show();
  strip.updateLength(defaultData.ledLenght);


  int brightnessIndex = firstIndex + 1;
  defaultData.brightness = (int)val[brightnessIndex];
  Serial.println("************Default***********");
  Serial.println("LedLenght: " + String(defaultData.ledLenght));
  Serial.println("Velocity: " + String(defaultData.brightness));
  Serial.println("******************************");
}

void setFixedColorData(const std::string &val, int firstIndex){
  int colorIndex = firstIndex;
  fixedColorData.color = Adafruit_NeoPixel::Color((int)val[colorIndex], (int)val[colorIndex + 1], (int)val[colorIndex + 2]);

  Serial.println("********FixedColor Mod********");
  Serial.println("Color: " + String((int)val[colorIndex]) + "," + String((int)val[colorIndex + 1]) + "," + String((int)val[colorIndex + 2]));
  Serial.println("******************************");
}

void setRainbowData(const std::string &val, int firstIndex) {
  int velocityIndex = firstIndex;
  rainbowData.velocity = (int)val[velocityIndex];

  Serial.println("**********Rainbow Mod*********");
  Serial.println("Velocity: " + String(rainbowData.velocity));
  Serial.println("******************************");
}

void setColorSplitData(const std::string &val, int index) {
  int endFirstLedSplitIndex = index;
  colorSplitData.endFirstLedSplit = (int)val[endFirstLedSplitIndex];

  int color1Index = index + 1;
  colorSplitData.color1 = Adafruit_NeoPixel::Color(val[color1Index], val[color1Index + 1], val[color1Index + 2]);

  int color2Index = index + 4;
  colorSplitData.color2 = Adafruit_NeoPixel::Color(val[color2Index], val[color2Index + 1], val[color2Index + 2]);

  Serial.println("********ColorSplit Mod********");
  Serial.println("FirstSplitLenght: " + String(colorSplitData.endFirstLedSplit));
  Serial.println("SecondSplitLenght: " + String(defaultData.ledLenght - colorSplitData.endFirstLedSplit));
  Serial.println("Color1: " + String(val[color1Index]) + "," + String(val[color1Index + 1]) + "," + String(val[color1Index + 2]));
  Serial.println("Color2: " + String(val[color2Index]) + "," + String(val[color2Index + 1]) + "," + String(val[color2Index + 2]));
  Serial.println("******************************");
}

class DeviceCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    Serial.println(value.length());

    switch ((int)value[0]) {
    case 0:
      setDefaultSettings(value, 1);
      break;
    case (int)Mode_Type::fixed_color:
      setFixedColorData(value, 1);
      mode = Mode_Type::fixed_color;
      break;
    case (int)Mode_Type::rainbow:
      setRainbowData(value, 1);
      mode = Mode_Type::rainbow;
      break;
    case (int)Mode_Type::color_split:
      setColorSplitData(value, 1);
      mode = Mode_Type::color_split;
      break;
    default:
      break;
    }
  }
};



void setup() {
  Serial.begin(9600);

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );


  pCharacteristic->setCallbacks(new DeviceCallback());

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  // pAdvertising->setMinPreferred(0x06);
  // pAdvertising->setMinPreferred(0x12);
  // pAdvertising->setMinInterval(1);
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");


  led.begin();
  led.show();


  pinMode(int(Board_Pin::push_button), INPUT);
}

void loop() {
  if (deviceConnected) {
    delay(10); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }

  switch (mode) {
  case Mode_Type::fixed_color:
    fixed_color(strip, defaultData, fixedColorData);
    break;
  case Mode_Type::rainbow:
    rainbow(strip, defaultData, rainbowData);
    break;
  case Mode_Type::color_split:
    color_split(strip, defaultData, colorSplitData);
    break;
  default:
    break;
  }

  strip.show();
}