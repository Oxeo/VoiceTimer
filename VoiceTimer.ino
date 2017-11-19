#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <LowPower.h>
//#define DEBUG
#include "DebugUtils.h"

#define LED1 9
#define BUTTON1 3
#define BUTTON2 2
#define DFPLAYER_RX_PIN 10
#define DFPLAYER_TX_PIN 11

SoftwareSerial dfPlayerSerial(DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
DFRobotDFPlayerMini dfPlayer;
int timeSelected = 0;
int timeToWait[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 45, 60};
unsigned long timer;
unsigned long lastVoice = 0;

volatile bool button1PressedFlag = false;
volatile unsigned long button1PressedTime = millis();

volatile bool button2PressedFlag = false;
volatile unsigned long button2PressedTime = millis();

bool led1Blink = false;
int led1State = HIGH;

void setup()
{
  timeSelected = 0;
  timer = 0;
  lastVoice = 0;

  button1PressedFlag = false;
  button1PressedTime = 0;

  button2PressedFlag = false;
  button2PressedTime = 0;

  led1Blink = false;
  led1State = HIGH;

#ifdef DEBUG
  Serial.begin(9600);
#endif

  initDfPlayer();

  pinMode(LED1, OUTPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);

  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));

  attachInterrupt(digitalPinToInterrupt(BUTTON1), button1Handler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON2), button2Handler, CHANGE);

  digitalWrite(LED1, led1State);
  DEBUG_PRINT("init done");
}

void loop()
{
  // Button 1 pressed (select time)
  if (button1PressedFlag == true) {
    button1PressedFlag = false;

    timeSelected += 1;
    if (timeSelected > 16) {
      setTimerOff();
    } else {
      DEBUG_PRINT("Time select set to " + String(timeToWait[timeSelected]) + " minutes");
      led1Blink = true;
      timer = millis();
      dfPlayer.playFolder(1, timeToWait[timeSelected]);
      lastVoice = millis();
    }
  }

  // Button 1 long press
  if ((button1PressedTime != 0) && ((millis() - button1PressedTime) > 1000) && button1PressedFlag == false) {
    button1PressedTime = 0;
    setTimerOff();
  }

  // Button 2 pressed (say time remaining)
  if (button2PressedFlag == true) {
    button2PressedFlag = false;
    int remainingTime = (timer + timeToWait[timeSelected] * 60000 - millis()) / 60000;
    if (remainingTime == 0) {
      remainingTime = 1;
    }

    if (remainingTime <= 60) {
      DEBUG_PRINT("Remaining time: " + String(remainingTime));
      dfPlayer.playFolder(4, remainingTime);
    } else {
      dfPlayer.playFolder(3, 4);  // SD:/01/100.mp3; Folder Name(1~99); File Name(1~255)
    }
  }

  // timer running
  if (timer > 0) {
    long remainingTimeS = long ((timer + timeToWait[timeSelected] * 60000 - millis())) / 1000;

    // countdown finish
    if (remainingTimeS <= 0 && remainingTimeS > -3 && (millis() - lastVoice > 5000)) {
      DEBUG_PRINT("Countdown timer elapsed");
      led1Blink = false;
      dfPlayer.playFolder(3, 5);
      lastVoice = millis();
    }

    // countdown finish, play funny song after alert
    if (remainingTimeS <= -8) {
      DEBUG_PRINT("Play funny song");
      timer = 0;
      timeSelected = 0;
      dfPlayer.playFolder(2, random(1, 80));
    }

    //say time remaining auto
    if (millis() - lastVoice > 30000) {
      if (remainingTimeS <= 60 && remainingTimeS > 55) {            // 1 minutes
        DEBUG_PRINT("1 minute remaining");
        dfPlayer.playFolder(3, 1);
        lastVoice = millis();
      } else if (remainingTimeS <= 120 && remainingTimeS > 115) {   // 2 minutes
        dfPlayer.playFolder(2, random(1, 80));
        lastVoice = millis();
      } else if (remainingTimeS <= 180 && remainingTimeS > 175) {   // 3 minutes
        DEBUG_PRINT("3 minute remaining");
        dfPlayer.playFolder(3, 2);
        lastVoice = millis();
      } else if (remainingTimeS <= 300 && remainingTimeS > 295) {   // 5 minutes
        dfPlayer.playFolder(2, random(1, 80));
        lastVoice = millis();
      } else if (remainingTimeS <= 600 && remainingTimeS > 595) {   // 10 minutes
        DEBUG_PRINT("10 minute remaining");
        dfPlayer.playFolder(3, 3);
        lastVoice = millis();
      } else if (remainingTimeS <= 720 && remainingTimeS > 715) {   // 12 minutes
        dfPlayer.playFolder(2, random(1, 80));
        lastVoice = millis();
      } else if (remainingTimeS <= 1320 && remainingTimeS > 1315) { // 22 minutes
        dfPlayer.playFolder(2, random(1, 80));
        lastVoice = millis();
      }
    }
  }

  // blink led 1
  if (led1Blink == true) {
    if (led1State == LOW) {
      led1State = HIGH;
    } else {
      led1State = LOW;
    }
    digitalWrite(LED1, led1State);
  } else if (led1State == LOW) {
    led1State = HIGH;
    digitalWrite(LED1, led1State);
  }

#ifdef DEBUG
  if (dfPlayer.available()) {
    printDfPlayerDetail(dfPlayer.readType(), dfPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
#endif

#ifdef DEBUG
  delay(250);
#else
  delay(250);
//  if (timer > 0) {
//    LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);
//  } else {
//    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
//  }
#endif
}

void setTimerOff() {
  DEBUG_PRINT("Timer set to OFF");
  timeSelected = 0;
  timer = 0;
  led1Blink = false;
  dfPlayer.playFolder(1, 61);  // SD:/01/100.mp3; Folder Name(1~99); File Name(1~255)
}

void button1Handler() {
  if ((millis() - button1PressedTime) > 50) {
    if (digitalRead(BUTTON1) == LOW) {
      if (button1PressedTime != 0) {
        button1PressedFlag = true;
      }

      button1PressedTime = 0;
    } else if (digitalRead(BUTTON1) == HIGH && button1PressedTime == 0) {
      button1PressedTime = millis();
    }
  }
}

void button2Handler() {
  if ((millis() - button2PressedTime) > 50) {
    if (digitalRead(BUTTON2) == LOW) {
      if (button2PressedTime != 0) {
        button2PressedFlag = true;
      }

      button2PressedTime = 0;
    } else if (digitalRead(BUTTON2) == HIGH && button2PressedTime == 0) {
      button2PressedTime = millis();
    }
  }
}

void initDfPlayer() {
  dfPlayerSerial.begin(9600);

#ifdef DEBUG
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
#endif

  if (!dfPlayer.begin(dfPlayerSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while (true);
  }

#ifdef DEBUG
  Serial.println(F("DFPlayer Mini online."));
#endif

  dfPlayer.volume(17);  //Set volume value. From 0 to 30
  //dfPlayer.play(1);  //for debuging
  dfPlayer.playFolder(3, 7);
}

void printDfPlayerDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

