// SensorController.hh

#ifndef SENSOR_CONTROLLER
#define SENSOR_CONTROLLER

//#define ARMCONTROLLER 1
#define GPIO 1
#define DRIVECONTROLLER 1


#include "Arduino.h"
#include "ESP32Encoder.h"
#include "SPI.h"
#include "GamepadWebsocket.h"

// Note: different than Arduino
const int MAX_ANALOG_IN = 4095;
const int MIN_ANALOG_IN = 0;

// CURRENT SENSORS
// varies linearly with DC in
const int NUM_CHASSIS_MOTORS = 6;

const int MAX_CURRENT_IN = 20;

const int spiClk = 1000000; // 1 MHz

//uninitalised pointers to SPI objects
#ifdef ARMCONTROLLER   
  SPIClass * vspi = NULL;
#endif
SPIClass * hspi = NULL;

void writeServer(String message);

// code running on Core #0
class SensorController
{
public:
	// let WiFi callback access these directly
	static int currentValues[NUM_CHASSIS_MOTORS];
	static int speedValues[NUM_CHASSIS_MOTORS];
  static int siCurrentsResetByte;// bit = 0 motor 0 etc.
	// RotaryEncoder library will count number of ticks in specified time length
	// specified in constructor (last parameter in microseconds)
	static ESP32Encoder encoders[NUM_CHASSIS_MOTORS];
	static int deltaTicks[NUM_CHASSIS_MOTORS];
  static int potVals[NUM_CHASSIS_MOTORS];
  
	// pin assignments
	static int A_PINS[NUM_CHASSIS_MOTORS];
	static int B_PINS[NUM_CHASSIS_MOTORS];
	//static int CURRENT_IN[NUM_CHASSIS_MOTORS];

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
  static void CurrentResetCmd();
};

// link statics
int SensorController::siCurrentsResetByte = 0;
int SensorController::currentValues[NUM_CHASSIS_MOTORS] = {};
int SensorController::speedValues[NUM_CHASSIS_MOTORS] = {};
int SensorController::potVals[NUM_CHASSIS_MOTORS] = {};
ESP32Encoder SensorController::encoders[NUM_CHASSIS_MOTORS];

// pin assignments temporary
int SensorController::A_PINS[NUM_CHASSIS_MOTORS] = {36, 34, 32, 25, 26, 19};
int SensorController::B_PINS[NUM_CHASSIS_MOTORS] = {39, 35, 33, 23, 27, 18};
//int SensorController::CURRENT_IN[NUM_CHASSIS_MOTORS] = {9, 10, 11, 19, 18, 5};
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
const int SensorController::CORE_LOOP_DELAY = 9;
const int SensorController::ENCODER_TIME = 1000;

// temp array for reading encoder ticks
int deltaTicks[NUM_CHASSIS_MOTORS] = {};
const int CORE_LOOP_DELAY = 10;
const int ENCODER_TIME = 1000;


const int SIGNAL_LIGHT = 22;
int signalLightCounter = 0;
bool signalLight = false;
bool gp_connected = false;

void SensorController::sensorsCoreLoop()
{
	while (true)
	{

  signalLightCounter++;
  if(signalLightCounter > 100){
    signalLightCounter = 0;


    //Serial.print(gp_connected);
    //if websocket connected
    if(gp_connected)
    {
      if(signalLight){
        digitalWrite(SIGNAL_LIGHT, LOW);
      }
      else{
        digitalWrite(SIGNAL_LIGHT,HIGH);
      }
      signalLight = !signalLight; //invert status
    }
  }
  //
   
#ifdef ARMCONTROLLER   
    potSPICmd();
#else
		// update encoder and current sensor data
   //max speed 1.2khz = 1.2m/s
   //min speed 12hz = .012m/s
    #ifdef DRIVECONTROLLER
		for (int i = 0; i < NUM_CHASSIS_MOTORS; i++)
		{
			deltaTicks[i] = encoders[i].getCountRaw();
      encoders[i].clearCount();
			speedValues[i] = (double)(deltaTicks[i])/10;
			//currentValues[i] = analogRead(CURRENT_IN[i]);
		}
   #endif
   
#endif
    CurrentSPICmd();
#ifdef GPIO    
    CurrentResetCmd();
#endif

		delay(CORE_LOOP_DELAY);
  } 
}

void SensorController::setupSensors(void* args)
{
	// create RotaryEncoder objects
 
	#ifdef DRIVECONTROLLER
	for (int i = 0; i < NUM_CHASSIS_MOTORS; i++)
	{
     encoders[i].attachHalfQuad(A_PINS[i], B_PINS[i]);
	}
  #endif

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


  //setup:   HSPI A/d  set setup resgister to 
  digitalWrite(HSPI_CS_CURR, HIGH);
  digitalWrite(VSPI_CS_POT, HIGH);
  digitalWrite(HSPI_CS_IO, HIGH);
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(VSPI_CS_POT, LOW);
  vspi->transfer(0x68);
  digitalWrite(VSPI_CS_POT, HIGH);
  hspi->endTransaction();
  delay(1);
  
 #endif
  pinMode(HSPI_CS_CURR, OUTPUT);
  pinMode(HSPI_CS_IO, OUTPUT);

  //setup:   HSPI A/d  set setup resgister to 
  digitalWrite(HSPI_CS_CURR, HIGH);
  digitalWrite(HSPI_CS_IO, HIGH);
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(HSPI_CS_CURR, LOW);
  hspi->transfer(0x68);
  digitalWrite(HSPI_CS_CURR, HIGH);
  hspi->endTransaction();
  delay(1);
  
#ifdef GPIO
  //setup:   GPIO
  //IODIR – I/O DIRECTION REGISTER (ADDR 0x00) set to 00
  //IOCON – I/O EXPANDER CONFIGURATION REGISTER (ADDR 0x05) set to 0x38
  digitalWrite(HSPI_CS_CURR, HIGH);
  digitalWrite(HSPI_CS_IO, HIGH);
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(HSPI_CS_IO, LOW);
  hspi->transfer(0x40); //Device Opcode write
  hspi->transfer(0x00); //IODIR
  hspi->transfer(0x00); //set to 00
  digitalWrite(HSPI_CS_IO, HIGH);
  hspi->endTransaction();

  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(HSPI_CS_IO, LOW);
  hspi->transfer(0x40); //Device Opcode write
  hspi->transfer(0x05); //IOCON
  hspi->transfer(0x38); //set to 0x38
  digitalWrite(HSPI_CS_IO, HIGH);
  hspi->endTransaction();
#endif
  
	// don't let this task end
	sensorsCoreLoop();
}

