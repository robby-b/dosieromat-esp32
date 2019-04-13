#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "HX711.h"
#include <iostream>
#include <string>
#include <regex>
#include <map>
#include "DosieromatHelper.h"
#include "ESP32_Servo.h"

#define SERVICE_UUID "a964d61b-a5b3-4b5c-9e1e-65029b3d936d"
#define CHARACTERISTIC_UUID_RX "9c731b8a-9088-48fe-8f8a-9bc071a3784b"
#define CHARACTERISTIC_UUID_TX "8d0c925d-0f4a-4c4b-bfc3-758c89282220"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
float txValue = 0;

const int DOUT_PIN = 19;
const int SCK_PIN = 18;

HX711 scale;
Servo myServo;

/*
Map von allen verfügbaren Zutaten des Dosieromatsystems mit dem zugehörigen Pin
Im Prototyp nur eine Zutat Kaffee, ist also primär zur Erweiterbarkeit
*/
std::map<String, int> availableIngredients; 

class MyServerCallbacks : public BLEServerCallbacks
{ 
  // Callback wird ausgelöst, wenn sich ein Gerät mit dem BLE Server des ESP verbindet
  void onConnect(BLEServer *pServer)
  {
    Serial.println("Connected");
    deviceConnected = true;
  };

  // Callback wird ausgelöst, wenn die Verbindung zwischen Gerät und dem BLE Server des ESP getrennt wird
  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("Disconnected");
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  // splits a string at a delimiter in two parts and returns either the first part (n=0) or the second part (n=1)
  // Wird genutzt um einen Befehl von der App im Format ZUTAT;GRAMM (bspw. COFFEE;60) zu trennen
  std::string split(std::string s, std::string delimiter, int n)
  {
    size_t pos = 0;
    std::string token;

    while ((pos = s.find(delimiter)) != std::string::npos)
    {
      token = s.substr(0, pos);
      if (n == 0)
      {
        return token;
      }
      else
      {
        s.erase(0, pos + delimiter.length());
        return s;
      }
      break;
    }

    return "";
  }

  /*
  Callback wird ausgelöst wenn vom Client eine Nachricht gesendet wird
  Der Code innerhalb dient dem parsen der gesendeten Nachricht, ob bspw. ein Befehl gefunden wird.
  Darauf folgend wird dann ggf. die entsprechende Methode in der Hilfsklasse aufgerufen.
  */
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();
    Serial.println("raw value");
    Serial.println(rxValue.c_str());

    if (rxValue.length() > 0)
    {
      Serial.println("*********");

      bool matched = false;

      for (auto const &ingredient : availableIngredients)
      {
        String regexString = String("(" + ingredient.first + ");(\\d*)");

        if (std::regex_match(rxValue, std::regex(regexString.c_str())))
        {
          matched = true;
          Serial.println("Matched value:");
          Serial.println(rxValue.c_str());

          Serial.println("First");
          Serial.println(ingredient.first.c_str());
          Serial.println("Second");
          Serial.println(ingredient.second); // PIN NUMBER
          Serial.println("amount");
          int amount = std::atoi(split(rxValue, ";", 1).c_str()); // amount
          Serial.printf("Gesendete Menge: %d\n", amount);
          DosieromatHelper::measureOut(scale, &myServo, pCharacteristic, amount);
        }
      }
      if (!matched)
      {
        if (rxValue.find("WEIGH") != std::string::npos)
        {
          Serial.println("Weigh command found!");
          //DosieromatHelper::weighItem(scale);
          DosieromatHelper::measureOut(scale, &myServo, pCharacteristic, 60);
        }
        else if (rxValue.find("START_MOTOR") != std::string::npos) {
          Serial.println("Start motor command found!");
          DosieromatHelper::startMotor(&myServo, pCharacteristic, 10);
         
    
        }
        else if (rxValue.find("STOP_MOTOR") != std::string::npos) {
          Serial.println("Stop motor command found!");
        }
        else
        {
          Serial.println("NOT matched value:");
          Serial.println(rxValue.c_str());
        }
      }

      Serial.println("*********");
    }
  }
};

void setup()
{
  Serial.begin(115200);

  // Initialisierung der Waage
  scale.begin(DOUT_PIN, SCK_PIN);
  scale.set_scale(384); // Kalibrierungsfaktor durch mehrmaliges Wiegen von bekannten Gewichten bestimmt
  scale.tare();

  // Servo an Pin 15 angeschlossen
  myServo.attach(15);

  // Setzt die verfügbaren Zutaten dieses Dosieromaten, und den zugehörige Pin.
  availableIngredients["COFFEE"] = 15;

  // Create the BLE Device
  BLEDevice::init("Dosieromat Proto");

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic for writing to the client
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX,
      BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->addDescriptor(new BLE2902());

  // Create a BLE Characteristic for receiving messages from the client
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pCharacteristic->setWriteNoResponseProperty(true);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("Ready");
}

void loop()
{
  if (deviceConnected)
  {
    // Arbeit wird praktisch ausschließlich in den Callbacks erledigt, daher ist die Loop funktion bis auf Debugausgaben und delays leer
  }
  else
  {
    Serial.println("Not connected!");
    delay(2000);
  }

  delay(10); 
}

//Schüssel: 263,24g