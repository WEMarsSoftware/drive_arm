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

String controller1_data = "0,0,0,0,0";
String controller2_data = "1,0,0,0,0";

String strLeftRight;
String strForwardBack;

int iLeftRight;
int iForwardBack;
int iWorkingLeft;
int iWorkingRight;

int GPWnumPings;

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
    Serial.println("Websocket client connection received");
    globalClient = client; //declare client
  }
  //if the websocket has disconnected
  else if(type == WS_EVT_DISCONNECT){
    Serial.println("Client disconnected");
    globalClient = NULL; //to avoid errors
  }
  //if data has been recieved
  else if(type == WS_EVT_DATA){
    bool flag = true;
    char temp[len];
    
    //translate data into string
    for(int i=0; i < len; i++) {
       if('_' != (char)data[i] && flag){
          temp[i] = (char)data[i];  
       }
       else if (flag){
          flag = false;
          temp[i] = ',';
       }
       else{
          temp[i] = '_'; 
       }
    }

    GPWnumPings++;
    controller1_data = temp; //save controller data
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

#endif
