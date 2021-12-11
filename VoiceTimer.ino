#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <LowPower.h>

#define LED1 9
#define BUTTON1 2
#define BUTTON2 3
#define DFPLAYER_CMD 4
#define DFPLAYER_BUSY 5
#define DFPLAYER_RX_PIN 10
#define DFPLAYER_TX_PIN 11

SoftwareSerial dfPlayerSerial(DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
DFRobotDFPlayerMini dfPlayer;
bool dfPlayerOn = false;
unsigned long dfPlayerOnTimer = 0;
int timeSelected = 0;
int timeToWait[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 45, 60};
unsigned long timer;
unsigned long lastVoice = 0;
unsigned long sleepForeverTimer = 0;
bool led1Blink = false;
int led1State = LOW;
unsigned long led1Timer = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("start init...");

  pinMode(LED1, OUTPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(DFPLAYER_BUSY, INPUT);
  pinMode(DFPLAYER_CMD, OUTPUT);

  initDfPlayer();
  randomSeed(analogRead(0));

  attachInterrupt(digitalPinToInterrupt(BUTTON1), button1Handler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON2), button2Handler, CHANGE);

  Serial.println("init done");
}

void loop()
{ 
  // Button 1 pressed
  if (digitalRead(BUTTON1) == LOW) {
    unsigned long pressedTime = millis();
    bool longPress = false;
  
    while (digitalRead(BUTTON1) == LOW) {
      if ((millis() - pressedTime) > 1000) {
        longPress = true;
        Serial.println("button 1 long pressed");
        setTimerOff();
        delay(2000);
      }
    }
  
    if (longPress == false) {
      Serial.println("button 1 pressed");
      timeSelected += 1;
      
      if (timeSelected > 16) {
        setTimerOff();
      } else {
        Serial.println("Time select set to " + String(timeToWait[timeSelected]) + " minutes");
        led1Blink = true;
        timer = millis();
  
        powerDfPlayer();
        dfPlayer.playFolder(1, timeToWait[timeSelected]);
        lastVoice = millis();
        delay(200);
      }
    }
  }

   // Button 2 pressed
  if (digitalRead(BUTTON2) == LOW) {
    while (digitalRead(BUTTON2) == LOW) {
    }

    Serial.println("button 2 pressed");
    int remainingTime = (timer + timeToWait[timeSelected] * 60000 - millis()) / 60000;
    if (remainingTime == 0) {
      remainingTime = 1;
    }

    if (remainingTime <= 60) {
      Serial.println("Remaining time: " + String(remainingTime));
      powerDfPlayer();
      dfPlayer.playFolder(4, remainingTime);
    } else {
      powerDfPlayer();
      dfPlayer.playFolder(3, 4);
    }

    delay(100);
  }

  // timer running
  if (timer > 0) {
    long remainingTimeS = long ((timer + timeToWait[timeSelected] * 60000 - millis())) / 1000;

    // countdown finish
    if (remainingTimeS <= 0 && remainingTimeS > -3 && (millis() - lastVoice > 5000)) {
      Serial.println("Countdown timer elapsed");
      led1Blink = false;
      powerDfPlayer();
      dfPlayer.playFolder(3, 5);
      lastVoice = millis();
    }

    // countdown finish, play funny song after alert
    if (remainingTimeS <= -8) {
      Serial.println("Play funny song");
      timer = 0;
      timeSelected = 0;
      powerDfPlayer();
      dfPlayer.playFolder(2, random(1, 80));
    }

    //say time remaining auto
    if (millis() - lastVoice > 30000) {
      if (remainingTimeS <= 60 && remainingTimeS > 55) {            // 1 minutes
        Serial.println("1 minute remaining");
        powerDfPlayer();
        dfPlayer.playFolder(3, 1);
        lastVoice = millis();
      } else if (remainingTimeS <= 120 && remainingTimeS > 115) {   // 2 minutes
        Serial.println("2 minutes remaining");
        powerDfPlayer();
        dfPlayer.playFolder(2, random(1, 80));
        lastVoice = millis();
      } else if (remainingTimeS <= 180 && remainingTimeS > 175) {   // 3 minutes
        Serial.println("3 minutes remaining");
        powerDfPlayer();
        dfPlayer.playFolder(3, 2);
        lastVoice = millis();
      } else if (remainingTimeS <= 300 && remainingTimeS > 295) {   // 5 minutes
        Serial.println("5 minutes remaining");
        powerDfPlayer();
        dfPlayer.playFolder(2, random(1, 80));
        lastVoice = millis();
      } else if (remainingTimeS <= 600 && remainingTimeS > 595) {   // 10 minutes
        Serial.println("10 minutes remaining");
        powerDfPlayer();
        dfPlayer.playFolder(3, 3);
        lastVoice = millis();
      } else if (remainingTimeS <= 720 && remainingTimeS > 715) {   // 12 minutes
        Serial.println("12 minutes remaining");
        powerDfPlayer();
        dfPlayer.playFolder(2, random(1, 80));
        lastVoice = millis();
      } else if (remainingTimeS <= 1320 && remainingTimeS > 1315) { // 22 minutes
        Serial.println("22 minutes remaining");
        powerDfPlayer();
        dfPlayer.playFolder(2, random(1, 80));
        lastVoice = millis();
      }
    }
  }

  manageDfPlayerAutoShutdown();
  manageLed();

  if (timer == 0 && (millis() - sleepForeverTimer) > 10000) {
    shutdownDfPlayer();
    sleepForever();
  }
}

