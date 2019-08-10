#ifndef armcontrol_h
#define armcontrol_h

#include "Arduino.h"
#include "SensorController.hh"

byte joints = 5;
//float deadZone = .5;

//TODO: set min and max pot values - Kyle
int armSetpoint[joints]; //most recent intentional position of arm, must update every time user moves joint //setpoint
float p_value = ;

const int ARM_PINS = {15, 2, 4, 16, 17};
cont int ARM_CHANNELS = {1,2,3,4,5}

void armControl(){
  
  //loop through joints
  for(int a = 0; a < joints; a++){
      float error = armSetpoint[a] - potVals[a];
      float motorSpeed = p_value*error;
    }
  }
}

//call every time user updates joint position
updateJoint(int joint){
  armSetpoint[joint] = potVals[joint]
  pwmCompensation[joint] = pmwStart; //reset
}



#endif
