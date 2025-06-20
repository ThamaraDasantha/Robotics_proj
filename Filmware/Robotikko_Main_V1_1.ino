#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>

#include <ESP32Servo.h>
#include <iostream>
#include <sstream>
//main func. define
#define relay1 35
#define relay2 32
#define relay3 33
#define relay4 25                //not using
#define servo1 26                //arm servo1
#define servo2 27                //arm servo 2
#define servo3 14                //arm servo 3
#define servo4 12                //arm servo 4
#define SENSe digitalRead(19)    // edge
#define SENSel digitalRead(21)   // edge left
#define SENSer digitalRead(22)   // edge right
#define SENSf !digitalRead(23)   // front
#define SENSfl !digitalRead(15)  // front left
#define SENSfr !digitalRead(13)  // front right
//j2
#define MD_EN_A 2     //Motor1 speed
#define MD_Input1 4   //Motor1 direction
#define MD_Input2 16  //motor1 direction
//j3
#define MD_EN_B 0     //Motor2 speed
#define MD_Input3 17  //motor2 direction
#define MD_Input4 5   //motor2 direction

#define LED 18
#define delay90 800  //delay to turn 90deg.

struct ServoPins {
  Servo servo;
  int servoPin;
  String servoName;
  int initialPosition;
};
std::vector<ServoPins> servoPins = {
  { Servo(), 27, "Base", 90 },
  { Servo(), 26, "Shoulder", 90 },
  { Servo(), 14, "Elbow", 0 },
};

bool turn1;      // 1-left , 0-right
int count;       // edge count
int wspeed = 8;  // wheel speed 0-255
int input;
int minput;

struct RecordedStep {
  int servoIndex;
  int value;
  int delayInStep;
};
std::vector<RecordedStep> recordedSteps;

bool recordSteps = false;
bool playRecordedSteps = false;

unsigned long previousTimeInMilli = millis();
const int PWMFreq = 1000; /* 1 KHz */
const int PWMResolution = 8;
const int PWMSpeedChannel = 4;

const char *ssid = "Robotikko";
const char *password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket wsRobotInput("/RobotInput");

