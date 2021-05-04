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
#include <Adafruit_NeoPixel.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEUUID.h>
#include <BLE2902.h>

#include <ArduinoJson.h>

#include "Arduino.h"
#include "SPIFFS.h"

#include "settings.h"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/


#pragma region Declarations
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
Adafruit_NeoPixel led(1, int(deviceInfo.led_pin), NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip(30, int(deviceInfo.strip_pin), NEO_GRB + NEO_KHZ800);

#pragma endregion Declarations

// Loop Functions
#pragma region LoopFunctions
void fixed_color(Adafruit_NeoPixel & strip, DefaultData dData, FixedColorData& data) {
  strip.fill(Adafruit_NeoPixel::Color(data.color.r, data.color.g, data.color.b), 0, dData.ledLenght);
  strip.setBrightness(dData.brightness);
}

void rainbow(Adafruit_NeoPixel& strip, DefaultData dData, RainbowData& data) {
  static long firstPixelHue = 0;
  long hueDifference = 65536L / strip.numPixels();

  // millis() % 1000 Transorms the internal clock in the claped version of single second
  // Then the result is translated into double with (result / 1000.0)
  // Now we have a function that returns a value between 0 and 1 so we * 65535L
  // The Max value for a HUE Color.
  //
  // 4 * rainbowData.velocity / 100
  // (rainbowData.velocity / 100) limits the velocity between 0 and 1
  // the the "3" means: We do the full walk of the HUE that times a second.
  firstPixelHue = int(((int(millis() * 3 * deviceInfo.rainbowData.velocity / 100) % 1000) / 1000.0) * 65535L);

  for (int i = 0; i < strip.numPixels(); i++)
  {
    int pixelHue = firstPixelHue + (i * hueDifference);
    if (pixelHue >65536)
    {
      pixelHue -= 65535;
    }
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue, 255,deviceInfo.defaultData.brightness)));
  }
}

void color_split(Adafruit_NeoPixel & strip, DefaultData dData, ColorSplitData& data) {
  // Set First Color
  strip.fill(
    Adafruit_NeoPixel::Color(
      data.color1.r,
      data.color1.g,
      data.color1.b),
    0,
    data.endFirstLedSplit
  );

  // Set Second Color
  strip.fill(
    Adafruit_NeoPixel::Color(
      data.color2.r,
      data.color2.g,
      data.color2.b),
    data.endFirstLedSplit
  );
}
#pragma endregion LoopFuctions


#pragma region CallbackSetMods

void loadSettingsData() {
  // TODO: Update function for sending data to the right location.
  Serial.println("[STRIP] - sendSettings - Sending Settings to Phone");
  byte* data;

  // blecDefaultData
  byte defaultData[] = {deviceInfo.defaultData.ledLenght, deviceInfo.defaultData.brightness};
  deviceInfo.blecDefaultData->setValue(defaultData, sizeof(DefaultData));
  data = deviceInfo.blecDefaultData->getData();
  Serial.println(
    "[STRIP] - sendSettings - blecDefaultData - Data: " + 
    String(int(data[0])) + "," +
    String(int(data[1]))
  );

  // blecFixedColorData
  byte fixedColorData[] = {
    deviceInfo.fixedColorData.color.r,
    deviceInfo.fixedColorData.color.g,
    deviceInfo.fixedColorData.color.b,
  };
  deviceInfo.blecFixedColorData->setValue(fixedColorData, sizeof(fixedColorData));
  data = deviceInfo.blecFixedColorData->getData();
  Serial.println(
    "[STRIP] - sendSettings - blecFixedColorData - Data: " + 
    String(int(data[0])) + "," +
    String(int(data[1]))
  );

  // blecRainbowData
  byte rainbowData[] = {
    deviceInfo.rainbowData.velocity,
  };
  deviceInfo.blecRainbowData->setValue(rainbowData, sizeof(rainbowData));
  data = deviceInfo.blecRainbowData->getData();
  Serial.println(
    "[STRIP] - sendSettings - blecRainbowData - Data: " + 
    String(int(data[0]))
  );

    // blecFixedColorData
  byte colorSplitData[] = {
    deviceInfo.colorSplitData.endFirstLedSplit,
    deviceInfo.colorSplitData.color1.r,
    deviceInfo.colorSplitData.color1.g,
    deviceInfo.colorSplitData.color1.b,
    deviceInfo.colorSplitData.color2.r,
    deviceInfo.colorSplitData.color2.g,
    deviceInfo.colorSplitData.color2.b,
  };
  deviceInfo.blecColorSplitData->setValue(colorSplitData, sizeof(colorSplitData));
  data = deviceInfo.blecColorSplitData->getData();
  Serial.println(
    "[STRIP] - sendSettings - blecColorSplitData - Data: " + 
    String(int(data[0])) + "," +
    String(int(data[1])) + "," +
    String(int(data[2])) + "," +
    String(int(data[3])) + "," +
    String(int(data[4])) + "," +
    String(int(data[5])) + "," +
    String(int(data[6]))
  );

    // blecRainbowData
  byte blecActiveMode[] = {
    byte(deviceInfo.activeMode),
  };
  deviceInfo.blecActiveMode->setValue(blecActiveMode, sizeof(blecActiveMode));
  data = deviceInfo.blecActiveMode->getData();
  Serial.println(
    "[STRIP] - sendSettings - blecActiveMode - Data: " + 
    String(int(data[0]))
  );

  Serial.println("[STRIP] - sendSettings - Sending Settings Done");
}

class StripServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Device Conected");

    loadSettingsData();
    BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device Disconnected");
  }
};

void setDefaultSettings(const byte *buffer){
  Serial.println("[STRIP] - setDefaultSettings - Called");
  if(buffer[0] > 0) {
    strip.clear();
    strip.fill(0);
    strip.show();
    deviceInfo.defaultData.ledLenght = buffer[0];
    strip.updateLength(deviceInfo.defaultData.ledLenght);
    strip.show();
  } else {
    Serial.println("[STRIP] - setDefaultSettings - ERROR - Recived negative lenght");
  }
  Serial.println("[STRIP] - setDefaultSettings - Strip Lenght updated");


  deviceInfo.defaultData.brightness = buffer[1];
  strip.setBrightness(deviceInfo.defaultData.brightness);
  strip.show();
  Serial.println("[STRIP] - setDefaultSettings - Strip brightess Updated");

  Serial.println("************Default***********");
  Serial.println("LedLenght: " + String(deviceInfo.defaultData.ledLenght));
  Serial.println("Brightness: " + String(deviceInfo.defaultData.brightness));
  Serial.println("******************************");
}

void setFixedColorData(const byte *buffer){
  Serial.println("[STRIP] - setFixedColorData - Called");
  int index = 0;
  deviceInfo.fixedColorData.color.r = buffer[index++];
  deviceInfo.fixedColorData.color.g = buffer[index++];
  deviceInfo.fixedColorData.color.b = buffer[index++];
  Serial.println("[STRIP] - setFixedColorData - color updated");


  Serial.println("********FixedColor Mod********");
  Serial.println(
    "Color: " +
    String(deviceInfo.fixedColorData.color.r) + "," +
    String(deviceInfo.fixedColorData.color.g) + "," +
    String(deviceInfo.fixedColorData.color.b)
  );
  Serial.println("******************************");
}

void setRainbowData(const byte *buffer) {
  Serial.println("[STRIP] - setRainbowData - Called");

  int velocityIndex = 0;
  deviceInfo.rainbowData.velocity = buffer[velocityIndex];

  Serial.println("**********Rainbow Mod*********");
  Serial.println("Velocity: " + String(deviceInfo.rainbowData.velocity));
  Serial.println("******************************");
}

