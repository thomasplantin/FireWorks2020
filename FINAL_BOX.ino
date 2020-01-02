/*  January 2020
 *  This code contains the logic of the terminal box (or the output to fireworks) 
 *  to communicate with the detonator in order to effectively ignite 
 *  fireworks from a distance while maximizing the safety of the user.
 *  
 *  DISCLAIMER: PER THE LICENSE BELOW, I AM NOT RESPONSIBLE FOR ANYTHING 
 *  THAT ANYONE MIGHT USE THIS SOFTWARE FOR.
 
  MIT License

  Copyright (c) [2020] [Thomas Plantin]
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 */

// Include the libraries
#include <SPI.h>
#include "RF24.h"

// Set up the transceiver
byte addresses[][6] = {"1Node","2Node"};
RF24 radio(9, 10); // NRF24L01 used SPI pins + Pin 9 and 10 on the NANO
bool radioNumber = 0;

// Define the fuse igniters to be activated.
#define fuse1 2
#define fuse2 3
#define fuse3 4

// Define the probes to test the state of the fuse igniters.
#define lineTest1 A2
#define lineTest2 A3
#define lineTest3 A4

// Define the probe to test the overall state of the box.
#define armed 5

// Define the LED strip.
#define LEDStrip 6

// Variables for LED strip fading
unsigned long previousMillisOnLEDStrip = 0;
unsigned long timeOutOnLEDStrip = 20;
unsigned long previousMillisOffLEDStrip = 0;
unsigned long timeOutOffLEDStrip = 750;
int brightnessLEDStrip = 0;
int fadeAmountLEDStrip = 1;
bool turnedOffLEDStrip = false;

// Variables for fire mode logic
bool inFireMode = false;

// The threshold for 'analogRead()' when performing line test.
const int lineTestThreshold = 700;

// Define the payload to be transmitted and received.
struct Payload{
  int value;
  int randomValues[8];
}myData, incomingData;

void setup() {

  Serial.begin(9600);
  
  pinMode(fuse1, OUTPUT);
  pinMode(fuse2, OUTPUT);
  pinMode(fuse3, OUTPUT);
  pinMode(lineTest1, INPUT);
  pinMode(lineTest2, INPUT);
  pinMode(lineTest3, INPUT);
  pinMode(armed, INPUT);
  pinMode(LEDStrip, OUTPUT);

  // Signal boot up
  blinkStripBootUp();

  // Begin the radio communication between the two modules.
  radio.begin();
  radio.setPALevel(RF24_PA_MIN);
  radio.setDataRate(RF24_250KBPS);
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  if(radioNumber){
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
  }else{
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
  }
  // Start the radio listening for data
  radio.startListening();
}

