// // *********************************
// //  LoLa board test
// //  https://www.acmesystems.it/LOLA
// //  Rainbow effect
// // *********************************

// #include <Adafruit_NeoPixel.h>
// #include <math.h>
// #include "Arduino.h"
// #include <WiFi.h>
// #include <WiFiClient.h>
// #include <PubSubClient.h>




// WiFiClient wifi_client;
// PubSubClient mqtt_client;
// bool mqtt_status;

// void mqtt_callback(const char* topic, byte* payload, unsigned int length) {
//   Serial.println("Message arrived [" + String(topic) + "]");
//   for (int i=0;i<length;i++) {
//     Serial.print((char)payload[i]);
//   }
//   Serial.println();
// }

// int WIFI_init() {
//   Serial.println("WIFI - Connection STARTING.");
//   WiFi.mode(WIFI_STA);

//   int retries = 0;


//   WiFi.begin("acmestaff", "acmesbc2021");

//   while ((WiFi.status() != WL_CONNECTED) && (retries < 100)) {
//     retries++;
//     delay(1000);
//     Serial.println("#");
//   }

//   if(WiFi.status() == WL_CONNECTED) {
//     Serial.println("WIFI - Connection SUCCESS.");
//   } else {
//     Serial.println("WIFI - Connection FAILED.");
//   }
//   return WiFi.status();
// }

// int MQTT_init(const char* mqtt_topic)
// {
//   mqtt_client.setClient(wifi_client);
//   IPAddress ip;
//   ip.fromString("192.168.1.113");
//   mqtt_client.setServer(ip, 1883);
//   mqtt_client.setCallback(mqtt_callback);

//   std::string mqtt_client_id = "Strip_Led_Client";

//   Serial.print("MQTT - Initializing communication as ");
//   Serial.println(mqtt_client_id.c_str());

//   if (mqtt_client.connect("Strip_Led_Client")) {
//     Serial.println(mqtt_client.state());
//     Serial.println("MQTT - Connection to broker SUCCESS.");
//     if(mqtt_client.subscribe(mqtt_topic)) {
//       Serial.println("MQTT - Subscription to topic [" + String(mqtt_topic) + "] SUCCESS.");
//     } else {
//       Serial.println("MQTT - Unable to subscribe to [" + String(mqtt_topic) + "] ERROR.");
//       mqtt_client.disconnect();
//     }
//   } else {
//     Serial.println("MQTT - Connection to broker ERROR.");
//     Serial.println(mqtt_client.state());
//   }
//   return mqtt_client.connected();
// }



// void setup1()
// {
//   Serial.begin(9600);
//   delay(4000);
//   Serial.println("Booting");

//   Serial.println("Start");

//   WIFI_init();

//   // WIFI - MQTT setup
//   mqtt_status = MQTT_init("/pippo");
//   delay(1000);

//   if (!mqtt_status) {
//     Serial.println("MQTT - Status ERROR");
//   } else {
//     Serial.println("MQTT - Status OK");
//     delay(1000);
//   }
// }





// void loop1()
// {


// /*
//   char payload[20];
//   static int state=0;

//   if (digitalRead(BlueButton)==LOW) {
//     Serial.println("Button pressed");
//     delay(100);

//     if (mqtt_status) {
//       if (state==0) {
//         sprintf(payload,"on");
//         state=1;
//       } else {
//         sprintf(payload,"off");
//         state=0;
//       }
//       mqtt_client.publish("giovanni/button", payload);
//       while (digitalRead(BlueButton)==LOW) {
//         delay(100);
//       }
//     } else {
//       Serial.println("Error mqtt_status in loop.");
//     }
//   }
// */


//   if (mqtt_client.connected()) {
//     mqtt_client.loop();
//   } else {
//     Serial.println("MQTT - Not Connected.");
//   }
// }