void setColorSplitData(const byte *buffer) {
  Serial.println("[STRIP] - setColorSplitData - Called");
  int index = 0;

  deviceInfo.colorSplitData.endFirstLedSplit = buffer[index++];
  Serial.println("[STRIP] - setFixedColorData - endFirstLedSplit updated");

  deviceInfo.colorSplitData.color1.r = buffer[index++];
  deviceInfo.colorSplitData.color1.g = buffer[index++];
  deviceInfo.colorSplitData.color1.b = buffer[index++];
  Serial.println("[STRIP] - setFixedColorData - color1 updated");

  deviceInfo.colorSplitData.color2.r = buffer[index++];
  deviceInfo.colorSplitData.color2.g = buffer[index++];
  deviceInfo.colorSplitData.color2.b = buffer[index++];
  Serial.println("[STRIP] - setFixedColorData - color2 updated");

  Serial.println("********ColorSplit Mod********");
  Serial.println("FirstSplitLenght: " + String(deviceInfo.colorSplitData.endFirstLedSplit));
  Serial.println("SecondSplitLenght: " + String(deviceInfo.defaultData.ledLenght - deviceInfo.colorSplitData.endFirstLedSplit));
  Serial.println(
    "Color1: " +
    String(deviceInfo.colorSplitData.color1.r) + "," +
    String(deviceInfo.colorSplitData.color1.g) + "," +
    String(deviceInfo.colorSplitData.color1.b)
    );
  Serial.println(
    "Color2: " +
    String(deviceInfo.colorSplitData.color2.r) + "," +
    String(deviceInfo.colorSplitData.color2.g) + "," +
    String(deviceInfo.colorSplitData.color2.b));
  Serial.println("******************************");
}

void setActiveMode(const byte* buffer) {
  Serial.println("[STRIP] - setActiveMode - Called");
  switch (buffer[0]) {

  case (int)Mode_Type::fixed_color:
    deviceInfo.activeMode = Mode_Type::fixed_color;
    Serial.println("[STRIP] - setActiveMode - Mode_Type::fixed_color Active");
    break;

  case (int)Mode_Type::rainbow:
    deviceInfo.activeMode = Mode_Type::rainbow;
    Serial.println("[STRIP] - setActiveMode - Mode_Type::rainbow Active");
    break;

  case (int)Mode_Type::color_split:
    deviceInfo.activeMode = Mode_Type::color_split;
    Serial.println("[STRIP] - setActiveMode - Mode_Type::color_split Active");
    break;

  default:
    Serial.println("[STRIP] - setActiveMode - ERROR - Recived another value: " + String((int)buffer[0]));
    break;
  }
}

bool save_data(const char* filename) {

  // Open the file in READ_MODE
  File outFile = SPIFFS.open(filename, "r");

  // Allocate JsonDocument
  DynamicJsonDocument sett(1024);

  // Parse outFile in Json
  if(deserializeJson(sett, outFile) != DeserializationError::Ok) {
    return false;
  }

  // Close the outFile
  outFile.close();

  // Sett Parameters
  sett["DefaultData"]["ledLenght"] = deviceInfo.defaultData.ledLenght;
  sett["DefaultData"]["brightness"] = deviceInfo.defaultData.brightness;

  sett["FixedColorData"]["color"][0] = deviceInfo.fixedColorData.color.r;
  sett["FixedColorData"]["color"][1] = deviceInfo.fixedColorData.color.g;
  sett["FixedColorData"]["color"][2] = deviceInfo.fixedColorData.color.b;

  sett["RainbowData"]["velocity"] = deviceInfo.rainbowData.velocity;

  sett["ColorSplitData"]["endFirstLedSplit"] = deviceInfo.colorSplitData.endFirstLedSplit;
  sett["ColorSplitData"]["color1"][0] = deviceInfo.colorSplitData.color1.r;
  sett["ColorSplitData"]["color1"][1] = deviceInfo.colorSplitData.color1.g;
  sett["ColorSplitData"]["color1"][2] = deviceInfo.colorSplitData.color1.b;

  sett["ColorSplitData"]["color2"][0] = deviceInfo.colorSplitData.color2.r;
  sett["ColorSplitData"]["color2"][1] = deviceInfo.colorSplitData.color2.g;
  sett["ColorSplitData"]["color2"][2] = deviceInfo.colorSplitData.color2.b;

  Serial.println(String((int)deviceInfo.activeMode));

  sett["mode"] = (byte)deviceInfo.activeMode;

  outFile = SPIFFS.open(filename, "w");

  if(serializeJson(sett, outFile) == 0) {
    return false;
  }

  outFile.close();

  return true;
}

#pragma endregion CallbackSetMods

bool SPIFFS_init() {
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return false;
  }
  return true;
}