void loop() {

  if(radio.available()){
      while (radio.available()) { // While there is data available
        radio.read( &incomingData, sizeof(incomingData) );  // Collect the payload
      }

      if(incomingData.value == 0) { 
        // If command to exit fire mode
        Serial.println("Command => Exit Fire Mode");
        inFireMode = false; // Exit Fire Mode
        digitalWrite(LEDStrip, LOW);
        myData.value = 1000;
      }

      else if(incomingData.value == 123) { 
        // If command to test the connection
        Serial.println("Command => Test Connection");
        // Copy the incomingData array to myData array
        for(int i = 0; i < 8; i++) {
          myData.randomValues[i] = incomingData.randomValues[i]; // Send back the random values you received.
        }
      }

      else if(incomingData.value == 4) { 
        // If the armed status is requested
        Serial.println("Prompt => Armed Status?");
        if(digitalRead(armed)){
          Serial.println("Resp => Switch Armed");
          myData.value = 41;
        }
        else {
          myData.value = 40;
          Serial.println("Resp => Switch Unarmed");
        }
      }

      else if(incomingData.value == 1) { 
        // If request line test 1
        Serial.println("Prompt => Test Line 1?");
        if(digitalRead(armed) && analogRead(lineTest1) > lineTestThreshold) { 
          // If igniter 1 is connected
          Serial.println("Resp => Line 1 ON");
          myData.value = 11;
        }
        else { 
          // If igniter 1 is disconnected
          Serial.println("Resp => Line 1 OFF");
          myData.value = 10;
        }
      }

      else if(incomingData.value == 2) { 
        // If request line test 2
        Serial.println("Prompt => Test Line 2?");
        if(digitalRead(armed) && analogRead(lineTest2) > lineTestThreshold) { 
          // If igniter 2 is connected
          Serial.println("Resp => Line 2 ON");
          myData.value = 21;
        }
        else { 
          // If igniter 2 is disconnected
          Serial.println("Resp => Line 2 OFF");
          myData.value = 20;
        }
      }

      else if(incomingData.value == 3) { 
        // If request line test 3
        Serial.println("Prompt => Test Line 3?");
        if(digitalRead(armed) && analogRead(lineTest3) > lineTestThreshold) { 
          // If igniter 3 is connected
          Serial.println("Resp => Line 3 ON");
          myData.value = 31;
        }
        else { 
          // If igniter 3 is disconnected
          Serial.println("Resp => Line 3 OFF");
          myData.value = 30;
        }
      }

      else if(incomingData.value == 5) { 
        // If command to set fire mode ON.
        Serial.println("Command => Set Fire Mode ON");
        inFireMode = true;
        myData.value = 51;
      }

      else if(incomingData.value == 100) { 
        // If commmand to fire fuse 1
        Serial.println("Command => Ignite Fuse 1");
        ignite(fuse1);
        myData.value = 1001;
      }
      
      else if(incomingData.value == 200) { 
        // If commmand to fire fuse 2
        Serial.println("Command => Ignite Fuse 2");
        ignite(fuse2);
        myData.value = 2001;
      }

      else if(incomingData.value == 300) { 
        // If commmand to fire fuse 3
        Serial.println("Command => Ignite Fuse 3");
        ignite(fuse3);
        myData.value = 3001;
      }

      else if(incomingData.value == 400) { 
        // If commmand to fire all fuses
        Serial.println("Command => Ignite ALL Fuses");
        // Ignite manually since different fuses at the same time.
        digitalWrite(fuse1, HIGH);
        digitalWrite(fuse2, HIGH);
        digitalWrite(fuse3, HIGH);
        delay(20);
        digitalWrite(fuse1, LOW);
        digitalWrite(fuse2, LOW);
        digitalWrite(fuse3, LOW);
        myData.value = 4001;
      }

      else { // If any unexpected payload is received.
        Serial.println("Unexpected payload received -> " + String(incomingData.value));
      }

      sendPackage(myData);
      Serial.println("Sending Data");
      
  }

  else if(inFireMode) {

    if(digitalRead(armed))
      blinkLEDStrip();

    else
      digitalWrite(LEDStrip, LOW);
    
  }
  
} // Loop

// Function to send data.
void sendPackage(Payload data) {
  radio.stopListening();   // Stop listening so we can send
  delay(10);
  radio.write(&data, sizeof(data));
  delay(10);
  radio.startListening(); // Back to listen mode
  delay(10);
}

// Function to blink the led strip.
void blinkLEDStrip() {
  unsigned long currentMillisOnLEDStrip = millis();
  unsigned long currentMillisOffLEDStrip = millis();
  if(!turnedOffLEDStrip && (currentMillisOnLEDStrip - previousMillisOnLEDStrip >= timeOutOnLEDStrip)) {
    analogWrite(LEDStrip, brightnessLEDStrip);
    brightnessLEDStrip = brightnessLEDStrip + fadeAmountLEDStrip;
    fadeAmountLEDStrip+=4;
    if(brightnessLEDStrip >= 255) {
      turnedOffLEDStrip = true;
      brightnessLEDStrip = 0;
      fadeAmountLEDStrip = 1;
      previousMillisOffLEDStrip = currentMillisOffLEDStrip;
      digitalWrite(LEDStrip, LOW);
    }
    previousMillisOnLEDStrip = currentMillisOnLEDStrip;
  }

  if(turnedOffLEDStrip && (currentMillisOffLEDStrip - previousMillisOffLEDStrip >= timeOutOffLEDStrip)) {
    turnedOffLEDStrip = false;
  }
}

// Function to ignite a fuse
void ignite(int fuse) {
  digitalWrite(fuse, HIGH);
  delay(20);
  digitalWrite(fuse, LOW);
}

// Function for signal boot up
void blinkStripBootUp() {
  for(int i = 0; i < 3; i++) {
    digitalWrite(LEDStrip, HIGH);
    delay(100);
    digitalWrite(LEDStrip, LOW);
    delay(100);
  }
}
