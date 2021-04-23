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
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
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

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    char data[10] = "Ciao";
    pCharacteristic->setWriteNoResponseProperty(false);
    pCharacteristic->setValue((uint8_t*)&data, sizeof(data));
    Serial.println(data);
    pCharacteristic->notify();

    char buffer[20];

    File file = SPIFFS.open("/settings.json");
    for (size_t i = 0; file.available() || i<20; i++)
    {
      buffer[i] = file.read();
      if(i == 19) {
        pCharacteristic->setValue((uint8_t*)&buffer, sizeof(data));
        i = 0;
      }
    }


    file.size();

    BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

void setDefaultSettings(const byte *val, int firstIndex){
  int ledLenghtIndex = firstIndex;
  if(val[ledLenghtIndex]) {
    deviceInfo.defaultData.ledLenght = val[ledLenghtIndex];
    strip.clear();
    strip.updateLength(deviceInfo.defaultData.ledLenght);
  }

  int brightnessIndex = firstIndex + 1;
  strip.setBrightness(deviceInfo.defaultData.brightness);

  deviceInfo.defaultData.brightness = val[brightnessIndex];
  Serial.println("************Default***********");
  Serial.println("LedLenght: " + String(deviceInfo.defaultData.ledLenght));
  Serial.println("Brightness: " + String(deviceInfo.defaultData.brightness));
  Serial.println("******************************");
}

void setFixedColorData(const byte *val, int firstIndex){
  int colorIndex = firstIndex;
  deviceInfo.fixedColorData.color.r = val[colorIndex];
  deviceInfo.fixedColorData.color.g = val[colorIndex + 1];
  deviceInfo.fixedColorData.color.b = val[colorIndex + 2];


  Serial.println("********FixedColor Mod********");
  Serial.println(
    "Color: " +
    String(deviceInfo.fixedColorData.color.r) + "," +
    String(deviceInfo.fixedColorData.color.g) + "," +
    String(deviceInfo.fixedColorData.color.b)
  );
  Serial.println("******************************");
}

void setRainbowData(const byte *val, int firstIndex) {
  int velocityIndex = firstIndex;
  deviceInfo.rainbowData.velocity = val[velocityIndex];

  Serial.println("**********Rainbow Mod*********");
  Serial.println("Velocity: " + String(deviceInfo.rainbowData.velocity));
  Serial.println("******************************");
}

