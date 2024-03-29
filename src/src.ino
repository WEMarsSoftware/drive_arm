#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <SPIFFS.h>;



#include "CommunicationStuff.hh"
#include "GamepadWebsocket.h"
#include "Electrical.hh"
#include "PreprocessorOptions.hh"
#include "esp32-hal-ledc.h"

// required for hal-ledc
const int LEFT_DRIVE_CHANNELS[] = {1, 2, 3};
const int RIGHT_DRIVE_CHANNELS[] = {4, 5, 6};

const int LEFT_DRIVE_PINS[] = {15, 2, 4};
const int RIGHT_DRIVE_PINS[] = {16, 17, 5};

const int NUM_MOTORS_PER_SIDE = 3;

int armPositionTimer;


// required for AsynchronousWebServer to run on alternate core
TaskHandle_t Task1;

// for timer interrupts
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
int lastPingVal = 0;

// Stops motors if we lost connection
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  
#ifdef DEBUG
  Serial.println(F("On timer"));
  Serial.println(lastPingVal - GPWnumPings);
#endif
  
  if (lastPingVal == GPWnumPings) {
    moveMotors(0, 0); // TURN MOTORS OFF -> WE LOST CONNECTION
  }
  lastPingVal = GPWnumPings;
  
  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
#endif

  

  // setup electrical stuff
  for (int i = 0; i < NUM_MOTORS_PER_SIDE; i++) {
    setupElec(LEFT_DRIVE_PINS[i], LEFT_DRIVE_CHANNELS[i]);
    setupElec(RIGHT_DRIVE_PINS[i], RIGHT_DRIVE_CHANNELS[i]);

    setDriveChannel(i, LEFT_DRIVE_CHANNELS[i]);
    setDriveChannel(i+3, RIGHT_DRIVE_CHANNELS[i]);
  }

  
  // run WiFi server and control motor PWM outputs (CORE 0 - secondary core)
  
   /**xTaskCreatePinnedToCore(
            setupESPServer, // Function 
            "ServerTask",   // Name 
            10000,          // Stack size 
            NULL,           // Parameter of function 
            1,              // Priority - 0 since this should override timer and sensor ISR's 
            &Task1,         // Task handle 
            0               // Core 
            );**/
  
  connectToWiFi();
  setupESPServer(NULL);
  
  // timer interrupt to check for connection loss
  // runs once per second
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 2000000, true);
  timerAlarmEnable(timer);

  // run sensor ISR on main core
   SensorController::setupSensors(nullptr);

   armPositionTimer = millis();
}

void loop() 
  { 
    //update every 50ms
    if(millis()-armPositionTimer > 50){
      armPositionTimer = millis();
      armControl();
    }
  }
