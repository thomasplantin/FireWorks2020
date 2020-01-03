/*  January 2020
 *  This code contains the logic of the detonator to communicate 
 *  with the terminal box (or the output to fireworks) in order to 
 *  effectively ignite fireworks from a distance while maximizing the 
 *  safety of the user.
 *  
 *  DISCLAIMER: PER THE LICENSE BELOW, I AM NOT RESPONSIBLE FOR ANYTHING 
 *  THAT ANYONE MIGHT USE THIS SOFTWARE FOR.
 
  MIT License

  Copyright (c) 2020 Thomas Plantin
  
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
#include <LiquidCrystal_I2C.h>

// Set up the transceiver
byte addresses[][6] = {"1Node","2Node"};
RF24 radio(9, 10); // NRF24L01 used SPI pins + Pin 9 and 10 on the NANO
bool radioNumber = 1;

//Set up the LCD screen
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// Define the safety switch
#define safetySwitch 2

// Define the main menu navigation
#define navButton A0
#define selectButton 4

// Define the trigger buttons
#define trig1 A1
#define trig2 A2
#define trig3 A3
#define trigAll 1

// Define the button lights
#define lightBut1 5
#define lightBut2 6
#define lightBut3 7
#define lightButAll 8

// Define the buzzer
#define buzzer 3

// Define menu viewing modes
bool viewingFireMode = true;
bool viewingTestConnectionMode = false;

// Define remote modes
bool fireMode = false;
bool fireModeEngaged = false;
bool testConnectionMode = false;

// Define line testing + firing logic
bool done1 = false;
bool done2 = false;
bool done3 = false;

// Define error boolean
bool errorFound = false;

// Menu blinking timing preferences
unsigned long previousMillisMenu = 0;
unsigned long timeOutMenu = 750;
bool appearingStateMenu = true;

// Communication blinking timing preferences
unsigned long previousMillisCom = 0;
unsigned long timeOutCom = 200;

// Define the payload to be transmitted and received.
struct Payload{
  int value;
  int randomValues[8];
}myData, incomingData;

void setup() {
  startingAnimation();
  Serial.begin(9600);
  
  lcd.begin(20, 4); //Declare the LCD with 20 columns and 4 rows
  lcd.clear();

  pinMode(safetySwitch, INPUT_PULLUP);
  pinMode(navButton, INPUT_PULLUP);
  pinMode(selectButton, INPUT_PULLUP);
  pinMode(trig1, INPUT_PULLUP);
  pinMode(trig2, INPUT_PULLUP);
  pinMode(trig3, INPUT_PULLUP);
  pinMode(trigAll, INPUT_PULLUP);
  
  pinMode(lightBut1, OUTPUT);
  pinMode(lightBut2, OUTPUT);
  pinMode(lightBut3, OUTPUT);
  pinMode(lightButAll, OUTPUT);
  pinMode(buzzer, OUTPUT);

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

  lcdClearPrint(0, 0, "Linking to box...");

  while(incomingData.value != 1000) { // While the response from box is not "reinitialized"
    requestStatus(0); // Send box reinitialization command.
    unpackPayload();
  }
  
}

void loop() {

  if(viewingFireMode) {
    displayView("fire mode");
  }

  if(viewingTestConnectionMode) {
    displayView("test connection");
  }

  if(fireMode) {
    lcdClearPrint(0, 0, "PUT SAFETY ON to");
    lcdPrint(0, 1, "begin the round.");
    while(!safetyOn()) { // While the safety is not engaged.
      // Wait...
    }
    delay(30); // Wait for the switch to debounce
    lcdClearPrint(0, 0, "Checking box status");
    delay(1500);
    requestStatus(4); // Request Armed Status
    unpackPayload(); // Unpack the response from the box.
    if(incomingData.value == 40) { 
      // If box is NOT armed
      lcdClearPrint(0, 0, "Got response:");
      delay(500);
      lcdPrint(0, 1, "Resp => Box unarmed");
      lcdPrint(0, 3, "PLEASE ARM BOX");
      delay(1000);
      bool firstSafetySwitchRead = true;
      while(incomingData.value != 41) { 
        // Loop until the box sends response that box is armed.
        
        if(safetyOn()) {
          // If safety is ON
          if(firstSafetySwitchRead) {
            // If first time safety is read, then print to the LCD
            // This is to avoid repetitive printing
            firstSafetySwitchRead = false;
            lcdClearPrint(0, 0, "Safety status = ON");
            lcdPrint(0, 1, "PLEASE ARM BOX");
            lcdPrint(0, 2, "Checking box status");
          }
          requestStatus(4); // Request Armed Status
          unpackPayload();
        }

        else if(!safetyOn()) {
          // If safety is OFF
          firstSafetySwitchRead = true;
          while(!safetyOn()) {
            // Loop until safety is switched ON
            if(firstSafetySwitchRead) {
              // If first time safety is read, then print to the LCD
              // This is to avoid repetitive printing
              firstSafetySwitchRead = false;
              lcdClearPrint(0, 0, "Put safety ON.");
            }
          }
          firstSafetySwitchRead = true;
        }
      }
      lcdClearPrint(0, 0, "Box now ARMED");
      lcdPrint(0, 1, "KEEP SAFETY ON");
      lcdPrint(0, 2, "Step away from box");
      lcdPrint(0, 3, "BEFORE proceeding.");
      delay(5000);
    }
    
    else if(incomingData.value == 41) { // If box is armed
      lcdClearPrint(0, 0, "Got response:");
      delay(500);
      lcdPrint(0, 1, "Resp => Box armed");
      delay(2000);
    }

    // Proceed if no error has been found.
    if(!errorFound) {
      // Now test the lines to make sure at least one output is connected.
      testLines();
      delay(3000); // Display the status of the lines for 3 seconds.
      bool firstSafetySwitchRead = true;
      bool refreshContent = true;
      fireModeEngaged = true;
      while(fireModeEngaged) { // At least one output is linked to a fuse

        if(!safetyOn()) { // Safety off - now formally entering fire mode
          
          if(firstSafetySwitchRead) {
            firstSafetySwitchRead = false;
            while(incomingData.value != 51) { 
              // Waiting for confirmation of fire mode command
              requestStatus(5); // Order Fire Mode
              unpackPayload();
            }
          }

          if(refreshContent) {
            refreshContent = false;
            digitalWrite(lightBut1, !done1);
            digitalWrite(lightBut2, !done2);
            digitalWrite(lightBut3, !done3);
            if(!done1 || !done2 || !done3) // If at least one of the fuses is armed
              digitalWrite(lightButAll, HIGH);
            lcdClearPrint(0, 0, "WARNING! FireMode ON");
            lcdPrint(0, 1, "Line 1 status = " + String(!done1));
            lcdPrint(0, 2, "Line 2 status = " + String(!done2));
            lcdPrint(0, 3, "Line 3 status = " + String(!done3));
          }

          if(fireModeEngaged && !done1 && !digitalRead(trig1)) {
            ensureBoxArmed();
            while(incomingData.value != 1001) {
              // Loop until confirmation that fuse 1 has been fired.
              requestStatus(100); // Order launch of fuse 1.
              unpackPayload();
            }
            done1 = true;
            refreshContent = true;
          }

          if(fireModeEngaged && !done2 && !digitalRead(trig2)) {
            ensureBoxArmed();
            while(incomingData.value != 2001) {
              // Loop until confirmation that fuse 2 has been fired.
              requestStatus(200); // Order launch of fuse 2.
              unpackPayload();
            }
            done2 = true;
            refreshContent = true;
          }

          if(fireModeEngaged && !done3 && !digitalRead(trig3)) {
            ensureBoxArmed();
            while(incomingData.value != 3001) {
              // Loop until confirmation that fuse 3 has been fired.
              requestStatus(300); // Order launch of fuse 3.
              unpackPayload();
            }
            done3 = true;
            refreshContent = true;
          }

          if(fireModeEngaged && (!done1 || !done2 || !done3) && !digitalRead(trigAll)) {
            ensureBoxArmed();
            while(incomingData.value != 4001) {
              // Loop until confirmation that ALL fuses has been fired.
              requestStatus(400); // Order launch of ALL fuses.
              unpackPayload();
            }
            done1 = true;
            done2 = true;
            done3 = true;
            refreshContent = true;
          }
          
        }

        else if(safetyOn()) {
          // If safety is switched ON
          firstSafetySwitchRead = true;
          digitalWrite(lightBut1, LOW);
          digitalWrite(lightBut2, LOW);
          digitalWrite(lightBut3, LOW);
          digitalWrite(lightButAll, LOW);
          while(safetyOn()) {
            
            if(firstSafetySwitchRead) {
              // If first time safety is read, then print to the LCD
              // This is to avoid repetitive printing
              firstSafetySwitchRead = false;
              lcdClearPrint(0, 0, "Safety status = ON");
              lcdPrint(0, 1, "Remove safety to");
              lcdPrint(0, 2, "fire.");
              while(incomingData.value != 1000) {
                // Loop until confirmation of Fire Mode exit.
                requestStatus(0);
                unpackPayload();
              }
            }
          }
          firstSafetySwitchRead = true;
          refreshContent = true;
        }

        if(done1 && done2 && done3) {
          fireModeEngaged = false;
          digitalWrite(lightBut1, LOW);
          digitalWrite(lightBut2, LOW);
          digitalWrite(lightBut3, LOW);
          digitalWrite(lightButAll, LOW);
          lcdClearPrint(0, 0, "All fuses fired.");
          lcdPrint(0, 2, "PUT SAFETY ON to");
          lcdPrint(0, 3, "terminate the round.");
          while(!safetyOn()) { // While the safety is not re-engaged.
            // Wait...
          }
          delay(30); // Wait for the switch to debounce
          while(incomingData.value != 1000) { 
            // Loop until confirmation of Fire Mode exit.
            requestStatus(0);
            unpackPayload();
          }
        }
        
      }
    }

    fireMode = false; // Exit fire mode.
    if(!errorFound) // If no error has been found.
      viewingFireMode = true; // Go to main menu.
  }

  if(testConnectionMode) {
    lcdClearPrint(0, 0, "Testing...");
    double quality = 0;
    bool correct;
    myData.value = 123; // Command to test connection.
    // Test over 20 arrays of 8 elements with random values between 0-255
    for(int i = 0; i < 20; i++){
      correct = true;
      for(int j = 0; j < 8; j++) {
        myData.randomValues[j] = random(0, 256);
      }
      sendPackage(myData); // Send the random array of integers.
      unpackPayload(); // Wait for the response.
      for(int j = 0; j < 8; j++) {
        // Check every element of the array received against every element of the array sent
        if(incomingData.randomValues[j] != myData.randomValues[j]) {
          correct = false;
          break;
        }
      }
      if(correct)
        quality++;
      delay(60);
    }
    lcdClearPrint(0, 0, "Test done.");
    lcdPrint(0, 2, String(quality/20*100) + "% Connection");
    delay(3000);
    testConnectionMode = false;
    viewingFireMode = true;
  }
  
} // Loop

// Function for tone and button animation at initialization.
// The code for this function (startingAnimation()) was copied from Experiment Boy 
// in the following video: https://www.youtube.com/watch?v=kAX5h3XODCg
// Some modifications were added.
void startingAnimation() {
  const int tempoSpeed = 50;
  int lightSelected;
  for(int i = 1; i <= 4; i++) {
    
    switch(i) {
      case 1:
        lightSelected = lightBut1;
        break;
      case 2:
        lightSelected = lightBut2;
        break;
      case 3:
        lightSelected = lightBut3;
        break;
      case 4:
        lightSelected = lightButAll;
        break;
    }
    
    digitalWrite(lightSelected, HIGH);
    
    for(int j = 0; j < 1000; j+=200) {
      int freq = i * 1000 + j;
      tone(buzzer, freq, tempoSpeed/3+7);
      delay(tempoSpeed/3+2);
    }
  }

  for(int i = 1; i <= 4; i++) {
    switch(i) {
      case 1:
        lightSelected = lightBut1;
        break;
      case 2:
        lightSelected = lightBut2;
        break;
      case 3:
        lightSelected = lightBut3;
        break;
      case 4:
        lightSelected = lightButAll;
        break;
    }
    digitalWrite(lightSelected, LOW);
    delay(tempoSpeed/2);
  }

  delay(tempoSpeed);

  for(byte i = 0; i < 3; i++) {
    tone(buzzer, 2500, tempoSpeed);
    for(int j = 1; j <= 4; j++) {
      switch(j) {
        case 1:
          lightSelected = lightBut1;
          break;
        case 2:
          lightSelected = lightBut2;
          break;
        case 3:
          lightSelected = lightBut3;
          break;
        case 4:
          lightSelected = lightButAll;
          break;
      }
      digitalWrite(lightSelected, HIGH); 
    }

    delay(tempoSpeed);

    for(int j = 1; j <= 4; j++) { 
      switch(j) {
        case 1:
          lightSelected = lightBut1;
          break;
        case 2:
          lightSelected = lightBut2;
          break;
        case 3:
          lightSelected = lightBut3;
          break;
        case 4:
          lightSelected = lightButAll;
          break;
      }
      digitalWrite(lightSelected, LOW);  
    }
    delay(tempoSpeed);
  }
}

// Function to clear and add content to the LCD
void lcdClearPrint(int x, int y, String message) {
  lcd.clear();
  lcd.setCursor(x, y);
  lcd.print(message);
}

// Function to add content to the LCD
void lcdPrint(int x, int y, String message) {
  lcd.setCursor(x, y);
  lcd.print(message);
}

// Function to diplay the appropriate view in the initial menu
void displayView(String mode) {
  lcdClearPrint(0, 0, "1 - Fire Mode");
  lcdPrint(0, 2, "2 - Test Connection");
  bool firstBlink = true;
  while(true) {

    // Monitor time spent to blink menu item appropriately
    unsigned long currentMillisMenu = millis();
    if(currentMillisMenu - previousMillisMenu >= timeOutMenu) {
      previousMillisMenu = currentMillisMenu;
      if(appearingStateMenu) {
        if(mode == "fire mode") {
          lcdPrint(0, 0, "                   ");
        }
        else if(mode == "test connection") {
          lcdPrint(0, 2, "                   ");
        }
        appearingStateMenu = false;
      }
      else {
        lcdClearPrint(0, 0, "1 - Fire Mode");
        lcdPrint(0, 2, "2 - Test Connection");
        appearingStateMenu = true;
      }
    }
    
    if(!digitalRead(navButton)) { 
      // If the navigation button is pressed
      delay(30); // Wait for button to debounce
      if(mode == "fire mode") {
        viewingFireMode = false;
        viewingTestConnectionMode = true;
      }
      else if(mode == "test connection") {
        viewingTestConnectionMode = false;
        viewingFireMode = true;
      }
      while(!digitalRead(navButton)) {
        // Wait until navButton is released
      }
      delay(30); // Wait for button to debounce
      break;
    }

    else if (!digitalRead(selectButton)) { 
      // If the selection button is pressed
      delay(30); // Wait for button to debounce
      if(mode == "fire mode") {
        viewingFireMode = false;
        fireMode = true;
      }
      else if(mode == "test connection") {
        viewingTestConnectionMode = false;
        testConnectionMode = true;
      }
      while(!digitalRead(selectButton)) {
        // Wait until selectButton is released
      }
      delay(30); // Wait for button to debounce
      break;
    }
  }
}

// Function to send data.
void sendPackage(Payload data) {
  radio.stopListening();   // Stop listening so we can send
  delay(10);
  radio.write(&data, sizeof(data));
  delay(10);
  radio.startListening(); // Back to listen mode
  delay(10);
}

// Function to check if the safety switch is on.
bool safetyOn() {
  if(digitalRead(safetySwitch)) // If safety is ON
    return true;
  if(!digitalRead(safetySwitch)) // If safety is OFF
    return false;
}

// Function to request a status from the box.
void requestStatus(int request) {
  myData.value = request; // Pass the formal parameter request to the value variable of the payload
  sendPackage(myData);
  bool firstWait = true;
  int tries = 0;
  while(!radio.available()) {
    // Loop until a response from the box is received.
    if(firstWait) {
      firstWait = false;
      lcdPrint(0, 3, "Awaiting response...");
    }
    
    if(tries >= 50) { 
      // If you have tried sending the package and 
      // waiting for a response more than 50 times.
      errorFound = true;
      lcdClearPrint(0, 0, "SYSTEM ERROR!");
      lcdPrint(0, 1, "Restart both modules");
      break; 
    }
    
    unsigned long currentMillisCom = millis();
    if(currentMillisCom - previousMillisCom >= timeOutCom) {
      // If no response has been received within the timeout
      // resend the package and increment the variable tries.
      sendPackage(myData);
      tries++;
      previousMillisCom = currentMillisCom;
    }
    
  }
}

// Function to gather response from box.
void unpackPayload() {
  while(radio.available()) { 
    //Once the response arrived, unpack the package until none left.
    radio.read(&incomingData, sizeof(incomingData)); 
  }
}

// Function to perform the line testing.
void testLines() {
  while(true) {
    lcdClearPrint(0, 0, "Testing the lines...");
    delay(2250);
    requestStatus(1); // Send request for line 1 status
    unpackPayload();
    if(incomingData.value == 11) // If line is connected
      done1 = false;
    else if(incomingData.value == 10) // If line is disconnected
      done1 = true;
  
    requestStatus(2); // Send request for line 2 status
    unpackPayload();
    if(incomingData.value == 21) // If line is connected
      done2 = false;
    else if(incomingData.value == 20) // If line is disconnected
      done2 = true;
  
    requestStatus(3); // Send request for line 3 status
    unpackPayload();
    if(incomingData.value == 31) // If line is connected
      done3 = false;
    else if(incomingData.value == 30) // If line is disconnected
      done3 = true;
  
    if(done1 && done2 && done3) { // If no lines are connected
      lcdClearPrint(0, 0, "No fuses connected.");
      lcdPrint(0, 1, "OR box NOT armed.");
      delay(2000);
      bool firstSafetySwitchRead = true;
      while(true) {

        if(safetyOn()) {
          if(firstSafetySwitchRead) {
            firstSafetySwitchRead = false;
            lcdClearPrint(0, 0, "Safety status = ON");
            lcdPrint(0, 1, "Please connect fuses");
            lcdPrint(0, 2, "AND ensure armed box");
            lcdPrint(0, 3, "Then press 'Select'");
          }
          if(!digitalRead(selectButton))
            break;
        }

        else if(!safetyOn()) {
          firstSafetySwitchRead = true;
          while(!safetyOn()) {
            if(firstSafetySwitchRead) {
              firstSafetySwitchRead = false;
              lcdClearPrint(0, 0, "PUT SAFETY ON before");
              lcdPrint(0, 1, "approaching the box");
              lcdPrint(0, 2, "to connect fuses.");
            }
          }
          firstSafetySwitchRead = true;
        }
        
      }
    }
    else {
      lcdClearPrint(0, 0, "STATUS (1=ON, 0=OFF)");
      lcdPrint(0, 1, "Line 1: " + String(!done1));
      lcdPrint(0, 2, "Line 2: " + String(!done2));
      lcdPrint(0, 3, "Line 3: " + String(!done3));
      break;
    }
  }
}

// Function to ensure that the box is armed while fire mode is engaged.
void ensureBoxArmed() {
  while(incomingData.value != 41) {
    // Loop until switch armed confirmation
    requestStatus(4);
    unpackPayload();
    if(incomingData.value == 40) {
      // If switched is not armed, exit fire mode.
      fireModeEngaged = false;
      digitalWrite(lightBut1, LOW);
      digitalWrite(lightBut2, LOW);
      digitalWrite(lightBut3, LOW);
      digitalWrite(lightButAll, LOW);
      lcdClearPrint(0, 0, "BOX IS NOT ARMED");
      lcdPrint(0, 2, "Restarting system...");
      while(incomingData.value != 1000) { 
        // While the response from box is not "reinitialized" (Same as exiting fire mode)
        requestStatus(0); // Send box reinitialization command.
        unpackPayload();
      }
      delay(5000);
      break;
    }
  }
}
