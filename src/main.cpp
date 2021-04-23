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
#include "SPIFFS.h"
#include <ArduinoJson.h>

#include <Adafruit_NeoPixel.h>



BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

struct BluetoothSett {
  char Service_UUID[64];
  char Characteristic_UUID[64];
};


enum class Board_Pin
{
  led = 13,
  strip = 14,
  push_button = 16,
};

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
  byte color[3];
};

struct RainbowData {
  int velocity;
};
struct ColorSplitData {
  int endFirstLedSplit;
  byte color1[3];
  byte color2[3];
};

struct RGB_Color {
  byte r;
  byte g;
  byte b;
};


#pragma region Declarations
Adafruit_NeoPixel led(1, int(Board_Pin::led), NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip(30, int(Board_Pin::strip), NEO_GRB + NEO_KHZ800);
BluetoothSett bluetoothSett;
char ID_DEVICE[64];
Mode_Type mode;

DefaultData defaultData;
FixedColorData fixedColorData;
RainbowData rainbowData;
ColorSplitData colorSplitData;
#pragma endregion Declarations



// Loop Functions
#pragma region LoopFunctions
void fixed_color(Adafruit_NeoPixel & strip, DefaultData dData, FixedColorData& data) {
  strip.fill(Adafruit_NeoPixel::Color(data.color[0], data.color[1], data.color[2]), 0, dData.ledLenght);
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
  firstPixelHue = int(((int(millis() * 3 * rainbowData.velocity / 100) % 1000) / 1000.0) * 65535L);

  for (int i = 0; i < strip.numPixels(); i++)
  {
    int pixelHue = firstPixelHue + (i * hueDifference);
    if (pixelHue >65536)
    {
      pixelHue -= 65535;
    }
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue, 255,defaultData.brightness)));
  }
}

void color_split(Adafruit_NeoPixel & strip, DefaultData dData, ColorSplitData& data) {
  // Set First Color
  strip.fill(
    Adafruit_NeoPixel::Color(
      data.color1[0],
      data.color1[1],
      data.color1[2]),
    0,
    data.endFirstLedSplit
  );

  // Set Second Color
  strip.fill(
    Adafruit_NeoPixel::Color(
      data.color2[0],
      data.color2[1],
      data.color2[2]),
    data.endFirstLedSplit
  );
}
#pragma endregion LoopFuctions



class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

#pragma region CallbackSetMods

void setDefaultSettings(const byte *val, int firstIndex){
  int ledLenghtIndex = firstIndex;
  if(val[ledLenghtIndex]) {
    defaultData.ledLenght = val[ledLenghtIndex];
    strip.clear();
    strip.updateLength(defaultData.ledLenght);
  }

  int brightnessIndex = firstIndex + 1;
  defaultData.brightness = val[brightnessIndex];
  strip.setBrightness(defaultData.brightness);

  Serial.println("************Default***********");
  Serial.println("LedLenght: " + String(defaultData.ledLenght));
  Serial.println("Brightness: " + String(defaultData.brightness));
  Serial.println("******************************");
}

void setFixedColorData(const byte *val, int firstIndex){
  int colorIndex = firstIndex;
  fixedColorData.color[0] = val[colorIndex];
  fixedColorData.color[1] = val[colorIndex + 1];
  fixedColorData.color[2] = val[colorIndex + 2];


  Serial.println("********FixedColor Mod********");
  Serial.println(
    "Color: " +
    String(fixedColorData.color[0]) + "," +
    String(fixedColorData.color[1]) + "," +
    String(fixedColorData.color[2])
  );
  Serial.println("******************************");
}

void setRainbowData(const byte *val, int firstIndex) {
  int velocityIndex = firstIndex;
  rainbowData.velocity = val[velocityIndex];

  Serial.println("**********Rainbow Mod*********");
  Serial.println("Velocity: " + String(rainbowData.velocity));
  Serial.println("******************************");
}

void setColorSplitData(const byte *val, int index) {
  int endFirstLedSplitIndex = index;
  colorSplitData.endFirstLedSplit = val[endFirstLedSplitIndex];

  int color1Index = index + 1;
  colorSplitData.color1[0] = val[color1Index];
  colorSplitData.color1[1] = val[color1Index + 1];
  colorSplitData.color1[2] = val[color1Index + 2];


  int color2Index = index + 4;
  colorSplitData.color2[0] = val[color2Index];
  colorSplitData.color2[1] = val[color2Index + 1];
  colorSplitData.color2[2] = val[color2Index + 2];

  Serial.println("********ColorSplit Mod********");
  Serial.println("FirstSplitLenght: " + String(colorSplitData.endFirstLedSplit));
  Serial.println("SecondSplitLenght: " + String(defaultData.ledLenght - colorSplitData.endFirstLedSplit));
  Serial.println("Color1: " + String(colorSplitData.color1[0]) + "," + String(colorSplitData.color1[1]) + "," + String(colorSplitData.color1[2]));
  Serial.println("Color2: " + String(colorSplitData.color2[0]) + "," + String(colorSplitData.color2[1]) + "," + String(colorSplitData.color2[2]));
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
    mode = Mode_Type::fixed_color;
    break;
  case (int)Mode_Type::rainbow:
    setRainbowData(data, 1);
    mode = Mode_Type::rainbow;
    break;
  case (int)Mode_Type::color_split:
    setColorSplitData(data, 1);
    mode = Mode_Type::color_split;
    break;
  default:
    break;
  }
}

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
  sett["DefaultData"]["ledLenght"] = defaultData.ledLenght;
  sett["DefaultData"]["brightness"] = defaultData.brightness;

  sett["FixedColorData"]["color"][0] = fixedColorData.color[0];
  sett["FixedColorData"]["color"][1] = fixedColorData.color[1];
  sett["FixedColorData"]["color"][2] = fixedColorData.color[2];

  sett["RainbowData"]["velocity"] = rainbowData.velocity;

  sett["ColorSplitData"]["endFirstLedSplit"] = colorSplitData.endFirstLedSplit;
  sett["ColorSplitData"]["color1"][0] = colorSplitData.color1[0];
  sett["ColorSplitData"]["color1"][1] = colorSplitData.color1[1];
  sett["ColorSplitData"]["color1"][2] = colorSplitData.color1[2];

  sett["ColorSplitData"]["color2"][0] = colorSplitData.color1[0];
  sett["ColorSplitData"]["color2"][1] = colorSplitData.color1[1];
  sett["ColorSplitData"]["color2"][2] = colorSplitData.color1[2];

  Serial.println(String((int)mode));

  sett["mode"] = (byte)mode;

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
  }
};