void setTimerOff() {
  Serial.println("Timer set to OFF");
  timeSelected = 0;
  timer = 0;
  led1Blink = false;
  powerDfPlayer();
  dfPlayer.playFolder(1, 61);
}

void sleepForever() {
  pinMode(DFPLAYER_CMD, INPUT);
  pinMode(DFPLAYER_TX_PIN, INPUT);
  pinMode(LED1, INPUT);
  Serial.println("sleep forever");
  delay(1000);
  
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  
  Serial.println("wake up");
  pinMode(DFPLAYER_CMD, OUTPUT);
  pinMode(DFPLAYER_TX_PIN, OUTPUT);
  pinMode(LED1, OUTPUT);

  sleepForeverTimer = millis();
}

void shutdownDfPlayer() {
  if (dfPlayerOn) {
    while(digitalRead(DFPLAYER_BUSY) == LOW) {}
    dfPlayerSerial.end();
    digitalWrite(DFPLAYER_CMD, LOW);
    dfPlayerOn = false;
  }
}

void powerDfPlayer() {
  if (dfPlayerOn == false) {
    digitalWrite(DFPLAYER_CMD, HIGH);
    delay(150);
    dfPlayer.volume(20);  //first not working
    dfPlayer.volume(20);  //Set volume value. From 0 to 30
    dfPlayerOn = true;
  }

  dfPlayerOnTimer = millis();
  sleepForeverTimer = millis();
}

void initDfPlayer() {
  digitalWrite(DFPLAYER_CMD, HIGH);
  delay(1000);
  
  dfPlayerSerial.begin(9600);

  if (!dfPlayer.begin(dfPlayerSerial)) {
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  }

  Serial.println(F("DFPlayer Mini online."));

  dfPlayer.volume(20);  //Set volume value. From 0 to 30
  //dfPlayer.play(1);
  dfPlayerOn = true;
}

void button1Handler() {
}

void button2Handler() {
}

void manageDfPlayerAutoShutdown() {
  if (dfPlayerOn) {
    if (digitalRead(DFPLAYER_BUSY) == LOW) {
      dfPlayerOnTimer = millis();
    } else if ((millis() - dfPlayerOnTimer) > 10000) {
      Serial.println("DfPlayer shutdown after 10s of inactivity");
      shutdownDfPlayer();
    }
  }
}

void manageLed() {
  if (led1Blink) {
    if ((millis() - led1Timer) > 1000) {
      led1State = led1State == HIGH ? LOW : HIGH;
      digitalWrite(LED1, led1State);
      led1Timer = millis();
    }
  } else {
    if (led1State == HIGH) {
      led1State = LOW;
      digitalWrite(LED1, led1State);
    }
  }
}
