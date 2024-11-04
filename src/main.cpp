#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>

#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <ESP32Encoder.h>

ESP32Encoder encoder;
ESP32Encoder encoder2;
ESP32Encoder encoder3;
ESP32Encoder encoder4;
#define PULSES_PER_REVOLUTION 1700  // Ganti dengan jumlah pulsa per putaran dari encoder Anda

volatile long lastSpeed1 = 0;
volatile long lastSpeed2 = 0;
volatile long lastSpeed3 = 0;
volatile long lastSpeed4 = 0;

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define UP_LEFT 5
#define UP_RIGHT 6
#define DOWN_LEFT 7
#define DOWN_RIGHT 8
#define TURN_LEFT 9
#define TURN_RIGHT 10
#define STOP 0

#define FRONT_RIGHT_MOTOR 1
#define BACK_RIGHT_MOTOR 3
#define FRONT_LEFT_MOTOR 0
#define BACK_LEFT_MOTOR 2

#define FORWARD 1
#define BACKWARD -1


struct MOTOR_PINS
{
  int pinIN1;
  int pinIN2;    
};


std::vector<MOTOR_PINS> motorPins = 
{
  {17, 16}, //FRONT_LEFT_MOTOR 0
  {4, 2},   //FRONT_RIGHT_MOTOR 1
  {32, 33},  //BACK_LEFT_MOTOR  2  
  {26, 25},  //BACK_RIGHT_MOTOR 3
};

const char* ssid     = "MyWiFiCar";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
    .arrows {
      font-size:70px;
      color:red;
    }
    .circularArrows {
      font-size:80px;
      color:blue;
    }
    td {
      background-color:black;
      border-radius:25%;
      box-shadow: 5px 5px #888888;
    }
    td:active {
      transform: translate(5px,5px);
      box-shadow: none; 
    }

    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
         -khtml-user-select: none; /* Konqueror HTML */
           -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }
    </style>
  </head>
  <body class="noselect" align="center" style="background-color:white">
     
    <h1 style="color: teal;text-align:center;">Robot Mecanum</h1>
    <h2 style="color: teal;text-align:center;">Wi-Fi &#128663; Control</h2>
    <h3 style="color: teal;text-align:center;">Speed Data</h3>
    <div id="speedData" style="color: teal;text-align:center;">
      <p>Speed 1: <span id="speed1">0</span> Speed 2: <span id="speed2">0</span> Speed 3: <span id="speed3">0</span> Speed 4: <span id="speed4">0</span></p>
      <p>Vx: <span id="Vx">0</span> Vy: <span id="Vy">0</span> Omega_z: <span id="Omega_z">0</span></p>
    </div>
    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr>
        <td ontouchstart='onTouchStartAndEnd("5")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11017;</span></td>
        <td ontouchstart='onTouchStartAndEnd("1")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#8679;</span></td>
        <td ontouchstart='onTouchStartAndEnd("6")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11016;</span></td>
      </tr>
      
      <tr>
        <td ontouchstart='onTouchStartAndEnd("3")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#8678;</span></td>
        <td></td>    
        <td ontouchstart='onTouchStartAndEnd("4")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#8680;</span></td>
      </tr>
      
      <tr>
        <td ontouchstart='onTouchStartAndEnd("7")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11019;</span></td>
        <td ontouchstart='onTouchStartAndEnd("2")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#8681;</span></td>
        <td ontouchstart='onTouchStartAndEnd("8")' ontouchend='onTouchStartAndEnd("0")'><span class="arrows" >&#11018;</span></td>
      </tr>
    
      <tr>
        <td ontouchstart='onTouchStartAndEnd("9")' ontouchend='onTouchStartAndEnd("0")'><span class="circularArrows" >&#8634;</span></td>
        <td style="background-color:white;box-shadow:none"></td>
        <td ontouchstart='onTouchStartAndEnd("10")' ontouchend='onTouchStartAndEnd("0")'><span class="circularArrows" >&#8635;</span></td>
      </tr>
    </table>



    <script>
      var webSocketUrl = "ws:\/\/" + window.location.hostname + "/ws";
      var websocket;
      
      function initWebSocket() 
      {
        websocket = new WebSocket(webSocketUrl);
        websocket.onopen    = function(event){};
        websocket.onclose   = function(event){setTimeout(initWebSocket, 2000);};
        websocket.onmessage = function(event){
          var data = JSON.parse(event.data);
          document.getElementById("speed1").innerText = data.speed1;
          document.getElementById("speed2").innerText = data.speed2;
          document.getElementById("speed3").innerText = data.speed3;
          document.getElementById("speed4").innerText = data.speed4;
          document.getElementById("Vx").innerText = data.Vx;
          document.getElementById("Vy").innerText = data.Vy;
          document.getElementById("Omega_z").innerText = data.Omega_z;
        };
      }

      function onTouchStartAndEnd(value) 
      {
        websocket.send(value);
      }
          
      window.onload = initWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
      });      
    </script>
    
  </body>
