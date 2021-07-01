#ifndef BLE_HPP
#define BLE_HPP

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUUID.h>
#include <BLEUtils.h>

#include "Arduino.h"
#include "SPIFFS.h"
#include "settings.h"
#include <Adafruit_NeoPixel.h>

#pragma region CallbackSetMods

void loadSettingsData() {
  // TODO: Update function for sending data to the right location.
  Serial.println("[STRIP] - sendSettings - Sending Settings to Phone");
  byte *data;

  // blecDefaultData
  byte defaultData[] = {device.defaultData.ledLenght,
                        device.defaultData.brightness};
  device.blecDefaultData->setValue(defaultData, sizeof(DefaultData));
  data = device.blecDefaultData->getData();
  Serial.println("[STRIP] - sendSettings - blecDefaultData - Data: " +
                 String(int(data[0])) + "," + String(int(data[1])));

  // blecFixedColorData
  byte fixedColorData[] = {
      device.fixedColorData.color.r,
      device.fixedColorData.color.g,
      device.fixedColorData.color.b,
  };
  device.blecFixedColorData->setValue(fixedColorData, sizeof(fixedColorData));
  data = device.blecFixedColorData->getData();
  Serial.println("[STRIP] - sendSettings - blecFixedColorData - Data: " +
                 String(int(data[0])) + "," + String(int(data[1])));

  // blecRainbowData
  byte rainbowData[] = {
      device.rainbowData.velocity,
  };
  device.blecRainbowData->setValue(rainbowData, sizeof(rainbowData));
  data = device.blecRainbowData->getData();
  Serial.println("[STRIP] - sendSettings - blecRainbowData - Data: " +
                 String(int(data[0])));

  // blecFixedColorData
  byte colorSplitData[] = {
      device.colorSplitData.endFirstLedSplit, device.colorSplitData.color1.r,
      device.colorSplitData.color1.g,         device.colorSplitData.color1.b,
      device.colorSplitData.color2.r,         device.colorSplitData.color2.g,
      device.colorSplitData.color2.b,
  };
  device.blecColorSplitData->setValue(colorSplitData, sizeof(colorSplitData));
  data = device.blecColorSplitData->getData();
  Serial.println("[STRIP] - sendSettings - blecColorSplitData - Data: " +
                 String(int(data[0])) + "," + String(int(data[1])) + "," +
                 String(int(data[2])) + "," + String(int(data[3])) + "," +
                 String(int(data[4])) + "," + String(int(data[5])) + "," +
                 String(int(data[6])));

  // blecRainbowData
  byte blecActiveMode[] = {
      byte(device.activeMode),
  };
  device.blecActiveMode->setValue(blecActiveMode, sizeof(blecActiveMode));
  data = device.blecActiveMode->getData();
  Serial.println("[STRIP] - sendSettings - blecActiveMode - Data: " +
                 String(int(data[0])));

  byte blecOnOff[] = {
      byte(device.isOn),
  };
  device.blecOnOff->setValue(blecOnOff, sizeof(blecOnOff));
  data = device.blecActiveMode->getData();
  Serial.println("[STRIP] - sendSettings - blecOnOff - Data: " +
                 String(int(data[0])));

  Serial.println("[STRIP] - sendSettings - Sending Settings Done");
}

void setDefaultSettings(const byte *buffer) {
  Serial.println("[STRIP] - setDefaultSettings - Called");
  if (buffer[0] > 0) {
    device.strip.clear();
    device.strip.fill(0);
    device.strip.show();
    device.defaultData.ledLenght = buffer[0];
    device.strip.updateLength(device.defaultData.ledLenght);
    device.strip.show();
  } else {
    Serial.println(
        "[STRIP] - setDefaultSettings - ERROR - Recived negative lenght");
  }
  Serial.println("[STRIP] - setDefaultSettings - Strip Lenght updated");

  device.defaultData.brightness = buffer[1];
  device.strip.setBrightness(device.defaultData.brightness);
  device.strip.show();
  Serial.println("[STRIP] - setDefaultSettings - Strip brightess Updated");

  Serial.println("************Default***********");
  Serial.println("LedLenght: " + String(device.defaultData.ledLenght));
  Serial.println("Brightness: " + String(device.defaultData.brightness));
  Serial.println("******************************");
}

void setFixedColorData(const byte *buffer) {
  Serial.println("[STRIP] - setFixedColorData - Called");
  int index                     = 0;
  device.fixedColorData.color.r = buffer[index++];
  device.fixedColorData.color.g = buffer[index++];
  device.fixedColorData.color.b = buffer[index++];
  Serial.println("[STRIP] - setFixedColorData - color updated");

  Serial.println("********FixedColor Mod********");
  Serial.println("Color: " + String(device.fixedColorData.color.r) + "," +
                 String(device.fixedColorData.color.g) + "," +
                 String(device.fixedColorData.color.b));
  Serial.println("******************************");
}