const char *htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>

    .arrows {
      font-size:40px;
      color:red;
    }
    td.button {
      background-color:black;
      border-radius:25%;
      box-shadow: 5px 5px #888888;
    }
    td.button:active {
      transform: translate(5px,5px);
      box-shadow: none; 
    }

    input[type=button]
    {
      background-color:red;color:white;border-radius:30px;width:100%;height:40px;font-size:20px;text-align:center;
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

    .slidecontainer {
      width: 100%;
    }

    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 20px;
      border-radius: 5px;
      background: #d3d3d3;
      outline: none;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
    }

    .slider:hover {
      opacity: 1;
    }
  
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

    .slider::-moz-range-thumb {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

    </style>
  
  </head>
  <body class="noselect" align="center" style="background-color:white">
     
    <h1 style="color: teal;text-align:center;">ROBOTIKKO</h1>
    <h2 style="color: teal;text-align:center;">Robot Control</h2>
	
    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
	  <tr></tr/>	 
	 <tr>
        <td></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","1")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows" >&#8679;</span></td>
        <td></td>
      </tr>
      <tr>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","3")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows" >&#8678;</span></td>
        <td class="button"></td>    
        <td class="button" ontouchstart='sendButtonInput("MoveCar","4")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows" >&#8680;</span></td>
      </tr>
      <tr>
        <td></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","2")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows" >&#8681;</span></td>
        <td></td>
      </tr>
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Elbow:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="180" value="0" class="slider" id="Elbow" oninput='sendButtonInput("Elbow",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Shoulder:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Shoulder" oninput='sendButtonInput("Shoulder",value)'>
          </div>
        </td>
      </tr>  
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Base:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Base" oninput='sendButtonInput("Base",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/>  
	    <tr>
        <td style="text-align:left;font-size:25px"><b>Speed:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="255" value="255" class="slider" id="Gripper" oninput='sendButtonInput("Gripper",value)'>
          </div>
        </td>
      </tr> 
      <tr>
        <td style="text-align:left;font-size:25px"><b>Record:</b></td>
        <td><input type="button" id="Record" value="OFF" ontouchend='onclickButton(this)'></td>
        <td></td>
      </tr>
      <tr/><tr/> 
      <tr>
        <td style="text-align:left;font-size:25px"><b>Play:</b></td>
        <td><input type="button" id="Play" value="OFF" ontouchend='onclickButton(this)'></td>
        <td></td>
      </tr>    
      <tr/><tr/> 
      <tr>
        <td style="text-align:left;font-size:25px"><b>Brush :</b></td>
        <td><input type="button" id="Brush" value="OFF" ontouchend='onclickButton(this)'></td>
        <td></td>
      </tr>  
      <tr/><tr/> 
      <tr>
        <td style="text-align:left;font-size:25px"><b>Automated Drive:</b></td>
        <td><input type="button" id="Mode" value="OFF" ontouchend='onclickButton(this)'></td>
        <td></td>
      </tr>    
      <tr/><tr/>  
    </table>
  
    <script>
      var webSocketRobotInputUrl = "ws:\/\/" + window.location.hostname + "/RobotInput";      
      var websocketRobotInput;
      
      function initRobotInputWebSocket() 
      {
        websocketRobotInput = new WebSocket(webSocketRobotInputUrl);
        websocketRobotInput.onopen    = function(event){};
        websocketRobotInput.onclose   = function(event){setTimeout(initRobotInputWebSocket, 2000);};
        websocketRobotInput.onmessage    = function(event)
        {
          var keyValue = event.data.split(",");
          var button = document.getElementById(keyValue[0]);
		      var speedButton = document.getElementById("Speed");
          sendButtonInput("Speed", speedButton.value);
          var elevateButton = document.getElementById("Elevate");
          sendButtonInput("Elevate", elevateButton.value);   
          button.value = keyValue[1];
          if (button.id == "Record" || button.id == "Play")
          {
            button.style.backgroundColor = (button.value == "ON" ? "green" : "red");  
            enableDisableButtonsSliders(button);
          }
        };
		    
      }
      
      function sendButtonInput(key, value) 
      {
        var data = key + "," + value;
        websocketRobotInput.send(data);
      }
      
      function onclickButton(button) 
      {
        button.value = (button.value == "ON") ? "OFF" : "ON" ;        
        button.style.backgroundColor = (button.value == "ON" ? "green" : "red");          
        var value = (button.value == "ON") ? 1 : 0 ;
        sendButtonInput(button.id, value);
        enableDisableButtonsSliders(button);
      }
      
      function enableDisableButtonsSliders(button)
      {
        if(button.id == "Play")
        {
          var disabled = "auto";
          if (button.value == "ON")
          {
            disabled = "none";            
          }
          document.getElementById("Elbow").style.pointerEvents = disabled;          
          document.getElementById("Shoulder").style.pointerEvents = disabled;          
          document.getElementById("Base").style.pointerEvents = disabled; 
          document.getElementById("Record").style.pointerEvents = disabled;
        }
        if(button.id == "Record")
        {
          var disabled = "auto";
          if (button.value == "ON")
          {
            disabled = "none";            
          }
          document.getElementById("Play").style.pointerEvents = disabled;
        }        
      }
           
      window.onload = initRobotInputWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
      });       
    </script>
  </body>    
</html>
)HTMLHOMEPAGE";


void setup(void) {

  pinMode(relay1, OUTPUT);     //relay1
  pinMode(relay2, OUTPUT);     //relay2
  pinMode(relay3, OUTPUT);     //relay3
  pinMode(relay4, OUTPUT);     //relay4
  pinMode(MD_Input1, OUTPUT);  //Motor1 direction
  pinMode(MD_Input2, OUTPUT);  //motor1 direction
  pinMode(MD_Input3, OUTPUT);  //motor2 direction
  pinMode(MD_Input4, OUTPUT);  //motor2 direction
  pinMode(MD_EN_A, OUTPUT);    //Motor1 speed
  pinMode(MD_EN_B, OUTPUT);    //Motor2 speed
  pinMode(LED, OUTPUT);
  pinMode(servo1, OUTPUT);
  pinMode(servo2, OUTPUT);
  pinMode(servo3, OUTPUT);
  pinMode(servo4, OUTPUT);
  pinMode(19, INPUT);  // Sensors
  pinMode(21, INPUT);
  pinMode(22, INPUT);
  pinMode(23, INPUT);
  pinMode(15, INPUT);
  pinMode(13, INPUT);
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
  digitalWrite(relay3, LOW);
  digitalWrite(relay4, LOW);
  digitalWrite(MD_EN_A, LOW);  //--------En
  digitalWrite(MD_Input1, LOW);
  digitalWrite(MD_Input2, LOW);
  digitalWrite(MD_EN_B, LOW);  //--------En
  digitalWrite(MD_Input3, LOW);
  digitalWrite(MD_Input4, LOW);

  //wifi clint
  setUpPinModes();
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  wsRobotInput.onEvent(onRobotInputWebSocketEvent);
  server.addHandler(&wsRobotInput);

  server.begin();
  Serial.println("HTTP server started");
}

// void rotateMotor(int motorNumber, int motorDirection)
// {
//   Serial.println(motorNumber);
//   Serial.println(motorDirection);
// }