</html> 

)HTMLHOMEPAGE";


void rotateMotor(int motorNumber, int motorDirection)
{
  if (motorDirection == FORWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);    
  }
  else if (motorDirection == BACKWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, HIGH);     
  }
  else
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);       
  }
}

void processCarMovement(String inputValue)
{
  Serial.printf("Got value as %s %d\n", inputValue.c_str(), inputValue.toInt());  
  switch(inputValue.toInt())
  {

    case UP:
      rotateMotor(FRONT_RIGHT_MOTOR, FORWARD);
      rotateMotor(BACK_RIGHT_MOTOR, FORWARD);
      rotateMotor(FRONT_LEFT_MOTOR, FORWARD);
      rotateMotor(BACK_LEFT_MOTOR, FORWARD);                  
      break;
  
    case DOWN:
      rotateMotor(FRONT_RIGHT_MOTOR, BACKWARD);
      rotateMotor(BACK_RIGHT_MOTOR, BACKWARD);
      rotateMotor(FRONT_LEFT_MOTOR, BACKWARD);
      rotateMotor(BACK_LEFT_MOTOR, BACKWARD);   
      break;
  
    case LEFT:
      rotateMotor(FRONT_RIGHT_MOTOR, FORWARD);
      rotateMotor(BACK_RIGHT_MOTOR, BACKWARD);
      rotateMotor(FRONT_LEFT_MOTOR, BACKWARD);
      rotateMotor(BACK_LEFT_MOTOR, FORWARD);   
      break;
  
    case RIGHT:
      rotateMotor(FRONT_RIGHT_MOTOR, BACKWARD);
      rotateMotor(BACK_RIGHT_MOTOR, FORWARD);
      rotateMotor(FRONT_LEFT_MOTOR, FORWARD);
      rotateMotor(BACK_LEFT_MOTOR, BACKWARD);  
      break;
  
    case UP_LEFT:
      rotateMotor(FRONT_RIGHT_MOTOR, FORWARD);
      rotateMotor(BACK_RIGHT_MOTOR, STOP);
      rotateMotor(FRONT_LEFT_MOTOR, STOP);
      rotateMotor(BACK_LEFT_MOTOR, FORWARD);  
      break;
  
    case UP_RIGHT:
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(BACK_RIGHT_MOTOR, FORWARD);
      rotateMotor(FRONT_LEFT_MOTOR, FORWARD);
      rotateMotor(BACK_LEFT_MOTOR, STOP);  
      break;
  
    case DOWN_LEFT:
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(BACK_RIGHT_MOTOR, BACKWARD);
      rotateMotor(FRONT_LEFT_MOTOR, BACKWARD);
      rotateMotor(BACK_LEFT_MOTOR, STOP);   
      break;
  
    case DOWN_RIGHT:
      rotateMotor(FRONT_RIGHT_MOTOR, BACKWARD);
      rotateMotor(BACK_RIGHT_MOTOR, STOP);
      rotateMotor(FRONT_LEFT_MOTOR, STOP);
      rotateMotor(BACK_LEFT_MOTOR, BACKWARD);   
      break;
  
    case TURN_LEFT:
      rotateMotor(FRONT_RIGHT_MOTOR, FORWARD);
      rotateMotor(BACK_RIGHT_MOTOR, FORWARD);
      rotateMotor(FRONT_LEFT_MOTOR, BACKWARD);
      rotateMotor(BACK_LEFT_MOTOR, BACKWARD);  
      break;
  
    case TURN_RIGHT:
      rotateMotor(FRONT_RIGHT_MOTOR, BACKWARD);
      rotateMotor(BACK_RIGHT_MOTOR, BACKWARD);
      rotateMotor(FRONT_LEFT_MOTOR, FORWARD);
      rotateMotor(BACK_LEFT_MOTOR, FORWARD);   
      break;
  
    case STOP:
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(BACK_RIGHT_MOTOR, STOP);
      rotateMotor(FRONT_LEFT_MOTOR, STOP);
      rotateMotor(BACK_LEFT_MOTOR, STOP);    
      break;
  
    default:
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(BACK_RIGHT_MOTOR, STOP);
      rotateMotor(FRONT_LEFT_MOTOR, STOP);
      rotateMotor(BACK_LEFT_MOTOR, STOP);    
      break;
  }
}