void setRainbowData(const byte *buffer) {
  Serial.println("[STRIP] - setRainbowData - Called");

  int velocityIndex           = 0;
  device.rainbowData.velocity = buffer[velocityIndex];

  Serial.println("**********Rainbow Mod*********");
  Serial.println("Velocity: " + String(device.rainbowData.velocity));
  Serial.println("******************************");
}

void setColorSplitData(const byte *buffer) {
  Serial.println("[STRIP] - setColorSplitData - Called");
  int index = 0;

  device.colorSplitData.endFirstLedSplit = buffer[index++];
  Serial.println("[STRIP] - setFixedColorData - endFirstLedSplit updated");

  device.colorSplitData.color1.r = buffer[index++];
  device.colorSplitData.color1.g = buffer[index++];
  device.colorSplitData.color1.b = buffer[index++];
  Serial.println("[STRIP] - setFixedColorData - color1 updated");

  device.colorSplitData.color2.r = buffer[index++];
  device.colorSplitData.color2.g = buffer[index++];
  device.colorSplitData.color2.b = buffer[index++];
  Serial.println("[STRIP] - setFixedColorData - color2 updated");

  Serial.println("********ColorSplit Mod********");
  Serial.println("FirstSplitLenght: " +
                 String(device.colorSplitData.endFirstLedSplit));
  Serial.println("SecondSplitLenght: " +
                 String(device.defaultData.ledLenght -
                        device.colorSplitData.endFirstLedSplit));
  Serial.println("Color1: " + String(device.colorSplitData.color1.r) + "," +
                 String(device.colorSplitData.color1.g) + "," +
                 String(device.colorSplitData.color1.b));
  Serial.println("Color2: " + String(device.colorSplitData.color2.r) + "," +
                 String(device.colorSplitData.color2.g) + "," +
                 String(device.colorSplitData.color2.b));
  Serial.println("******************************");
}

void setActiveMode(const byte *buffer) {
  Serial.println("[STRIP] - setActiveMode - Called");
  switch (buffer[0]) {
  case (int)Mode_Type::fixed_color:
    device.activeMode = Mode_Type::fixed_color;
    Serial.println("[STRIP] - setActiveMode - Mode_Type::fixed_color Active");
    break;

  case (int)Mode_Type::rainbow:
    device.activeMode = Mode_Type::rainbow;
    Serial.println("[STRIP] - setActiveMode - Mode_Type::rainbow Active");
    break;

  case (int)Mode_Type::color_split:
    device.activeMode = Mode_Type::color_split;
    Serial.println("[STRIP] - setActiveMode - Mode_Type::color_split Active");
    break;

  default:
    Serial.println("[STRIP] - setActiveMode - ERROR - Recived another value: " +
                   String((int)buffer[0]));
    break;
  }
}

bool save_data(const char *filename) {
  // Open the file in READ_MODE
  File outFile = SPIFFS.open(filename, "r");

  // Allocate JsonDocument
  DynamicJsonDocument sett(1024);

  // Parse outFile in Json
  if (deserializeJson(sett, outFile) != DeserializationError::Ok) {
    return false;
  }

  // Close the outFile
  outFile.close();

  // Sett Parameters
  sett["DefaultData"]["ledLenght"]  = device.defaultData.ledLenght;
  sett["DefaultData"]["brightness"] = device.defaultData.brightness;

  sett["FixedColorData"]["color"][0] = device.fixedColorData.color.r;
  sett["FixedColorData"]["color"][1] = device.fixedColorData.color.g;
  sett["FixedColorData"]["color"][2] = device.fixedColorData.color.b;

  sett["RainbowData"]["velocity"] = device.rainbowData.velocity;

  sett["ColorSplitData"]["endFirstLedSplit"] =
      device.colorSplitData.endFirstLedSplit;
  sett["ColorSplitData"]["color1"][0] = device.colorSplitData.color1.r;
  sett["ColorSplitData"]["color1"][1] = device.colorSplitData.color1.g;
  sett["ColorSplitData"]["color1"][2] = device.colorSplitData.color1.b;

  sett["ColorSplitData"]["color2"][0] = device.colorSplitData.color2.r;
  sett["ColorSplitData"]["color2"][1] = device.colorSplitData.color2.g;
  sett["ColorSplitData"]["color2"][2] = device.colorSplitData.color2.b;

  sett["mode"] = (byte)device.activeMode;

  outFile = SPIFFS.open(filename, "w");

  if (serializeJson(sett, outFile) == 0) {
    return false;
  }

  outFile.close();

  return true;
}

TaskHandle_t NotificationTask;
EventGroupHandle_t notificationEvent;

void notificationLoop(void *pvParameters) {
  EventBits_t uxBits;
  // device.led.begin();

  while (true) {
    uxBits = xEventGroupWaitBits(notificationEvent, BIT0, pdTRUE, pdFALSE,
                                 pdMS_TO_TICKS(portMAX_DELAY));

    if ((uxBits & BIT0) != 0) {
      Serial.println("notification() start");

      // for (size_t i = 0; i < 10; i++) {
      //   digitalWrite(13, HIGH);
      //   // device.led.setPixelColor(0,
      //   //                          Adafruit_NeoPixel::Color(255, 255, 255,
      //   //                          255));
      //   // device.led.show();
      //   vTaskDelay(500 / portTICK_RATE_MS);
      //   digitalWrite(13, LOW);
      //   // device.led.clear();
      //   // device.led.show();
      //   vTaskDelay(500 / portTICK_RATE_MS);
      // }

      Serial.println("notification() end");
    } else {
      continue;
    }
  }
}

