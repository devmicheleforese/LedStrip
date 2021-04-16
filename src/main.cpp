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
#include "functions/functions.hpp"



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

int brightness = 255;

uint32_t color = 0;

const String ID_DEVICE = "StripLed_v1";

enum class Mode_Type : byte{
  fixed_color = 1,
  rainbow = 2,
  sinfun = 3,
} mode;

const int DEVICE_MODES_LENGHT = 3;

byte DEVICE_MODES[DEVICE_MODES_LENGHT] = {
  (int)Mode_Type::fixed_color,
  (int)Mode_Type::rainbow,
  (int)Mode_Type::sinfun,
};





class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

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


class DeviceCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    brightness = value[0];
    color = Adafruit_NeoPixel::Color(value[1], value[2], value[3]);


    //RGB_Color c = hex_to_rgb(hex2int(value.c_str()));

    //color = Adafruit_NeoPixel::Color(c.r, c.g, c.b);

    //brightness = String(value.c_str()).toInt();

    if(value.length() > 0) {

      Serial.println("******************************");
      Serial.println("Message: ");
      Serial.println("Lenght: ");
      Serial.println(value.length());

      Serial.println("Brightness: " + String((int)value[0]));
      Serial.println("Red: " + String((int)value[1]));
      Serial.println("Green: " + String((int)value[2]));
      Serial.println("Blue: " + String((int)value[3]));

      Serial.println("******************************");
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
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");


  led.begin();
  led.show();


  pinMode(int(Board_Pin::push_button), INPUT);

  // mode = Mode_Type::rainbow;
}

void loop() {
    // // notify changed value
    // if (deviceConnected) {
    //     pCharacteristic->setValue((uint8_t*)&value, 4);
    //     pCharacteristic->notify();
    //     value++;
    //     delay(10); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    // }
    // disconnecting
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


  // // Loop mods
  // if (digitalRead(int(Board_Pin::push_button)) == 0)
  // {
  //   if(mode == Mode_Type::sinfun) {
  //     mode = Mode_Type::rainbow;
  //   } else {
  //     mode = Mode_Type(int(mode) + 1);
  //   }
  //   while(digitalRead(int(Board_Pin::push_button)) == 0) ;
  // }

  // Serial.println(digitalRead(int(Board_Pin::push_button)));

  fixed_color(strip, color, brightness);

  // switch (mode)
  // {
  // case Mode_Type::rainbow:
  //   rainbow(strip);
  //   break;
  // case Mode_Type::fixed_color:
  //   fixed_color(strip, color, brightness);
  //   break;
  // default:
  //   break;
  // }
  strip.show();
  delay(5);
}