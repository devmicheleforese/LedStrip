/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF:
   https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic
   notifications. The service advertises itself as:
   4fafc201-1fb5-459e-8fcc-c5c9c331914b And has a characteristic of:
   beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that
   performs notification every couple of seconds.
*/
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUUID.h>
#include <BLEUtils.h>

#include "Arduino.h"
#include "SPIFFS.h"
#include "ble.hpp"
#include "loop_modes.hpp"
#include "settings.h"

bool SPIFFS_init() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return false;
  }
  return true;
}

void setJsonSettingsData() {
  // Initialize SPIFFS
  if (!SPIFFS_init()) {
    Serial.println("ERROR: SPIFFS_init()");
    return;
  } else {
    Serial.println("SPIFFS Initialized");
  }

  // Open File Settings
  File file = SPIFFS.open("/settings.json", FILE_READ);

  // Error Checking for reading file
  if (!file) {
    Serial.print("Failed to open file for reading");
    return;
  } else {
    Serial.println("File opened");
  }

  // Initialize Json document for settings
  DynamicJsonDocument sett(1024);
  // Error checking
  DeserializationError error = deserializeJson(sett, file);
  if (error)
    Serial.print("Failed to Deserialize.");
  file.close();

  //-------- retrive settings --------//

  Serial.println("Starting Retriving");

  // DefaultData
  device.defaultData.ledLenght  = sett["DefaultData"]["ledLenght"];
  device.defaultData.brightness = sett["DefaultData"]["brightness"];

  // FixedColorData
  device.fixedColorData.color.r = sett["FixedColorData"]["color"][0];
  device.fixedColorData.color.g = sett["FixedColorData"]["color"][1];
  device.fixedColorData.color.b = sett["FixedColorData"]["color"][2];

  // RainbowData
  device.rainbowData.velocity = sett["RainbowData"]["velocity"];

  // ColorSplitData
  device.colorSplitData.endFirstLedSplit =
      sett["ColorSplitData"]["endFirstLedSplit"];

  device.colorSplitData.color1.r = sett["ColorSplitData"]["color1"][0];
  device.colorSplitData.color1.g = sett["ColorSplitData"]["color1"][1];
  device.colorSplitData.color1.b = sett["ColorSplitData"]["color1"][2];

  device.colorSplitData.color2.r = sett["ColorSplitData"]["color2"][0];
  device.colorSplitData.color2.g = sett["ColorSplitData"]["color2"][1];
  device.colorSplitData.color2.b = sett["ColorSplitData"]["color2"][2];

  // Mode
  switch ((int)sett["mode"]) {
  case (int)Mode_Type::fixed_color:
    device.activeMode = Mode_Type::fixed_color;
    break;
  case (int)Mode_Type::rainbow:
    device.activeMode = Mode_Type::rainbow;
    break;
  case (int)Mode_Type::color_split:
    device.activeMode = Mode_Type::color_split;
    break;
  default:
    device.activeMode = Mode_Type::fixed_color;
    break;
  }

  device.print();
}