void setColorSplitData(const byte *val, int index) {
  int endFirstLedSplitIndex = index;
  deviceInfo.colorSplitData.endFirstLedSplit = val[endFirstLedSplitIndex];

  int color1Index = index + 1;
  deviceInfo.colorSplitData.color1.r = val[color1Index];
  deviceInfo.colorSplitData.color1.g = val[color1Index + 1];
  deviceInfo.colorSplitData.color1.b = val[color1Index + 2];


  int color2Index = index + 4;
  deviceInfo.colorSplitData.color2.r = val[color2Index];
  deviceInfo.colorSplitData.color2.g = val[color2Index + 1];
  deviceInfo.colorSplitData.color2.b = val[color2Index + 2];

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

#pragma endregion CallbackSetMods


void data_transfer(const byte* data) {
  switch (data[0]) {
  case 0:
    setDefaultSettings(data, 1);
    break;
  case (int)Mode_Type::fixed_color:
    setFixedColorData(data, 1);
    deviceInfo.mode = Mode_Type::fixed_color;
    break;
  case (int)Mode_Type::rainbow:
    setRainbowData(data, 1);
    deviceInfo.mode = Mode_Type::rainbow;
    break;
  case (int)Mode_Type::color_split:
    setColorSplitData(data, 1);
    deviceInfo.mode = Mode_Type::color_split;
    break;
  default:
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

  sett["ColorSplitData"]["color2"][0] = deviceInfo.colorSplitData.color1.r;
  sett["ColorSplitData"]["color2"][1] = deviceInfo.colorSplitData.color1.g;
  sett["ColorSplitData"]["color2"][2] = deviceInfo.colorSplitData.color1.b;

  Serial.println(String((int)deviceInfo.mode));

  sett["mode"] = (byte)deviceInfo.mode;

  Serial.println(String((int)sett["FixedColorData"]["color"][0]));
  Serial.println(String((int)sett["FixedColorData"]["color"][1]));
  Serial.println(String((int)sett["FixedColorData"]["color"][2]));

  outFile = SPIFFS.open(filename, "w");

  if(serializeJson(sett, outFile) == 0) {
    return false;
  }

  outFile.close();



  //listAllFiles();
  //readDatafromFile("/settings.json");

  return true;
}





class DeviceCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    const byte * buffer = (byte*)value.c_str();
    // Serial.println(value.length());

    switch (buffer[0])
    {
    // Save Data
    case 0:
      if(!save_data("/settings.json")) {
        Serial.println("Error: Data Not Saved");
      } else {
        Serial.println("Data Saved");
      }
      break;

    // data-transfer
    case 1:
      data_transfer(&buffer[1]);
      break;

    // Error Handling
    default:
      Serial.println("Error Buffer, Recived: " + String((int)buffer[0]));
      break;
    }
    delay(3);
  }
};


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

  // Device ID
  strlcpy(
    deviceInfo.id,
    sett["id_device"] | "ID_NOT_FOUND",
    sizeof(deviceInfo.id)
  );

  Serial.println("id_device Retrived");

  // SERVICE_UUID
  strlcpy(
    deviceInfo.bluetoothSett.BLE_Service_UUID,
    sett["SERVICE_UUID"] | "example.com",
    sizeof(deviceInfo.bluetoothSett.BLE_Service_UUID)
  );
  Serial.println("Service_UUID: " + String(deviceInfo.bluetoothSett.BLE_Service_UUID));

  // CHARACTERISTIC_UUID
  strlcpy(
    deviceInfo.bluetoothSett.BLE_Characteristic_UUID,
    sett["CHARACTERISTIC_UUID"] | "example.com",
    sizeof(deviceInfo.bluetoothSett.BLE_Characteristic_UUID)
  );
  Serial.println("Characteristic_UUID: " + String(deviceInfo.bluetoothSett.BLE_Characteristic_UUID));

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

  deviceInfo.colorSplitData.color1.r = sett["ColorSplitData"]["color2"][0];
  deviceInfo.colorSplitData.color1.g = sett["ColorSplitData"]["color2"][1];
  deviceInfo.colorSplitData.color1.b = sett["ColorSplitData"]["color2"][2];

  Serial.println("colorSplitData.endFirstLedSplit: " + String(deviceInfo.colorSplitData.endFirstLedSplit));
  Serial.println(
    "colorSplitData.color1: " +
    String(deviceInfo.colorSplitData.color1.r) + "," +
    String(deviceInfo.colorSplitData.color1.g) + "," +
    String(deviceInfo.colorSplitData.color1.b)
  );

  Serial.println(
    "colorSplitData.color2: " +
    String(deviceInfo.colorSplitData.color1.r) + "," +
    String(deviceInfo.colorSplitData.color1.g) + "," +
    String(deviceInfo.colorSplitData.color1.b)
  );

  // Mode
  switch ((int)sett["mode"])
  {
  case (int)Mode_Type::fixed_color:
    deviceInfo.mode = Mode_Type::fixed_color;
    break;
  case (int)Mode_Type::rainbow:
    deviceInfo.mode = Mode_Type::rainbow;
    break;
  case (int)Mode_Type::color_split:
    deviceInfo.mode = Mode_Type::color_split;
    break;
  default:
    deviceInfo.mode = Mode_Type::fixed_color;
    break;
  }
  Serial.println(String((int)sett["mode"]));
}


void BLE_init() {
  //-------- Initialize BLE Operations --------//

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(deviceInfo.bluetoothSett.BLE_Service_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      deviceInfo.bluetoothSett.BLE_Characteristic_UUID,
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
  pAdvertising->addServiceUUID(deviceInfo.bluetoothSett.BLE_Service_UUID);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}


void setup() {
  Serial.begin(115200);

  Serial.println("BEGIN");

  delay(5000);

  setJsonSettingsData();

  BLE_init();

  led.begin();
  led.show();

  pinMode(int(deviceInfo.push_button_pin), INPUT);
}

void run_mod() {
  switch (deviceInfo.mode) {
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

  run_mod();

  strip.show();
}

int main() {
  setup();

  while(true) loop();
}