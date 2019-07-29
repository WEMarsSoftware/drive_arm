// SensorController.hh

#ifndef SENSOR_CONTROLLER
#define SENSOR_CONTROLLER

//#define ARMCONTROLLER 1

#include "Arduino.h"
#include "ESP32Encoder.h"
#include "SPI.h"

// Note: different than Arduino
const int MAX_ANALOG_IN = 4095;
const int MIN_ANALOG_IN = 0;

// CURRENT SENSORS
// varies linearly with DC in
const int NUM_CHASSIS_MOTORS = 6;

const int MAX_CURRENT_IN = 33;

const int spiClk = 1000000; // 1 MHz

//uninitalised pointers to SPI objects
#ifdef ARMCONTROLLER   
  SPIClass * vspi = NULL;
#endif
SPIClass * hspi = NULL;

// code running on Core #0
class SensorController
{
public:
	// let WiFi callback access these directly
	static int currentValues[NUM_CHASSIS_MOTORS];
	static int speedValues[NUM_CHASSIS_MOTORS];

	// RotaryEncoder library will count number of ticks in specified time length
	// specified in constructor (last parameter in microseconds)
	static ESP32Encoder encoders[NUM_CHASSIS_MOTORS];
	static int deltaTicks[NUM_CHASSIS_MOTORS];
  static int potVals[NUM_CHASSIS_MOTORS];
  
	// pin assignments
	static int A_PINS[NUM_CHASSIS_MOTORS];
	static int B_PINS[NUM_CHASSIS_MOTORS];
	static int CURRENT_IN[NUM_CHASSIS_MOTORS];

  // SPI constants
  static const int HSPI_CLK;
  static const int HSPI_MISO;
  static const int HSPI_MOSI;
  static const int HSPI_CS_CURR;
  static const int HSPI_CS_IO;
  
  static const int VSPI_CLK;
  static const int VSPI_MISO;
  static const int VSPI_MOSI;
  static const int VSPI_CS_POT;
  static const int VSPI_EOC_POT;

  static const int CAN_R;
  static const int CAN_D;
  
	// constants
  static const int CORE_LOOP_DELAY;
  static const int ENCODER_TIME;

	// main infinite loop to update data points
	static void sensorsCoreLoop();

	// attach encoders to pins
	static void setupSensors(void* args);
#ifdef ARMCONTROLLER 	
  static void potSPICmd();
#endif
  static void CurrentSPICmd();
};

// link statics
int SensorController::currentValues[NUM_CHASSIS_MOTORS] = {};
int SensorController::speedValues[NUM_CHASSIS_MOTORS] = {};
int SensorController::potVals[NUM_CHASSIS_MOTORS] = {};
ESP32Encoder SensorController::encoders[NUM_CHASSIS_MOTORS];

// pin assignments temporary
int SensorController::A_PINS[NUM_CHASSIS_MOTORS] = {36, 34, 32, 25, 26, 19};
int SensorController::B_PINS[NUM_CHASSIS_MOTORS] = {39, 35, 33, 23, 27, 18};
int SensorController::CURRENT_IN[NUM_CHASSIS_MOTORS] = {9, 10, 11, 19, 18, 5};
int SensorController::deltaTicks[NUM_CHASSIS_MOTORS] = {};

// SPI constants
const int SensorController::HSPI_CLK = 14;
const int SensorController::HSPI_MISO = 12;
const int SensorController::HSPI_MOSI = 13;
const int SensorController::HSPI_CS_CURR = 1;
const int SensorController::HSPI_CS_IO = 3;

const int SensorController::VSPI_CLK = 18;
const int SensorController::VSPI_MISO = 19;
const int SensorController::VSPI_MOSI = 23;
const int SensorController::VSPI_CS_POT = 26;
const int SensorController::VSPI_EOC_POT = 27;

const int SensorController::CAN_R = 22;
const int SensorController::CAN_D = 21;

// constants
const int SensorController::CORE_LOOP_DELAY = 10;
const int SensorController::ENCODER_TIME = 1000;

// temp array for reading encoder ticks
int deltaTicks[NUM_CHASSIS_MOTORS] = {};
const int CORE_LOOP_DELAY = 10;
const int ENCODER_TIME = 1000;

void SensorController::sensorsCoreLoop()
{
	while (true)
	{
   
#ifdef ARMCONTROLLER   
    potSPICmd();
#else
		// update encoder and current sensor data
   //max speed 1.2khz = 1.2m/s
   //min speed 12hz = .012m/s
   digitalWrite(21,HIGH);
		for (int i = 0; i < NUM_CHASSIS_MOTORS; i++)
		{
			deltaTicks[i] = encoders[i].getCountRaw();
      encoders[i].clearCount();
			speedValues[i] = (double)(deltaTicks[i])/10;
			currentValues[i] = analogRead(CURRENT_IN[i]);
		}
   digitalWrite(21,LOW);
   
#endif
    CurrentSPICmd();

		delay(CORE_LOOP_DELAY);
  } 
}

void SensorController::setupSensors(void* args)
{
	// create RotaryEncoder objects
   pinMode(21, OUTPUT); //set up spark power relay  
	for (int i = 0; i < NUM_CHASSIS_MOTORS; i++)
	{
		
     encoders[i].attachHalfQuad(A_PINS[i], B_PINS[i]);
      
	}

   // initialise two instances of the SPIClass attached to VSPI and HSPI respectively
  #ifdef ARMCONTROLLER   
    vspi = new SPIClass(VSPI);
  #endif
  hspi = new SPIClass(HSPI);
  
  // clock miso mosi ss

  // initialise
 #ifdef ARMCONTROLLER 
  vspi->begin(VSPI_CLK, VSPI_MISO, VSPI_MOSI, VSPI_CS_POT);
  vspi->setHwCs(false);
#endif
  hspi->begin(HSPI_CLK, HSPI_MISO, HSPI_MOSI, HSPI_CS_CURR);
  hspi->setHwCs(false);

  //set up slave select pins as outputs
 #ifdef ARMCONTROLLER 
  pinMode(VSPI_CS_POT, OUTPUT);
 #endif
  pinMode(HSPI_CS_CURR, OUTPUT);
  pinMode(HSPI_CS_IO, OUTPUT);
  
	// don't let this task end
	sensorsCoreLoop();
}

#ifdef ARMCONTROLLER 
// Update data from pots
void SensorController::potSPICmd() {
  byte address = 0; // junk data to illustrate usage

  // query MAX11628EEE ADC for 8 channels of data
  for (; address < NUM_CHASSIS_MOTORS; address++) {
    
    vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    digitalWrite(VSPI_CS_POT, LOW); // select chip
 
    vspi->transfer(address);  

    // see this thread about reading returned value http://forum.arduino.cc/index.php?topic=260836.0
    byte returnedValue = vspi->transfer(0);
    potVals[address] = (int)returnedValue;
    
    digitalWrite(VSPI_CS_POT, HIGH); // signal end of transfer
    vspi->endTransaction();
    
  }
}
#endif

// Update data from current sensors
void SensorController::CurrentSPICmd() {
  byte address = 0;
  for (; address < NUM_CHASSIS_MOTORS; address++) {  
    hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    digitalWrite(HSPI_CS_CURR, LOW);
    hspi->transfer(address);

    // see this thread about reading returned value http://forum.arduino.cc/index.php?topic=260836.0
    byte retVal = hspi->transfer(0);
    currentValues[address]= (int)retVal;

    digitalWrite(HSPI_CS_CURR, HIGH);
    hspi->endTransaction();
  }
}

#endif
