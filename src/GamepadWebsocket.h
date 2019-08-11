#ifndef websocketesp_h
#define websocketesp_h



#include "Arduino.h"
#include "ArmControl.h"
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#include "CommunicationStuff.hh"

// INFO FOR LOCAL ROUTER
//const char* ssid = "WE MARS Rover";
//const char* password = "westill1";

int controller_data[] = {0, 0, 0, 0, 0}; //btnMap, axis1, axis2, axis3, axis4


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
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

//returns if buttonID of
bool btnPressed(int buttonID) {

  int btnMap = controller_data[0];
  int mask = 1 << buttonID;
  int maskedBtn = mask & btnMap;
  if (maskedBtn == mask) {
    return true;
  }
  else {
    return false;
  }
}

//if there is a websocket event
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {

  //if the websocket has connected
  if (type == WS_EVT_CONNECT) {
    Serial.println("Websocket client connection received");
    globalClient = client; //declare client
  }
  //if the websocket has disconnected
  else if (type == WS_EVT_DISCONNECT) {
    Serial.println("Client disconnected");
    globalClient = NULL; //to avoid errors
  }
  //if data has been recieved
  else if (type == WS_EVT_DATA) {
    bool flag = true;
    String tempMessage;


    //translate data into string
    for (int i = 0; i < len; i++) {
      if ('_' != (char)data[i] && flag) {
        tempMessage += String((char)data[i]);
      }
      else {
        break;
      }
    }

    GPWnumPings++;
    
    for (int a = 0; a < 5; a++) {
      controller_data[a] = getValue(tempMessage, ',', a + 1).toInt();
    }
    
    int power_f = map(50, -100, 100, MIN_PWM_OUT, MAX_PWM_OUT); //power forward value
    int power_b = map(-50, -100, 100, MIN_PWM_OUT, MAX_PWM_OUT); //power backwards value

    /*
    if(btnPressed(0)){
      ledcWrite(ARM_CHANNELS[0],power_f);
    }
    else if(btnPressed(1)){
      ledcWrite(ARM_CHANNELS[0],power_b);
    }
    */

    int motorXpwr = map(controller_data[1],PERCENTAGE_0,PERCENTAGE_100,MIN_PWM_OUT,MAX_PWM_OUT);
    int motorYpwr = map(controller_data[2],PERCENTAGE_0,PERCENTAGE_100,MIN_PWM_OUT,MAX_PWM_OUT);
    //motor 0
    ledcWrite(ARM_CHANNELS[0],motorXpwr);
    //motor 1
    ledcWrite(ARM_CHANNELS[1],motorYpwr);

    //motor 2
    if(btnPressed(0)){
      ledcWrite(ARM_CHANNELS[2],power_f);
    }
    else if(btnPressed(3)){
      ledcWrite(ARM_CHANNELS[2],power_b);
    }
    else{
      ledcWrite(ARM_CHANNELS,NO_POWER_PWM);
    }

    //motor 3
    if(btnPressed(1)){
      ledcWrite(ARM_CHANNELS[2],power_f);
    }
    else if(btnPressed(2)){
      ledcWrite(ARM_CHANNELS[2],power_b);
    }
    else{
      ledcWrite(ARM_CHANNELS,NO_POWER_PWM);
    }
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
void inline startServer() {
  //need this to write to client
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //start server
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/ws.html", "text/html");
  });
  server.begin();
}

void writeServer(String message) {
  //if server is connected
  if (globalClient) {
    globalClient->text(message);
  }
}




#endif