#ifdef ARMCONTROLLER 
// Update data from pots
void SensorController::potSPICmd() {
   byte address = 0;
  
  digitalWrite(VSPI_CS_POT, HIGH);
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(VSPI_CS_POT, LOW);
  vspi->transfer(0xB0);
  digitalWrite(VSPI_CS_POT, HIGH);
  vspi->endTransaction();
  delay(1);
  for (; address < NUM_CHASSIS_MOTORS; address++) {  
    vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    digitalWrite(VSPI_CS_POT, LOW);
    //vspi->transfer(address);
    byte retVal = vspi->transfer(0);
    potVals[address]= (int)retVal << 8;
    // see this thread about reading returned value http://forum.arduino.cc/index.php?topic=260836.0
    retVal = vspi->transfer(0);
    potVals[address] |= (int)retVal;

    digitalWrite(VSPI_CS_POT, HIGH);
    vspi->endTransaction();
  }

}
#endif



//reset the current over loads on current monitor brds 
void SensorController::CurrentResetCmd()
{
  //mcp23s08 powers up with:
  //**IODIR – I/O DIRECTION REGISTER (ADDR 0x00) = ff needs to be set to 00
  //IPOL – INPUT POLARITY PORT REGISTER (ADDR 0x01) = 0
  //GPINTEN – INTERRUPT-ON-CHANGE PINS (ADDR 0x02) = 0
  //DEFVAL – DEFAULT VALUE REGISTER (ADDR 0x03) = 0
  //INTCON – INTERRUPT-ON-CHANGE CONTROL REGISTER (ADDR 0x04)  = 0
  //**IOCON – I/O EXPANDER CONFIGURATION REGISTER (ADDR 0x05) = 0 
  //GPPU – GPIO PULL-UP RESISTOR REGISTER (ADDR 0x06) = 0 pull up disabled
  //INTF – INTERRUPT FLAG REGISTER (ADDR 0x07) = 0 no interrupt are pending
  //INTCAP – INTERRUPT CAPTURED VALUE FOR PORT REGISTER (ADDR 0x08)  read only
  //GPIO – GENERAL PURPOSE I/O PORT REGISTER (ADDR 0x09) = 0 ( use to read pins but we don't need this on this project
  //**OLAT – OUTPUT LATCH REGISTER 0 (ADDR 0x0A) = 0 (outputs low)
  
  //registers with ** are needed with this code
  //setup:
  //IODIR – I/O DIRECTION REGISTER (ADDR 0x00) set to 00
  //IOCON – I/O EXPANDER CONFIGURATION REGISTER (ADDR 0x05) set to 0x38

  //This function writes cc to the OLAT – OUTPUT LATCH REGISTER 0 (ADDR 0x0A)
  
 
  digitalWrite(HSPI_CS_CURR, HIGH);
  digitalWrite(HSPI_CS_IO, HIGH);
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(HSPI_CS_IO, LOW);
  hspi->transfer(0x40); //Device Opcode write
   digitalWrite(HSPI_CS_IO, LOW);
  hspi->transfer(0x09); //OLAT
   digitalWrite(HSPI_CS_IO, LOW);
  siCurrentsResetByte ^= 0xff; 
  hspi->transfer(siCurrentsResetByte); //set to 00
  digitalWrite(HSPI_CS_IO, HIGH);
  hspi->endTransaction();
}



// Update data from current sensors
void SensorController::CurrentSPICmd()
{
  //max11628 powers up in mode 10 and all other are set to zero
  //using mode ten we need ot send a Conversion byte then wait 1ms and read all six a/d channels (autoINC starting at 0(lsb,msb) to N)
  //Conversion byte = 1,0110,00,0 : msb bit = 1 to isgnify it is a Conversion bye, n = 6 , scann = 0 so we sscan 0 to N: = 0xB0
  byte address = 0;
  
  digitalWrite(HSPI_CS_IO, HIGH);
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(HSPI_CS_CURR, LOW);
  hspi->transfer(0xB0);
  digitalWrite(HSPI_CS_CURR, HIGH);
  hspi->endTransaction();
  delay(1);
  for (; address < NUM_CHASSIS_MOTORS; address++) {  
    hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    digitalWrite(HSPI_CS_CURR, LOW);
    //hspi->transfer(address);
    byte retVal = hspi->transfer(0);
    currentValues[address]= (int)retVal;
    // see this thread about reading returned value http://forum.arduino.cc/index.php?topic=260836.0
    retVal = hspi->transfer(0);
    currentValues[address]= (int)retVal;

    
    hspi->endTransaction();
  }
  digitalWrite(HSPI_CS_CURR, HIGH);
}

#endif