bool SPIFFS_init() {
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return false;
  }
  return true;
}


void setup() {
  Serial.begin(115200);

  Serial.println("BEGIN");

  delay(5000);

  // Initialize SPIFFS
  if(!SPIFFS_init()) return;

  Serial.println("SPIFFS Initialized");

  // Open File Settings
  File file = SPIFFS.open("/settings.json", "r");

  Serial.println("File opened");

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


  //-------- retrive settings --------//

  Serial.println("Starting Retriving");


  // strlcpy(
  //   ID_DEVICE,
  //   sett["id_device"],
  //   sizeof(ID_DEVICE)
  // );

  Serial.println("id_device Retrived");

  // SERVICE_UUID
  strlcpy(bluetoothSett.Service_UUID,
    sett["SERVICE_UUID"] | "example.com",
    sizeof(bluetoothSett.Service_UUID)
  );
  Serial.println("Service_UUID: " + String(bluetoothSett.Service_UUID));

  // CHARACTERISTIC_UUID
  strlcpy(bluetoothSett.Characteristic_UUID,
    sett["CHARACTERISTIC_UUID"] | "example.com",
    sizeof(bluetoothSett.Characteristic_UUID)
  );
  Serial.println("Characteristic_UUID: " + String(bluetoothSett.Characteristic_UUID));

  // DefaultData
  defaultData.ledLenght     = sett["DefaultData"]["ledLenght"];
  defaultData.brightness    = sett["DefaultData"]["brightness"];
  Serial.println("defaultData.ledLenght: " + String(defaultData.ledLenght));
  Serial.println("defaultData.brightness: " + String(defaultData.brightness));

  // FixedColorData
  fixedColorData.color[0] = sett["FixedColorData"]["color"][0];
  fixedColorData.color[1] = sett["FixedColorData"]["color"][1];
  fixedColorData.color[2] = sett["FixedColorData"]["color"][2];
  Serial.println(
    "fixedColorData.color: " +
    String((int)sett["FixedColorData"]["color"][0]) + "," +
    String((int)sett["FixedColorData"]["color"][1]) + "," +
    String((int)sett["FixedColorData"]["color"][2])
  );


  // RainbowData
  rainbowData.velocity = sett["RainbowData"]["velocity"];
  Serial.println("rainbowData.velocity: " + String(rainbowData.velocity));

  // ColorSplitData
  colorSplitData.endFirstLedSplit = sett["ColorSplitData"]["endFirstLedSplit"];

  colorSplitData.color1[0] = sett["ColorSplitData"]["color1"][0];
  colorSplitData.color1[1] = sett["ColorSplitData"]["color1"][1];
  colorSplitData.color1[2] = sett["ColorSplitData"]["color1"][2];

  colorSplitData.color1[0] = sett["ColorSplitData"]["color2"][0];
  colorSplitData.color1[1] = sett["ColorSplitData"]["color2"][1];
  colorSplitData.color1[2] = sett["ColorSplitData"]["color2"][2];

  Serial.println("colorSplitData.endFirstLedSplit: " + String(colorSplitData.endFirstLedSplit));
  Serial.println(
    "colorSplitData.color1: " +
    String(colorSplitData.color1[0]) + "," +
    String(colorSplitData.color1[1]) + "," +
    String(colorSplitData.color1[2])
  );

  Serial.println(
    "colorSplitData.color2: " +
    String(colorSplitData.color1[0]) + "," +
    String(colorSplitData.color1[1]) + "," +
    String(colorSplitData.color1[2])
  );

  // Mode
  switch ((int)sett["mode"])
  {
  case (int)Mode_Type::fixed_color:
    mode = Mode_Type::fixed_color;
    break;
  case (int)Mode_Type::rainbow:
    mode = Mode_Type::rainbow;
    break;
  case (int)Mode_Type::color_split:
    mode = Mode_Type::color_split;
    break;
  default:
    mode = Mode_Type::fixed_color;
    break;
  }
  Serial.println(String((int)sett["mode"]));

  //-------- Initialize BLE Operations --------//

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(bluetoothSett.Service_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      bluetoothSett.Characteristic_UUID,
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
  pAdvertising->addServiceUUID(bluetoothSett.Service_UUID);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  // pAdvertising->setMinPreferred(0x06);
  // pAdvertising->setMinPreferred(0x12);
  // pAdvertising->setMinInterval(1);
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");


  led.begin();
  led.show();


  pinMode(int(Board_Pin::push_button), INPUT);
  file.close();
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