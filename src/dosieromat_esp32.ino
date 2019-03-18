/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "HX711.h"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
float txValue = 0;

const int DOUT_PIN = 19;
const int SCK_PIN = 18;

HX711 scale(DOUT_PIN, SCK_PIN);

#define SERVICE_UUID           "a964d61b-a5b3-4b5c-9e1e-65029b3d936d" 
#define CHARACTERISTIC_UUID_RX "9c731b8a-9088-48fe-8f8a-9bc071a3784b"
#define CHARACTERISTIC_UUID_TX "8d0c925d-0f4a-4c4b-bfc3-758c89282220"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("Connected");
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");

        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }

        // Serial.println();

        // Do stuff based on the command received from the app
        // if (rxValue.find("A") != -1) { 
        //   Serial.print("Turning ON!");
        //   digitalWrite(LED, HIGH);
        // }
        // else if (rxValue.find("B") != -1) {
        //   Serial.print("Turning OFF!");
        //   digitalWrite(LED, LOW);
        // }

        // Serial.println();
        Serial.println("*********");
      }
    }
};

void setup() {
  Serial.begin(115200);


  // Create the BLE Device
  BLEDevice::init("Dosieromat Proto"); // Give it a name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                      
  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();  
  Serial.println("Waiting a client connection to notify...");


/* 
HX711 SETUP
Call set_scale() with no parameter.
Call tare() with no parameter.
Place a known weight on the scale and call get_units(10).
Divide the result in step 3 to your known weight. You should get about the parameter you need to pass to set_scale().
Adjust the parameter in step 4 until you get an accurate reading. 
*/

  // delay(1000);
  // Serial.println("set scale");
  // scale.set_scale();
  // delay(1000);
  // Serial.println("Tarieren");
  // scale.tare();

  
  // erster versuch: echtes gewicht: 146g
  // ergebnis: 29737
  // faktor: 0,0049097
  // alternative: 203,678


  scale.set_scale(203.f);
  scale.tare();


  Serial.println("Ready for weighing...");

  weighItem();
}

int weighItem() {
  Serial.println("10 Sekunden um Gewicht zu platzieren!");

  for(int i = 0; i < 10; i++) {
    Serial.println(i);
    delay(1000);
  }

  int weight = (int) scale.get_units(10);

  Serial.printf("Gewicht: %d Gramm\n", weight);

  return weight;
}


void loop() {  
  if (deviceConnected) {
    Serial.println("Device connected!");
    
    // Fabricate some arbitrary junk for now...
    txValue = random(4) / 3.456; // This could be an actual sensor reading!

    // Let's convert the value to a char array:
    char txString[8]; // make sure this is big enuffz
    dtostrf(txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer
    
//    pCharacteristic->setValue(&txValue, 1); // To send the integer value
//    pCharacteristic->setValue("Hello!"); // Sending a test message
    pCharacteristic->setValue(txString);
    
    pCharacteristic->notify(); // Send the value to the app!
    Serial.print("*** Sent Value: ");
    Serial.print(txString);
    Serial.println(" ***");
  }
  // Serial.print("test");
  //delay(1000);
}