void moveCar(int inputValue) {
  Serial.printf("Got value as %d\n", inputValue);
}

//manual controlling fn
void remotecon() {
  int read = input;
  motorcon(read, wspeed);
}

// motor driver fn
void motorcon(int direc, int spd) {
  analogWrite(MD_EN_A, spd);
  analogWrite(MD_EN_B, spd);

  switch (direc) {
    case 1:  //forward
      digitalWrite(MD_Input1, HIGH);
      digitalWrite(MD_Input2, LOW);
      digitalWrite(MD_Input3, HIGH);
      digitalWrite(MD_Input4, LOW);
      break;
    case 2:  //backward
      digitalWrite(MD_Input1, LOW);
      digitalWrite(MD_Input2, HIGH);
      digitalWrite(MD_Input3, LOW);
      digitalWrite(MD_Input4, HIGH);
      break;
    case 4:  //turn right
      digitalWrite(MD_Input1, HIGH);
      digitalWrite(MD_Input2, LOW);
      digitalWrite(MD_Input3, LOW);
      digitalWrite(MD_Input4, HIGH);
      //  delay(1000);
      break;
    case 3:  //turn left
      digitalWrite(MD_Input1, LOW);
      digitalWrite(MD_Input2, HIGH);
      digitalWrite(MD_Input3, HIGH);
      digitalWrite(MD_Input4, LOW);
      //  delay(1000);
      break;
    // case 'sr':  //turn right on solar panel
    //   digitalWrite(MD_Input1, HIGH);
    //   digitalWrite(MD_Input2, LOW);
    //   digitalWrite(MD_Input3, LOW);
    //   digitalWrite(MD_Input4, LOW);
    //   delay(1000);
    //   break;
    // case 'sl':  //turn left on solar panel
    //   digitalWrite(MD_Input1, LOW);
    //   digitalWrite(MD_Input2, LOW);
    //   digitalWrite(MD_Input3, HIGH);
    //   digitalWrite(MD_Input4, LOW);
    //   delay(1000);
    //   break;
    case 0:  //stop
      digitalWrite(MD_Input1, LOW);
      digitalWrite(MD_Input2, LOW);
      digitalWrite(MD_Input3, LOW);
      digitalWrite(MD_Input4, LOW);
      break;
    default:  //stop
      digitalWrite(MD_Input1, LOW);
      digitalWrite(MD_Input2, LOW);
      digitalWrite(MD_Input3, LOW);
      digitalWrite(MD_Input4, LOW);
  }
}

void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "File Not Found");
}

void onRobotInputWebSocketEvent(AsyncWebSocket *server,
                                AsyncWebSocketClient *client,
                                AwsEventType type,
                                void *arg,
                                uint8_t *data,
                                size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      sendCurrentRobotArmState();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo *)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        std::string myData = "";
        myData.assign((char *)data, len);
        std::istringstream ss(myData);
        std::string key, value;
        std::getline(ss, key, ',');
        std::getline(ss, value, ',');
        Serial.printf("Key [%s] Value[%s]\n", key.c_str(), value.c_str());
        int valueInt = atoi(value.c_str());

        if (key == "Record") {
          recordSteps = valueInt;
          if (recordSteps) {
            recordedSteps.clear();
            previousTimeInMilli = millis();
          }
        } else if (key == "Play") {
          playRecordedSteps = valueInt;
        } else if (key == "Base") {
          writeServoValues(0, valueInt);
        } else if (key == "Shoulder") {
          writeServoValues(1, valueInt);
        } else if (key == "Elbow") {
          writeServoValues(2, valueInt);
        } else if (key == "MoveCar") {
          // motorcon(valueInt);
          input = valueInt;
          moveCar(valueInt);
        } else if (key == "Mode") {
          minput = valueInt;
        } else if (key == "Brush") {
          digitalWrite(relay2, valueInt);
          digitalWrite(relay3, valueInt);
        }
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;
  }
}

void sendCurrentRobotArmState() {
  for (int i = 0; i < servoPins.size(); i++) {
    wsRobotInput.textAll(servoPins[i].servoName + "," + servoPins[i].servo.read());
  }
  wsRobotInput.textAll(String("Record,") + (recordSteps ? "ON" : "OFF"));
  wsRobotInput.textAll(String("Play,") + (playRecordedSteps ? "ON" : "OFF"));
}

