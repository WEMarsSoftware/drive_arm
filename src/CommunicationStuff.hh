// CommunicationStuff.hh

#ifndef COMMUNICATION_STUFF
#define COMMUNICATION_STUFF

#include "Arduino.h"
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <SPIFFS.h>;

#include "Electrical.hh"
#include "SensorController.hh"

// INFO FOR LOCAL ROUTER
const char* ssid = "WE MARS Rover";
const char* password = "westill1";

// COMMUNICATION CONSTANTS
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient * globalClient = NULL; //client for server

IPAddress staticIP(192,168,1,100);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

int motorShutdown = 0;

// POST PARAMETERS
const char* motor1 = "MOTOR1";
const char* motor2 = "MOTOR2";
const int NUM_PARAMS = 2;

//HTTP GET PARAMS
const String motorParams[] = {"left-side", "right-side"};

// incremented on each http connection
int numPings = 0;

int wifi_status; //status of wifi 

bool server_connect = false; //if server is connected


//if there is a websocket event
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

  //if the websocket has connected
  if(type == WS_EVT_CONNECT){
 
    Serial.println("Websocket client connection received");
    globalClient = client; //declare client
    server_connect = true; 

  //if the websocket has disconnected
  } else if(type == WS_EVT_DISCONNECT){
    Serial.println("Client disconnected");
    globalClient = NULL; //to avoid errors
    server_connect = false;
  }
}

void inline connectToWiFi()
{
    // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    
  //need this to write to client
  if(!SPIFFS.begin(true))
  {
     Serial.println(F("An Error has occurred while mounting SPIFFS"));
     return;
  }


    WiFi.config(staticIP, gateway, subnet);
    
    delay(100);
  
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.print(F("Number of networks  "));
    Serial.println(n);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
#ifdef DEBUG
      Serial.println("Connecting to WiFi..");
#endif
    }

#ifdef DEBUG
    Serial.println("CONNECTED TO " + String(ssid));
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.macAddress());
#endif

 

}

// Note: will be running on Core #1 (the default core)
void inline setupESPServer(void * args)
{    
  /**
   * HTTP callback with paramaters
   * Will be expecting <IP>/?left-side=#&right-side=#
   * Thanks to:
   * https://techtutorialsx.com/2017/12/17/esp32-arduino-http-server-getting-query-parameters/
   */
   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      numPings++;
      
      // parse parameters into left and right
      int numParams = request->params();
#ifdef DEBUG
        Serial.println("numParams " + String(numParams));
   
#endif
 
      if (numParams != NUM_PARAMS) {
        request->send(200, "text/plain", "Error: number of paramaters not as expected.");
      }

      String left, right;
      for ( int i = 0; i < numParams; i++ ) {
        AsyncWebParameter* p = request->getParam(i);
        String name = p->name();
        if (name == motorParams[0]) {
            left = p->value();
        } else if (name == motorParams[1]) {
            right = p->value();
        }
      }
       
       // if all went well, we now have the two power percentage values
       // set motor output signals
       moveMotors(left.toInt(), right.toInt());
       
       
       // send response back with motor current vals
       // and pot positions in JSON
       // are both size 6 arrays
       String response = String("{") 
                + "\"Current-Left-0\":\"" + String(SensorController::currentValues[0]) 
                + "\",\"Current-Left-1\":\"" + String(SensorController::currentValues[1])
                + "\",\"Current-Left-2\":\"" + String(SensorController::currentValues[2])
                + "\",\"Current-Right-0\":\"" + String(SensorController::currentValues[3])
                + "\",\"Current-Right-1\":\"" + String(SensorController::currentValues[4])
                + "\",\"Current-Right-2\":\"" + String(SensorController::currentValues[5])
                + "\",\"Speed-Left-0\":\"" + String(SensorController::potVals[0])
                + "\",\"Speed-Left-1\":\"" + String(SensorController::potVals[1])
                + "\",\"Speed-Left-2\":\"" + String(SensorController::potVals[2])
                + "\",\"Speed-Right-0\":\"" + String(SensorController::potVals[3])
                + "\",\"Speed-Right-1\":\"" + String(SensorController::potVals[4])
                + "\",\"Speed-Right-2\":\"" + String(SensorController::potVals[5]) 
                + "\",\"ticks\":\"" + String(SensorController::deltaTicks[0])
                + "\"}";
        request->send(200, "text/plain", response);
   });

  /* Test callback -> test connection without affecting motors */
  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request){ 
       // send success
       request->send(200, "text/plain", "Hello!");
   });
   
  //start server
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/ws.html", "text/html");
  });
  server.begin();
}

#endif