void setJsonSettingsData() {
  // Initialize SPIFFS
  if(!SPIFFS_init()) return;

  Serial.println("SPIFFS Initialized");

  // Open File Settings
  File file = SPIFFS.open("/settings.json", FILE_READ);

  // Error Checking for reading file
  if(!file) {
    Serial.print("Failed to open file for reading");
    return;
  } else {
    Serial.println("File opened");
  }

  // Initialize Json document for settings
  DynamicJsonDocument sett(1024);
  // Error checking
  DeserializationError error = deserializeJson(sett, file);
  if(error) Serial.print("Failed to Deserialize.");
  file.close();


  //-------- retrive settings --------//

  Serial.println("Starting Retriving");

  // DefaultData
  deviceInfo.defaultData.ledLenght     = sett["DefaultData"]["ledLenght"];
  deviceInfo.defaultData.brightness    = sett["DefaultData"]["brightness"];
  Serial.println("defaultData.ledLenght: " + String(deviceInfo.defaultData.ledLenght));
  Serial.println("defaultData.brightness: " + String(deviceInfo.defaultData.brightness));

  // FixedColorData
  deviceInfo.fixedColorData.color.r = sett["FixedColorData"]["color"][0];
  deviceInfo.fixedColorData.color.g = sett["FixedColorData"]["color"][1];
  deviceInfo.fixedColorData.color.b = sett["FixedColorData"]["color"][2];
  Serial.println(
    "fixedColorData.color: " +
    String((int)sett["FixedColorData"]["color"][0]) + "," +
    String((int)sett["FixedColorData"]["color"][1]) + "," +
    String((int)sett["FixedColorData"]["color"][2])
  );

  // RainbowData
  deviceInfo.rainbowData.velocity = sett["RainbowData"]["velocity"];
  Serial.println("rainbowData.velocity: " + String(deviceInfo.rainbowData.velocity));

  // ColorSplitData
  deviceInfo.colorSplitData.endFirstLedSplit = sett["ColorSplitData"]["endFirstLedSplit"];

  deviceInfo.colorSplitData.color1.r = sett["ColorSplitData"]["color1"][0];
  deviceInfo.colorSplitData.color1.g = sett["ColorSplitData"]["color1"][1];
  deviceInfo.colorSplitData.color1.b = sett["ColorSplitData"]["color1"][2];

  deviceInfo.colorSplitData.color2.r = sett["ColorSplitData"]["color2"][0];
  deviceInfo.colorSplitData.color2.g = sett["ColorSplitData"]["color2"][1];
  deviceInfo.colorSplitData.color2.b = sett["ColorSplitData"]["color2"][2];

  Serial.println("colorSplitData.endFirstLedSplit: " + String(deviceInfo.colorSplitData.endFirstLedSplit));
  Serial.println(
    "colorSplitData.color1: " +
    String(deviceInfo.colorSplitData.color1.r) + "," +
    String(deviceInfo.colorSplitData.color1.g) + "," +
    String(deviceInfo.colorSplitData.color1.b)
  );

  Serial.println(
    "colorSplitData.color2: " +
    String(deviceInfo.colorSplitData.color2.r) + "," +
    String(deviceInfo.colorSplitData.color2.g) + "," +
    String(deviceInfo.colorSplitData.color2.b)
  );

  // Mode
  switch ((int)sett["mode"])
  {
  case (int)Mode_Type::fixed_color:
    deviceInfo.activeMode = Mode_Type::fixed_color;
    break;
  case (int)Mode_Type::rainbow:
    deviceInfo.activeMode = Mode_Type::rainbow;
    break;
  case (int)Mode_Type::color_split:
    deviceInfo.activeMode = Mode_Type::color_split;
    break;
  default:
    deviceInfo.activeMode = Mode_Type::fixed_color;
    break;
  }
  Serial.println(String((int)sett["mode"]));
}

#pragma region Callbacks
//----- Callbacks -----//
class blecDefaultDataCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecDefaultDataCallback - Called Callback");
    std::string value = pCharacteristic->getValue();
    const byte* buffer = (byte*)value.c_str();

    setDefaultSettings(buffer);
    Serial.println("[BLE] - blecDefaultDataCallback - End Callback");
  }
};

class blecFixedColorDataCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecFixedColorDataCallback - Called Callback");
    std::string value = pCharacteristic->getValue();
    const byte* buffer = (byte*)value.c_str();

    setFixedColorData(buffer);

    Serial.println("[BLE] - blecFixedColorDataCallback - End Callback");
  }
};

class blecRainbowDataCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecRainbowDataCallback - Called Callback");
    std::string value = pCharacteristic->getValue();
    const byte* buffer = (byte*)value.c_str();

    setRainbowData(buffer);

    Serial.println("[BLE] - blecRainbowDataCallback - End Callback");
  }
};

class blecColorSplitDataCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecColorSplitDataCallback - Called Callback");
    std::string value = pCharacteristic->getValue();
    const byte* buffer = (byte*)value.c_str();

    setColorSplitData(buffer);

    Serial.println("[BLE] - blecRainbowDataCallback - End Callback");
  }
};

class blecActiveModeCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecActiveModeCallback - Called Callback");
    std::string value = pCharacteristic->getValue();

    const byte* buffer = (byte*)value.c_str();

    setActiveMode(buffer);

    Serial.println("[BLE] - blecActiveModeCallback - End Callback");
  }
};

class blecSaveSettingsCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecSaveSettingsCallback - Called Callback");
    std::string value = pCharacteristic->getValue();

    const byte * buffer = (byte*)value.c_str();

    // if there is a "1" then save the data
    if(buffer[0] == true) {
      if(!save_data("/settings.json")) {
        Serial.println("[BLE] - blecSaveSettingsCallback - Error: Data Not Saved");
      } else {
        Serial.println("[BLE] - blecSaveSettingsCallback - Data Saved");
      }
    } else {
      Serial.println("[BLE] - blecSaveSettingsCallback - Error: buffer data");
      Serial.println("[BLE] - blecSaveSettingsCallback - Error: received " + String((int)buffer[0]));
    }
    Serial.println("[BLE] - blecSaveSettingsCallback - End Callback");
  }
};

class blecSendDataCallBack: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecSendDataCallBack - Called Callback");
    std::string value = pCharacteristic->getValue();

    const byte * buffer = (byte*)value.c_str();

    // if there is a "1" then save the data
    if(buffer[0] == true) {
      loadSettingsData();
      Serial.println("[BLE] - blecSendDataCallBack - Data Send");
    } else {
      Serial.println("[BLE] - blecSendDataCallBack - Error: buffer data");
      Serial.println("[BLE] - blecSendDataCallBack - Error: received " + String((int)buffer[0]));
    }
    Serial.println("[BLE] - blecSendDataCallBack - End Callback");
  }
};

class blecNotificationCallBack: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE] - blecNotificationCallBack - Called Callback");
    std::string value = pCharacteristic->getValue();

    const byte * buffer = (byte*)value.c_str();

    static bool isNofitying = false;
    if(buffer[0] == true && isNofitying == false) {
      isNofitying = true;
      for (size_t i = 0; i < 10; i++)
      {
        digitalWrite(deviceInfo.led_pin, HIGH);
        delay(500);
        digitalWrite(deviceInfo.led_pin, LOW);
        delay(500);
      }
      isNofitying = false;
    }

    Serial.println("[BLE] - blecNotificationCallBack - End Callback");
  }
};
#pragma endregion Callbacks


