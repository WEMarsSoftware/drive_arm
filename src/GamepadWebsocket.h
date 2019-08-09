/*
 * GamepadWebsocket.h
 * Kyle Inzunza
 */

 /*
 XBOX button layout

0 A
1 B
2 X
3 Y
4 LB
5 RB
6 LT
7 RT
8 BACK 
9 START
10 L AXIS
11 R AXIS
12 DPAD up
13 DPAD down
14 DPAD left
15 DPAD right
16 POWER
*/

#ifndef websocketesp_h
#define websocketesp_h

#include "Arduino.h"
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#include "CommunicationStuff.hh"

// INFO FOR LOCAL ROUTER
//const char* ssid = "WE MARS Rover";
//const char* password = "westill1";

String controller1_data = {0,0,0,0,0}; //btnMap, axis1, axis2, axis3, axis4
String controller2_data = {0,0,0,0,0}; //btnMap, axis1, axis2, axis3, axis4


String strLeftRight;
String strForwardBack;

int iLeftRight;
int iForwardBack;
int iWorkingLeft;
int iWorkingRight;

int GPWnumPings;

bool gp_connected = false; //if websocket is connected

// COMMUNICATION CONSTANTS
//AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient * globalClient = NULL; //client for server

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

//if there is a websocket event
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

  //if the websocket has connected
  if(type == WS_EVT_CONNECT){
    #ifdef DEBUG
    Serial.println("Websocket client connection received");
    #endif
    globalClient = client; //declare client
    gp_connected = true;
  }
  //if the websocket has disconnected
  else if(type == WS_EVT_DISCONNECT){
    #ifdef DEBUG
    Serial.println("Client disconnected");
    #endif
    globalClient = NULL; //to avoid errors
    gp_connected = false;
  }
  //if data has been recieved
  else if(type == WS_EVT_DATA){
    bool flag = true;
    String tempMessage;
    
    
    //translate data into string
    for(int i=0; i < len; i++) {
       if('_' != (char)data[i] && flag){
          tempMessage += String((char)data[i]);  
       }
       else{
          break;
       }
    }

    GPWnumPings++;

    int id = getValue(tempMessage, ',',0);
    int tempData[5];
    for(int a = 0; a < 5; a++){
      tempData = getValue(tempMessage,',',a+1);
    }

    if(id == 0){
      controller1_data = tempData;
    }
    else if(id == 1){
      controller2_data = tempData;
    }

    //remote turn off
    if(turnOffState()){
      turnOffSpike();
    }
    
    
    //old drive code, didn't delete because don't want to break
    strLeftRight = getValue(controller1_data, ',', 0);
    strForwardBack = getValue(controller1_data, ',', 1);
    iLeftRight = strLeftRight.toInt();
    iForwardBack = strForwardBack.toInt();
    if(iLeftRight < 0 )
    {
      iWorkingLeft = iForwardBack + iLeftRight;
      iWorkingRight = iForwardBack; 
    }
    else
    {
      iWorkingLeft = iForwardBack; 
      iWorkingRight = iForwardBack - iLeftRight;
    }
    
     moveMotors(iWorkingLeft, iWorkingRight);
    
  }
}

//starts wifi
//must begin serial before calling this function
void inline startWiFi()
{  
    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    Serial.println("CONNECTED TO " + String(ssid));
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.macAddress());
}

//starts server
void inline startServer(){
  //need this to write to client
  if(!SPIFFS.begin(true)){
     Serial.println("An Error has occurred while mounting SPIFFS");
     return;
  }

  //start server
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/ws.html", "text/html");
  });
  server.begin();
}

void writeServer(String message){
  //if server is connected
  if (globalClient){
    globalClient->text(message);
  }
}

//returns if buttonID of 
bool isBtnPressed(byte controller, int buttonID){

   int btnMap; 

   if(controller = 1){
      btnMap = controller1_data[0];
   }
   else{
      btnMap = controller2_data[0];
   }
   
   int mask = 1 << buttonID;
   int maskedBtn = mask & btnMap;
   if(maskedBtn == mask){
    return true;
   }
   else{
    return false;
   }
}

//checks if either of the controllers have selected to turn off rover
bool turnOffState(){
  bool ctr1 = (isBtnPressed(1,8) && isBtnPressed(1,9));
  bool ctr2 = (isBtnPressed(2,8) && isBtnPressed(1,9));
  return ctr1 || ctr2; 
}

#endif
