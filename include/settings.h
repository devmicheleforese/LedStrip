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
  char BLE_Service_UUID[64];
  char BLE_Characteristic_UUID[64];
  char BLE_Device_to_espr_UUID[64];
};

struct DeviceInfo {
  const byte led_pin = 13;
  const byte strip_pin = 14;
  const byte push_button_pin = 16;

  char id[64];

  BluetoothSett bluetoothSett;

  DefaultData defaultData;
  FixedColorData fixedColorData;
  RainbowData rainbowData;
  ColorSplitData colorSplitData;
  Mode_Type mode;

  BLEServer* pServer = NULL;
  BLEService *pService = NULL;
  BLECharacteristic* pCharacteristic = NULL;

} deviceInfo;