void BLE_init() {
  //-------- Initialize BLE Operations --------//

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  deviceInfo.bleSStripServer = BLEDevice::createServer();

  // Set Server Callback
  deviceInfo.bleSStripServer->setCallbacks(new StripServerCallbacks());

  // Create the BLE Service
  deviceInfo.blesService = deviceInfo.bleSStripServer->createService(deviceInfo.bluetoothSett.BLE_Service_UUID);

  //----- Create a BLE Characteristics -----//
  // blecDefaultData Characteristic
  deviceInfo.blecDefaultData = deviceInfo.blesService->createCharacteristic(
                      deviceInfo.bluetoothSett.BLE_DefaultData_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  deviceInfo.blecDefaultData->setCallbacks(new blecDefaultDataCallback());
  // deviceInfo.blecDefaultData->addDescriptor(new BLE2902());


  // blecFixedColorData Characteristic
  deviceInfo.blecFixedColorData = deviceInfo.blesService->createCharacteristic(
                      deviceInfo.bluetoothSett.BLE_FixedColorData_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  deviceInfo.blecFixedColorData->setCallbacks(new blecFixedColorDataCallback());
  // deviceInfo.blecFixedColorData->addDescriptor(new BLE2902());



  // blecRainbowData Characteristic
  deviceInfo.blecRainbowData = deviceInfo.blesService->createCharacteristic(
                      deviceInfo.bluetoothSett.BLE_RainbowData_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  deviceInfo.blecRainbowData->setCallbacks(new blecRainbowDataCallback());
  // deviceInfo.blecRainbowData->addDescriptor(new BLE2902());



  // blecColorSplitData Characteristic
  deviceInfo.blecColorSplitData = deviceInfo.blesService->createCharacteristic(
                      deviceInfo.bluetoothSett.BLE_ColorSplitData_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  deviceInfo.blecColorSplitData->setCallbacks(new blecColorSplitDataCallback());
  // deviceInfo.blecColorSplitData->addDescriptor(new BLE2902());


  // blecActiveMode Characteristic
  deviceInfo.blecActiveMode = deviceInfo.blesService->createCharacteristic(
                      deviceInfo.bluetoothSett.BLE_ActiveMode_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  if(deviceInfo.blecActiveMode == NULL)
    Serial.println("blecActiveMode NULL");

  deviceInfo.blecActiveMode->setCallbacks(new blecActiveModeCallback());
  // deviceInfo.blecActiveMode->addDescriptor(new BLE2902());


  // blesServiceSettings SERVICE
  deviceInfo.blesServiceSettings = deviceInfo.bleSStripServer->createService(deviceInfo.bluetoothSett.BLE_ServiceSettings_UUID);

    // blecSaveSettings Characteristic
  deviceInfo.blecSaveSettings = deviceInfo.blesServiceSettings->createCharacteristic(
                      deviceInfo.bluetoothSett.BLE_SaveSettings_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  deviceInfo.blecSaveSettings->setCallbacks(new blecSaveSettingsCallback());
  // deviceInfo.blecSaveSettings->addDescriptor(new BLE2902());

  // blecSendData Characteristic
  deviceInfo.blecSendData = deviceInfo.blesServiceSettings->createCharacteristic(
                      deviceInfo.bluetoothSett.BLE_SendData_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  deviceInfo.blecSendData->setCallbacks(new blecSendDataCallBack());
  // deviceInfo.blecSendData->addDescriptor(new BLE2902());


  deviceInfo.blecNotification = deviceInfo.blesServiceSettings->createCharacteristic(
    deviceInfo.bluetoothSett.BLE_Notification_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE
  );

  deviceInfo.blecNotification->setCallbacks(new blecNotificationCallBack());

  // Start the service
  deviceInfo.blesService->start();
  deviceInfo.blesServiceSettings->start();


  // Start advertising
  deviceInfo.bleaAdvertising = BLEDevice::getAdvertising();
  deviceInfo.bleaAdvertising->addServiceUUID(deviceInfo.bluetoothSett.BLE_Service_UUID);
  deviceInfo.bleaAdvertising->addServiceUUID(deviceInfo.bluetoothSett.BLE_ServiceSettings_UUID);
  deviceInfo.bleaAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}



void setup() {
  Serial.begin(115200);

  Serial.println("BEGIN");

  delay(5000);

  setJsonSettingsData();

  BLE_init();

  loadSettingsData();

  led.begin();
  led.show();

  pinMode(int(deviceInfo.push_button_pin), INPUT);
  pinMode(int(deviceInfo.led_pin), OUTPUT);
}

void run_mod() {
  switch (deviceInfo.activeMode) {
  case Mode_Type::fixed_color:
    fixed_color(strip, deviceInfo.defaultData, deviceInfo.fixedColorData);
    break;
  case Mode_Type::rainbow:
    rainbow(strip, deviceInfo.defaultData, deviceInfo.rainbowData);
    break;
  case Mode_Type::color_split:
    color_split(strip, deviceInfo.defaultData, deviceInfo.colorSplitData);
    break;
  default:
    break;
  }
}

void loop() {
  run_mod();

  strip.show();
}

int main() {
  setup();

  while(true) loop();
}