void writeServoValues(int servoIndex, int value) {
  if (recordSteps) {
    RecordedStep recordedStep;
    if (recordedSteps.size() == 0)  // We will first record initial position of all servos.
    {
      for (int i = 0; i < servoPins.size(); i++) {
        recordedStep.servoIndex = i;
        recordedStep.value = servoPins[i].servo.read();
        recordedStep.delayInStep = 0;
        recordedSteps.push_back(recordedStep);
      }
    }
    unsigned long currentTime = millis();
    recordedStep.servoIndex = servoIndex;
    recordedStep.value = value;
    recordedStep.delayInStep = currentTime - previousTimeInMilli;
    recordedSteps.push_back(recordedStep);
    previousTimeInMilli = currentTime;
  }
  servoPins[servoIndex].servo.write(value);
}

void playRecordedRobotArmSteps() {
  if (recordedSteps.size() == 0) {
    return;
  }
  //This is to move servo to initial position slowly. First 4 steps are initial position
  for (int i = 0; i < 4 && playRecordedSteps; i++) {
    RecordedStep &recordedStep = recordedSteps[i];
    int currentServoPosition = servoPins[recordedStep.servoIndex].servo.read();
    while (currentServoPosition != recordedStep.value && playRecordedSteps) {
      currentServoPosition = (currentServoPosition > recordedStep.value ? currentServoPosition - 1 : currentServoPosition + 1);
      servoPins[recordedStep.servoIndex].servo.write(currentServoPosition);
      wsRobotInput.textAll(servoPins[recordedStep.servoIndex].servoName + "," + currentServoPosition);
      delay(50);
    }
  }
  delay(2000);  // Delay before starting the actual steps.

  for (int i = 4; i < recordedSteps.size() && playRecordedSteps; i++) {
    RecordedStep &recordedStep = recordedSteps[i];
    delay(recordedStep.delayInStep);
    servoPins[recordedStep.servoIndex].servo.write(recordedStep.value);
    wsRobotInput.textAll(servoPins[recordedStep.servoIndex].servoName + "," + recordedStep.value);
  }
}

void setUpPinModes() {
  for (int i = 0; i < servoPins.size(); i++) {
    servoPins[i].servo.attach(servoPins[i].servoPin);
    servoPins[i].servo.write(servoPins[i].initialPosition);
  }
}

void changemode(int minput) {
  if (minput == 1) {
    obst_avoid();
    Serial.printf("auto mode active");
  }
}

void loop() {

  wsRobotInput.cleanupClients();
  if (playRecordedSteps) {
    playRecordedRobotArmSteps();
  }

  remotecon();

  // motorcon('s', 0);
  digitalWrite(LED, LOW);
  bool mode = minput;
  while (mode == 1) {  // automatic mode
                       // while (1) {
    Serial.print("automatic mode    ");
    Serial.println(mode);
    digitalWrite(LED, HIGH);
    obst_avoid();
    mode = minput;
    //delay(500);
    //  if (mode == 0) break;
    // }
  }
}

// obstacles avoiding fn
void obst_avoid() {
  //Serial.println("obst");
  int wsped = 180;
  static bool srrd_flag;               //surrounded flag
  if (SENSf == 1 || srrd_flag == 1) {  //edge or wall detected (infront)                   SENSe == 1 ||
    motorcon(0, 0);                    //stop
    srrd_flag = 0;
    //Serial.println("Entered to main if");
    if (SENSel == 0 && SENSfl == 0 && SENSer == 0 && SENSfr == 0) {  // no edge or wall (left & right)
      motorcon(4, wsped);                                            //turn right (random turn)
      delay(delay90);
      //Serial.println("turn right (random turn)");
    } else if ((SENSel == 1 || SENSfl == 1) && (SENSer == 0 && SENSfr == 0)) {  //edge or wall (left)
      motorcon(4, wsped);                                                       //turn right
      delay(delay90);
      //Serial.println("turn right");
    } else if ((SENSer == 1 || SENSfr == 1) && (SENSel == 0 && SENSfl == 0)) {  //edge or wall (right)
      motorcon(3, wsped);                                                       //turn left
      delay(delay90);
      //Serial.println("turn left");
    } else {               // edge or wall (left & right) surrounded
      motorcon(2, wsped);  // go backward
      //Serial.println("surrounded, go backward");
      delay(1000);
      motorcon(0, 0);  //stop
      srrd_flag = 1;
      obst_avoid();  // again search for free side to turn
    }
    motorcon(1, wsped);  // after turning, go forward
    //Serial.println("after turning, go forward");
    // } else if (SENSe == 1) {  //edge detected
    //   motorcon(2, wsped);    // go backward
    //   //Serial.println("edge, go backward");
    //   delay(1000);
    //   motorcon(0, 0);  //stop
    //   srrd_flag = 1;
    //   obst_avoid();  // again search for free side to turn
  } else {
    motorcon(1, wsped);  //no obstacle, go forword
    //Serial.println("no obstacle, go forword");
  }
  //Serial.println("Exited from main if");
}