#pragma endregion CallbackSetMods

#pragma region Callbacks

class StripServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    device.deviceConnected = true;
    Serial.println("Device Conected");
    device.led.fill(Adafruit_NeoPixel::Color(255, 255, 255, 255));
    device.led.show();

    xEventGroupSetBits(notificationEvent, BIT0);

    loadSettingsData();
    BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer *pServer) {
    device.deviceConnected = false;

    if (!save_data("/settings.json")) {
      Serial.println(
          "[DEVICE] - save_data('/settings.json') - Error: Data Not Saved");
    } else {
      Serial.println("[DEVICE] - save_data('/settings.json') - Data Saved");
    }

    device.led.clear();
    device.led.show();

    Serial.println("Device Disconnected");
  }
};

class blecDefaultDataCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecDefaultDataCallback - Called Callback");
    std::string value  = pCharacteristic->getValue();
    const byte *buffer = (byte *)value.c_str();

    setDefaultSettings(buffer);
    Serial.println("[BLE] - blecDefaultDataCallback - End Callback");
  }
};

class blecFixedColorDataCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecFixedColorDataCallback - Called Callback");
    std::string value  = pCharacteristic->getValue();
    const byte *buffer = (byte *)value.c_str();

    setFixedColorData(buffer);

    Serial.println("[BLE] - blecFixedColorDataCallback - End Callback");
  }
};

class blecRainbowDataCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecRainbowDataCallback - Called Callback");
    std::string value  = pCharacteristic->getValue();
    const byte *buffer = (byte *)value.c_str();

    setRainbowData(buffer);

    Serial.println("[BLE] - blecRainbowDataCallback - End Callback");
  }
};

class blecColorSplitDataCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecColorSplitDataCallback - Called Callback");
    std::string value  = pCharacteristic->getValue();
    const byte *buffer = (byte *)value.c_str();

    setColorSplitData(buffer);

    Serial.println("[BLE] - blecRainbowDataCallback - End Callback");
  }
};

class blecActiveModeCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecActiveModeCallback - Called Callback");
    std::string value = pCharacteristic->getValue();

    const byte *buffer = (byte *)value.c_str();

    setActiveMode(buffer);

    Serial.println("[BLE] - blecActiveModeCallback - End Callback");
  }
};

class blecSaveSettingsCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecSaveSettingsCallback - Called Callback");
    std::string value = pCharacteristic->getValue();

    const byte *buffer = (byte *)value.c_str();

    // if there is a "1" then save the data
    if (buffer[0] == true) {
      if (!save_data("/settings.json")) {
        Serial.println(
            "[BLE] - blecSaveSettingsCallback - Error: Data Not Saved");
      } else {
        Serial.println("[BLE] - blecSaveSettingsCallback - Data Saved");
      }
    } else {
      Serial.println("[BLE] - blecSaveSettingsCallback - Error: buffer data");
      Serial.println("[BLE] - blecSaveSettingsCallback - Error: received " +
                     String((int)buffer[0]));
    }
    Serial.println("[BLE] - blecSaveSettingsCallback - End Callback");
  }
};

class blecSendDataCallBack : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecSendDataCallBack - Called Callback");
    std::string value = pCharacteristic->getValue();

    const byte *buffer = (byte *)value.c_str();

    // if there is a "1" then save the data
    if (buffer[0] == true) {
      loadSettingsData();
      Serial.println("[BLE] - blecSendDataCallBack - Data Send");
    } else {
      Serial.println("[BLE] - blecSendDataCallBack - Error: buffer data");
      Serial.println("[BLE] - blecSendDataCallBack - Error: received " +
                     String((int)buffer[0]));
    }
    Serial.println("[BLE] - blecSendDataCallBack - End Callback");
  }
};

class blecNotificationCallBack : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecNotificationCallBack - Called Callback");
    std::string value = pCharacteristic->getValue();

    const byte *buffer = (byte *)value.c_str();

    if (buffer[0] == true) {
      xEventGroupSetBits(notificationEvent, BIT0);
    }

    Serial.println("[BLE] - blecNotificationCallBack - End Callback");
  }
};

class blecOnOffCallBack : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecOnOffCallBack - Called Callback");
    std::string value = pCharacteristic->getValue();

    const byte *buffer = (byte *)value.c_str();

    if (buffer[0] > 0) {
      device.isOn = true;
    } else {
      device.isOn = false;
    }

    Serial.println("[BLE] - blecOnOffCallBack - End Callback");
  }
};

#pragma endregion Callbacks

#endif // BLE_HPP