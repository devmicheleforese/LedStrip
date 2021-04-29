#include "Arduino.h"
#include "util.hpp"

enum class Mode_Type : byte{
  fixed_color = 1,
  rainbow = 2,
  color_split = 3,
};


//----- Modes Data structures -----//
struct DefaultData {
  int ledLenght;
  int brightness;
};
struct FixedColorData {
  Color_RGB color;
};

struct RainbowData {
  int velocity;
};
struct ColorSplitData {
  int endFirstLedSplit;
  Color_RGB color1;
  Color_RGB color2;
};

struct BluetoothSett {
  char BLE_Service_UUID[37] = "b722533f-8e22-4678-be87-bb6e6c237860";

  char BLE_DefaultData_UUID[37] = "9c1389dd-0a31-4355-ad5d-1bc47a11c7e2";
  char BLE_FixedColorData_UUID[37] = "2135f2b6-0ce2-47f3-bc2f-120e454be7e0";
  char BLE_RainbowData_UUID[37] = "e1ee97c2-f08d-451b-92e6-f52c508c40af";
  char BLE_ColorSplitData_UUID[37] = "47b31a46-19d1-407c-b126-f6c1561bb23c";
  char BLE_ActiveMode_UUID[37] = "6e19f003-b524-4852-a57c-63a6529c3a12";

  char BLE_ServiceSettings_UUID[37] = "f349aa66-7acf-41c6-b9a4-ce34ef3f54e6";

  char BLE_SaveSettings_UUID[37] = "2c203874-7ad6-4230-bc5c-09e2aa7a382f";
  char BLE_SendData_UUID[37] = "0c098b94-87d6-4cfa-b649-7ad5debb4409";
};

struct DeviceInfo {
  const byte led_pin = 13;
  const byte strip_pin = 14;
  const byte push_button_pin = 16;

  BluetoothSett bluetoothSett;

  DefaultData defaultData;
  FixedColorData fixedColorData;
  RainbowData rainbowData;
  ColorSplitData colorSplitData;
  Mode_Type activeMode;

  // BLE
  BLEServer* bleSStripServer = NULL;
  BLEService* blesService = NULL;
  BLEAdvertising* bleaAdvertising = NULL;

  BLECharacteristic* blecDefaultData = NULL;
  BLECharacteristic* blecFixedColorData = NULL;
  BLECharacteristic* blecRainbowData = NULL;
  BLECharacteristic* blecColorSplitData = NULL;
  BLECharacteristic* blecActiveMode = NULL;

  BLEService* blesServiceSettings = NULL;
  
  BLECharacteristic* blecSaveSettings = NULL;
  BLECharacteristic* blecSendData = NULL;


  BluetoothSerial SerialBT;

} deviceInfo;