void BLE_init() {
  //-------- Initialize BLE Operations --------//

  // Create the BLE Device
  BLEDevice::init("Strip Led");

  // Create the BLE Server
  device.bleSServer = BLEDevice::createServer();

  // Set Server Callback
  device.bleSServer->setCallbacks(new StripServerCallbacks());

  // Create the BLE Service
  device.blesData =
      device.bleSServer->createService(device.bluetoothSett.BLEs_Data_UUID);

  // blecDefaultData Characteristic
  device.blecDefaultData = device.blesData->createCharacteristic(
      device.bluetoothSett.BLEc_DefaultData_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  device.blecDefaultData->setCallbacks(new blecDefaultDataCallback());
  // deviceInfo.blecDefaultData->addDescriptor(new BLE2902());

  // blecFixedColorData Characteristic
  device.blecFixedColorData = device.blesData->createCharacteristic(
      device.bluetoothSett.BLEc_FixedColorData_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  device.blecFixedColorData->setCallbacks(new blecFixedColorDataCallback());
  // deviceInfo.blecFixedColorData->addDescriptor(new BLE2902());

  // blecRainbowData Characteristic
  device.blecRainbowData = device.blesData->createCharacteristic(
      device.bluetoothSett.BLEc_RainbowData_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  device.blecRainbowData->setCallbacks(new blecRainbowDataCallback());
  // deviceInfo.blecRainbowData->addDescriptor(new BLE2902());

  // blecColorSplitData Characteristic
  device.blecColorSplitData = device.blesData->createCharacteristic(
      device.bluetoothSett.BLEc_ColorSplitData_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  device.blecColorSplitData->setCallbacks(new blecColorSplitDataCallback());
  // deviceInfo.blecColorSplitData->addDescriptor(new BLE2902());

  // blecActiveMode Characteristic
  device.blecActiveMode = device.blesData->createCharacteristic(
      device.bluetoothSett.BLEc_ActiveMode_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  if (device.blecActiveMode == NULL)
    Serial.println("blecActiveMode NULL");

  device.blecActiveMode->setCallbacks(new blecActiveModeCallback());
  // deviceInfo.blecActiveMode->addDescriptor(new BLE2902());

  // SERVICE - blesServiceSettings
  device.blesServiceSettings =
      device.bleSServer->createService(device.bluetoothSett.BLEs_Settings_UUID);

  // Characteristic - blecSaveSettings
  device.blecSaveSettings = device.blesServiceSettings->createCharacteristic(
      device.bluetoothSett.BLEc_SaveSettings_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  device.blecSaveSettings->setCallbacks(new blecSaveSettingsCallback());

  // Characteristic - blecSendData
  device.blecSendData = device.blesServiceSettings->createCharacteristic(
      device.bluetoothSett.BLEc_SendData_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  device.blecSendData->setCallbacks(new blecSendDataCallBack());

  // Characteristic - blecNotification
  device.blecNotification = device.blesServiceSettings->createCharacteristic(
      device.bluetoothSett.BLEc_Notification_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  device.blecNotification->setCallbacks(new blecNotificationCallBack());

  // // Server - GenericAccess
  // device.blesGenericAccess = device.bleSServer->createService(
  //     device.bluetoothSett.BLEs_GenericAccess_UUID);

  // // Characteristic - blecDeviceName
  // device.blecDeviceName = device.blesGenericAccess->createCharacteristic(
  //     BLEUUID(device.bluetoothSett.BLEc_DeviceName_UUID),
  //     BLECharacteristic::PROPERTY_READ);

  // device.blecDeviceName->setValue("Strip Led");

  // // Characteristic - blecAppearance
  // device.blecAppearance = device.blesGenericAccess->createCharacteristic(
  //     BLEUUID(device.bluetoothSett.BLEc_Appearance_UUID),
  //     BLECharacteristic::PROPERTY_READ);

  uint16_t appearance = 0x07C6;
  // device.blecAppearance->setValue(appearance);

  // // Characteristic - blecPeripheralPreferredConnectionParameters
  // device.blecPeripheralPreferredConnectionParameters =
  //     device.blesGenericAccess->createCharacteristic(
  //         BLEUUID(device.bluetoothSett
  //                     .BLEc_PeripheralPreferredConnectionParameters_UUID),
  //         BLECharacteristic::PROPERTY_READ);

  // Server - Device Information
  device.blesDeviceInformation = device.bleSServer->createService(
      device.bluetoothSett.BLEs_DeviceInformation_UUID);

  device.blecManufacturerName =
      device.blesDeviceInformation->createCharacteristic(
          BLEUUID(device.bluetoothSett.BLEc_ManufacturerName_UUID),
          BLECharacteristic::PROPERTY_READ);

  device.blecManufacturerName->setValue("ACME Systems");

  device.blecModelNumber = device.blesDeviceInformation->createCharacteristic(
      BLEUUID(device.bluetoothSett.BLEc_ModelNumber_UUID),
      BLECharacteristic::PROPERTY_READ);

  device.blecModelNumber->setValue("v1.0");

  device.blecFirmwareRevision =
      device.blesDeviceInformation->createCharacteristic(
          BLEUUID(device.bluetoothSett.BLEc_FirmwareRevision_UUID),
          BLECharacteristic::PROPERTY_READ);

  device.blecFirmwareRevision->setValue("v1.0");

  // Start advertising
  device.bleaAdvertising = BLEDevice::getAdvertising();
  device.bleaAdvertising->addServiceUUID(device.bluetoothSett.BLEs_Data_UUID);
  device.bleaAdvertising->addServiceUUID(
      device.bluetoothSett.BLEs_Settings_UUID);
  // device.bleaAdvertising->addServiceUUID(
  //     device.bluetoothSett.BLEs_GenericAccess_UUID);
  device.bleaAdvertising->addServiceUUID(
      device.bluetoothSett.BLEs_DeviceInformation_UUID);
  device.bleaAdvertising->setMinPreferred(
      0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();

  // Start the service
  device.blesData->start();
  device.blesServiceSettings->start();
  // device.blesGenericAccess->start();
  device.blesDeviceInformation->start();

  device.bleaAdvertising->setMinPreferred(
      0x0); // set value to 0x00 to not advertise this parameter
  device.bleaAdvertising->start();
  device.bleaAdvertising->setAppearance(appearance);

  device.bleaAdvertising->start();

  BLEService *testService = device.bleSServer->getServiceByUUID(
      BLEUUID(device.bluetoothSett.BLEs_GenericAccess_UUID));

  if (testService != nullptr) {
    Serial.println("Service found: " + String(testService->toString().c_str()));
  }
  Serial.println("Waiting a client connection to notify...");
}

void setup() {
  Serial.begin(115200);

  Serial.println("BEGIN");

  // delay(5000);

  setJsonSettingsData();

  BLE_init();

  loadSettingsData();

  device.led.begin();
  // device.strip.begin();

  pinMode(int(device.push_button_pin), INPUT);
  pinMode(int(device.led_pin), OUTPUT);
}

void run_mod() {
  switch (device.activeMode) {
  case Mode_Type::fixed_color:
    fixed_color(device);
    break;
  case Mode_Type::rainbow:
    rainbow(device);
    break;
  case Mode_Type::color_split:
    color_split(device);
    break;
  default:
    break;
  }
}

void loop() {
  run_mod();

  device.strip.show();
}

int main() {
  setup();

  while (true)
    loop();
}