#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include "DebugUtils.h"

#define DEBUG

#define LED1 9
#define LED2 8
#define BUZZER 13
#define BUTTON1 3
#define BUTTON2 2
#define DFPLAYER_RX_PIN 10
#define DFPLAYER_TX_PIN 11

SoftwareSerial dfPlayerSerial(DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
DFRobotDFPlayerMini dfPlayer;
int timeSelected = 0;
int timeToWait[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 45, 60};
unsigned long timer;

void setup()
{
#ifdef DEBUG
  Serial.begin(9600);
#endif
  
  initDfPlayer();

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);

  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));
}

void loop()
{
  // Button 1 pressed (select time)
  if (digitalRead(BUTTON1) == HIGH) {
    timeSelected += 1;

    if (timeSelected > 16) {
      DEBUG_PRINT("Timer set to OFF");
      timeSelected = 0;
      timer = 0;
      digitalWrite(LED1, LOW);
      dfPlayer.playFolder(1, 100);  // SD:/01/100.mp3; Folder Name(1~99); File Name(1~255)
    } else {
      DEBUG_PRINT("Time select set to " + timeToWait[timeSelected] + " minutes");
      digitalWrite(LED1, HIGH);
      timer = millis();
      dfPlayer.playFolder(1, timeToWait[timeSelected]);
    }
  }

  // Button 2 pressed (say time remaining)
  if (digitalRead(BUTTON2) == HIGH) {
    int remainingTime = (timer + timeToWait[timeSelected]*60000 - millis()) / 60000;
    if (remainingTime = 0) {
      remainingTime = 1;
    }
    DEBUG_PRINT("Remaining time: " + remainingTime);
    dfPlayer.playFolder(1, remainingTime);
  }

  // timer running
  if (timer > 0) {
    if (millis() - timer > timeToWait[timeSelected] * 60000) {
      DEBUG_PRINT("Countdown timer elapsed");
      timer = 0;
      timeSelected = 0;
      digitalWrite(LED1, LOW);
      dfPlayer.playFolder(2, random(1, 10));
    }
  }

#ifdef DEBUG
  if (dfPlayer.available()) {
    printDfPlayerDetail(dfPlayer.readType(), dfPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
#endif
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

  dfPlayer.volume(30);  //Set volume value. From 0 to 30
  dfPlayer.play(1);  //Play the first mp3
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
