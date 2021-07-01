#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "Arduino.h"
#include "util.hpp"

enum class Mode_Type : byte {
  fixed_color = 1,
  rainbow     = 2,
  color_split = 3,
};

//----- Modes Data structures -----//
struct DefaultData {
  uint8_t ledLenght;
  uint8_t brightness;

  void print() {
    Serial.print("DefaultData.ledLenght: ");
    Serial.println(ledLenght);
    Serial.print("DefaultData.brightness: ");
    Serial.println(brightness);
  }
};
struct FixedColorData {
  Color_RGB color;

  void print() {
    Serial.print("FixedColorData.color: ");
    Serial.print(color.toString());
  }
};

struct RainbowData {
  uint8_t velocity;

  void print() {
    Serial.print("RainbowData.velocity: ");
    Serial.println(velocity);
  }
};
struct ColorSplitData {
  uint8_t endFirstLedSplit;
  Color_RGB color1;
  Color_RGB color2;

  void print() {
    Serial.print("ColorSplitData.endFirstLedSplit: ");
    Serial.println(endFirstLedSplit);
    Serial.print("ColorSplitData.color1: ");
    Serial.println(color1.toString());
    Serial.print("ColorSplitData.endFirstLedSplit: ");
    Serial.println(color2.toString());
  }
};

struct BluetoothSett {
  char BLEs_Data_UUID[37]           = "b722533f-8e22-4678-be87-bb6e6c237860";
  char BLEc_DefaultData_UUID[37]    = "9c1389dd-0a31-4355-ad5d-1bc47a11c7e2";
  char BLEc_FixedColorData_UUID[37] = "2135f2b6-0ce2-47f3-bc2f-120e454be7e0";
  char BLEc_RainbowData_UUID[37]    = "e1ee97c2-f08d-451b-92e6-f52c508c40af";
  char BLEc_ColorSplitData_UUID[37] = "47b31a46-19d1-407c-b126-f6c1561bb23c";
  char BLEc_ActiveMode_UUID[37]     = "6e19f003-b524-4852-a57c-63a6529c3a12";

  char BLEs_Settings_UUID[37]     = "f349aa66-7acf-41c6-b9a4-ce34ef3f54e6";
  char BLEc_SaveSettings_UUID[37] = "2c203874-7ad6-4230-bc5c-09e2aa7a382f";
  char BLEc_SendData_UUID[37]     = "0c098b94-87d6-4cfa-b649-7ad5debb4409";
  char BLEc_Notification_UUID[37] = "f5964ee2-01fb-4e94-8843-fdbeb3b3f723";
  char BLEc_OnOff_UUID[37]        = "301b81e3-8e41-4b84-804f-2ad18cc092e5";

  // uint16_t BLEs_GenericAccess_UUID = 0x1800; // Generic Service
  // uint16_t BLEc_DeviceName_UUID    = 0x2A00; // Strip Led
  // uint16_t BLEc_Appearance_UUID    = 0x2A01; // 0x07C6 - Multi­Color LED
  // Array uint16_t BLEc_PeripheralPreferredConnectionParameters_UUID =
  //     0x2A04; // 0x07C6 - Multi­Color LED Array

  uint16_t BLEs_DeviceInformation_UUID = 0x180A; // Device Information Service
  uint16_t BLEc_ManufacturerName_UUID  = 0x2A29; // ACME Systems
  uint16_t BLEc_ModelNumber_UUID       = 0x2A24; // v1.0
  uint16_t BLEc_FirmwareRevision_UUID  = 0x2A26; // v1.0
};

struct DeviceInfo {
  const byte led_pin         = 13;
  const byte strip_pin       = 14;
  const byte push_button_pin = 16;

  const BluetoothSett bluetoothSett;

  DefaultData defaultData;
  FixedColorData fixedColorData;
  RainbowData rainbowData;
  ColorSplitData colorSplitData;
  Mode_Type activeMode;

  void print() {
    defaultData.print();
    fixedColorData.print();
    rainbowData.print();
    colorSplitData.print();
    Serial.println("ActiveMode: " + String(int(activeMode)));
  }

  // BLE
  BLEServer *bleSServer           = nullptr;
  BLEAdvertising *bleaAdvertising = nullptr;

  BLEService *blesData                  = nullptr;
  BLECharacteristic *blecDefaultData    = nullptr;
  BLECharacteristic *blecFixedColorData = nullptr;
  BLECharacteristic *blecRainbowData    = nullptr;
  BLECharacteristic *blecColorSplitData = nullptr;
  BLECharacteristic *blecActiveMode     = nullptr;

  BLEService *blesServiceSettings     = nullptr;
  BLECharacteristic *blecSaveSettings = nullptr;
  BLECharacteristic *blecSendData     = nullptr;
  BLECharacteristic *blecNotification = nullptr;
  BLECharacteristic *blecOnOff        = nullptr;

  BLEService *blesGenericAccess                                  = nullptr;
  BLECharacteristic *blecDeviceName                              = nullptr;
  BLECharacteristic *blecAppearance                              = nullptr;
  BLECharacteristic *blecPeripheralPreferredConnectionParameters = nullptr;

  BLEService *blesDeviceInformation       = nullptr;
  BLECharacteristic *blecManufacturerName = nullptr;
  BLECharacteristic *blecModelNumber      = nullptr;
  BLECharacteristic *blecFirmwareRevision = nullptr;

  bool deviceConnected = false;

  Adafruit_NeoPixel led = Adafruit_NeoPixel(1, 13, NEO_GRB + NEO_KHZ800);
  Adafruit_NeoPixel strip =
      Adafruit_NeoPixel(30, int(strip_pin), NEO_GRB + NEO_KHZ800);

  bool isOn = true;

} device;

Adafruit_NeoPixel *led = new Adafruit_NeoPixel(1, 13, NEO_GRB + NEO_KHZ800);

#endif // SETTINGS_HPP