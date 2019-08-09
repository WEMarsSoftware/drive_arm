#ifndef armcontrol_h
#define armcontrol_h

#include "Arduino.h"
#include "SensorController.hh"

byte joints = 5;
float deadZone = .5;
float armPosition[joints]; //most recent intentional position of arm, must update every time user moves joint
int pmwStart = 100; //TODO: fix
int pmwCompesation[] = {pwmStart,pwmStart,pwmStart,pwmStart,pwmStart};
float pwmStep = 5;

void armControl(){
  //loop through joints
  for(int a = 0; a < joints; a++){
    //if not at desired position
    if (abs(armPosition[a]-potVals[a]) > deadZone){
      if(armPosition[s] - potVals[a] > 0){
        pwmCompensation[a] -= pwmStep;
      }
      else{
        pwmCompensation[a] += pwmStep;
      }
    }
    //move at set pwmCompensation
    }
  }
}

//call every time user updates joint position
updateJoint(int joint){
  armPosition[joint] = potVals[joint]
  pwmCompensation[joint] = pmwStart; //reset
}



#endif
