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

int controller1_data[] = {0, 0, 0, 0, 0}; //btnMap, axis1, axis2, axis3, axis4
int controller2_data[] = {0, 0, 0, 0, 0}; //btnMap, axis1, axis2, axis3, axis4


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
bool isBtnPressed(byte controller, int buttonID) {

  int btnMap;

  if (controller = 1) {
    btnMap = controller1_data[0];
  }
  else {
    btnMap = controller2_data[0];
  }

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

    String id_temp = getValue(tempMessage, ',', 0);
    int id = id_temp.toInt();
    int tempData[5];
    for (int a = 0; a < 5; a++) {
      tempData[a] = getValue(tempMessage, ',', a + 1).toInt();
    }

    if (id == 0) {
      controller1_data = tempData;
    }
    else if (id == 1) {
      controller2_data = tempData;

      //TODO: change into for loop
      if (btnPressed(2, 0)) {
        ledcWrite(ARM_CHANNEL[0], MAX_PWM_OUT);
      }
      else if (btnPressed(2, 1)) {
        ledcWrite(ARM_CHANNEL[0], MIN_PWM_OUT);
      }
      else if (btnPressed(2, 2)) {
        ledcWrite(ARM_CHANNEL[1], MAX_PWM_OUT);
      }
      else if (btnPressed(2, 3)) {
        ledcWrite(ARM_CHANNEL[1], MIN_PWM_OUT);
      }
      else if (btnPressed(2, 4)) {
        ledcWrite(ARM_CHANNEL[2], MAX_PWM_OUT);
      }
      else if (btnPressed(2, 5)) {
        ledcWrite(ARM_CHANNEL[2], MIN_PWM_OUT);
      }
      else if (btnPressed(2, 6)) {
        ledcWrite(ARM_CHANNEL[3], MAX_PWM_OUT);
      }
      else if (btnPressed(2, 7)) {
        ledcWrite(ARM_CHANNEL[3], MIN_PWM_OUT);
      }
      else if (btnPressed(2, 8)) {
        ledcWrite(ARM_CHANNEL[4], MAX_PWM_OUT);
      }
      else if (btnPressed(2, 9)) {
        ledcWrite(ARM_CHANNEL[4], MIN_PWM_OUT);
      }
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