void sendSpeedData() {
  long currentSpeed1 = encoder.getCount();
  long currentSpeed2 = encoder2.getCount();
  long currentSpeed3 = encoder3.getCount();
  long currentSpeed4 = encoder4.getCount();

  // Hitung RPM
  float rpm1 = ((currentSpeed1 - lastSpeed1) / (float)PULSES_PER_REVOLUTION) * 60.0;
  float rpm2 = ((currentSpeed2 - lastSpeed2) / (float)PULSES_PER_REVOLUTION) * 60.0;
  float rpm3 = ((currentSpeed3 - lastSpeed3) / (float)PULSES_PER_REVOLUTION) * 60.0;
  float rpm4 = ((currentSpeed4 - lastSpeed4) / (float)PULSES_PER_REVOLUTION) * 60.0;

  lastSpeed1 = currentSpeed1;
  lastSpeed2 = currentSpeed2;
  lastSpeed3 = currentSpeed3;
  lastSpeed4 = currentSpeed4;
float radius = 0.0475;
float L = 0.145;
float W = 0.155;
  // Hitung kecepatan linier dari setiap roda
  float V_FR = rpm2 * (2 * PI * radius) / 60.0;
  float V_FL = rpm1 * (2 * PI * radius) / 60.0;
  float V_BR = rpm4 * (2 * PI * radius) / 60.0;
  float V_BL = rpm3 * (2 * PI * radius) / 60.0;

  // Hitung Vx, Vy, dan Omega_z
  float Vx = (V_FR + V_FL + V_BR + V_BL) / 4.0;
  float Vy = (- V_FL + V_FR  + V_BL - V_BR) / 4.0;
  float Omega_z = (-V_FL + V_FR   - V_BL + V_BR ) / (4.0 * (L + W));

  String speedData = String("{\"speed1\":") + rpm1 + 
                     String(",\"speed2\":") + rpm2 + 
                     String(",\"speed3\":") + rpm3 + 
                     String(",\"speed4\":") + rpm4 + 
                     String(",\"Vx\":") + Vx + 
                     String(",\"Vy\":") + Vy + 
                     String(",\"Omega_z\":") + Omega_z + String("}");
  ws.textAll(speedData);
}

void handleRoot(AsyncWebServerRequest *request) 
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) 
{
    request->send(404, "text/plain", "File Not Found");
}


void onWebSocketEvent(AsyncWebSocket *server, 
                      AsyncWebSocketClient *client, 
                      AwsEventType type,
                      void *arg, 
                      uint8_t *data, 
                      size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      //client->text(getRelayPinsStatusJson(ALL_RELAY_PINS_INDEX));
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      processCarMovement("0");
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
      {
        std::string myData = "";
        myData.assign((char *)data, len);
        processCarMovement(myData.c_str());       
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

void setUpPinModes()
{
  for (int i = 0; i < motorPins.size(); i++)
  {
    pinMode(motorPins[i].pinIN1, OUTPUT);
    pinMode(motorPins[i].pinIN2, OUTPUT);  
    rotateMotor(i, STOP);  
  }
}

unsigned long previousMillis = 0; // Variabel untuk menyimpan waktu sebelumnya
const long interval = 1000; // Interval waktu dalam milidetik

void setup(void) 
{
  setUpPinModes();
  Serial.begin(115200);

    ESP32Encoder::useInternalWeakPullResistors = puType::up;
 // use pin 17 and 16 for the second encoder
  encoder.attachHalfQuad(5, 18); //FRONT_LEFT_MOTOR

  // use pin 19 and 18 for the first encoder
  encoder2.attachHalfQuad(23, 22); //FRONT_RIGHT_MOTOR
 

  // use pin 19 and 18 for the first encoder
  encoder3.attachHalfQuad(36, 39); //BACK_LEFT_MOTOR
  // use pin 17 and 16 for the second encoder
  encoder4.attachHalfQuad(35, 34); //BACK_RIGHT_MOTOR
  // set starting count value after attaching
  encoder.setCount(0);
  encoder2.setCount(0);
  encoder3.setCount(0);
  encoder4.setCount(0);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() 
{
  ws.cleanupClients(); 

  unsigned long currentMillis = millis(); // Mendapatkan waktu saat ini

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Menyimpan waktu saat ini
    sendSpeedData(); // Panggil fungsi sendSpeedData
  }
}