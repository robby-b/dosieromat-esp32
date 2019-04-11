#include <Arduino.h>
#include <BLEUtils.h>
#include <string.h>
#include <HX711.h>
#include <ESP32_Servo.h>


/*
Statische Klasse, dient nur dazu den Code der Waage und Motorsteuerung optisch vom Bluetooth in der Hauptklasse zu trennen
*/
class DosieromatHelper {
  public:
    /* 
    Statische Methode um manuell ein Gewicht zu wiegen
    Ausschließlich zum Debuggen und Kalibrieren der Waage genutzt 
    */
    static int weighItem(HX711& scale) {
      Serial.println("10 Sekunden um Gewicht zu platzieren!");

      for(int i = 0; i < 10; i++) {
        Serial.println(i);
        delay(1000);
      }

      int weight = (int) scale.get_units(10);

      Serial.printf("Gewicht: %d Gramm\n", weight);

      return weight;
    }

    /*
    Statische Methode die eine Nachricht über BLE an ein verbundenes Gerät (in dem Fall die App) sendet
    */
    static bool sendValueToApp(BLECharacteristic* pCharacteristic, String msg) {
      pCharacteristic->setValue(msg.c_str());
      delay(5);  
      pCharacteristic->notify(); // Send the value to the app!
      delay(5);
      Serial.print("*** Sent Message: ");
      Serial.print(msg);
      Serial.println(" ***");

      return true;
    }

    /*
    Hauptfunktion des Mikrocontrollers.
    Diese Funktion startet den Motor und wiegt konstant das derzeitige Gewicht auf der Waage.
    Ist das gewünschte Gewicht erreicht, stoppt der Motor. 
    */
    static int measureOut(HX711& scale, Servo* servo, BLECharacteristic* pCharacteristic, int desiredWeight) {
      Serial.println("Abwiegen gestartet!");
      scale.tare();
      float currentWeight = scale.get_units(20);

      while(currentWeight < desiredWeight) {
        servo->write(180);
        scale.wait_ready(10);
        Serial.printf("Current weight: %f\n", currentWeight);
        currentWeight = scale.get_units(20);
        int progress = (int) ((currentWeight/desiredWeight) * 100);
        Serial.printf("Progress: %d %%\n", progress);        
        delay(5);
      }
      servo->write(88);
      Serial.printf("Finished!\nCurrent weight: %f, desired: %d", currentWeight, desiredWeight);

      return 0;
    }

  /*
  Startet den Motor manuell für eine angegebende Anzahl an Sekunden.
  Nur während der Entwicklung genutzt.
  */
  static void startMotor(Servo* servo, BLECharacteristic* pCharacteristic, int seconds) {
    for(int i = 0; i < seconds; i++) {
      servo->write(180);
      delay(1000);
      
      int progress = (int) ((i/(float) seconds) * 100);

      DosieromatHelper::sendValueToApp(pCharacteristic, String("PROGRESS" + String(progress)));
    }

    servo->write(87);
  }
};