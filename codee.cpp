#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>


const char* ssid = "MH HRIDOY";
const char* password = "12345678";


#define STEERING_SERVO_PIN 18
#define MOTOR_PWM_PIN 19
#define MOTOR_DIR_PIN1 21
#define MOTOR_DIR_PIN2 22

Servo steeringServo;


WebServer server(80);


int steeringAngle = 90;  
String gear = "N";        
bool brakePressed = false;
bool motorRunning = false;


const char htmlPage[] PROGMEM = R"rawliteral(


<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>MH2 Car Control</title>
<style>
body { font-family: Arial; background: #000; color: #00f6ff; text-align:center; }
h2 { color:#00f6ff; margin-top:10px; }

#wheel-container { width: 250px; height: 250px; margin:20px auto; position: relative; }
#wheel { width:100%; height:100%; border-radius:50%; border:10px solid #00f6ff; background:#111; position:relative; transition: transform 0.1s ease-out; box-shadow: 0 0 20px #00f6ff; }
#pointer { width:6px; height:120px; background:#00f6ff; position:absolute; left:50%; top:50%; transform-origin: bottom center; margin-left:-3px; border-radius:3px; }

.gear-container, .motor-container { margin:20px auto; display:flex; justify-content:center; gap:15px; }
.gearBtn, .motorBtn { width:60px; height:60px; border-radius:50%; border:none; font-size:18px; font-weight:bold; cursor:pointer; transition:0.2s; color:#00f6ff; background:#111; border:2px solid #00f6ff; }
.gearBtn.active, .motorBtn.active { background:#00f6ff; color:#000; }

#brakeBtn { margin-top:20px; font-size:20px; padding:15px 30px; border-radius:15px; background:#f00; color:#fff; border:none; cursor:pointer; transition:0.2s; }

</style>
</head>
<body>
<h2>MH2 Car Control</h2>

<div id="wheel-container">
  <div id="wheel"><div id="pointer"></div></div>
</div>

<div class="gear-container">
  <button class="gearBtn" id="P" onclick="setGear('P')">P</button>
  <button class="gearBtn" id="N" onclick="setGear('N')">N</button>
  <button class="gearBtn" id="D" onclick="setGear('D')">D</button>
  <button class="gearBtn" id="R" onclick="setGear('R')">R</button>
</div>

<div class="motor-container">
  <button class="motorBtn" id="start" onclick="toggleMotor(true)">Start</button>
  <button class="motorBtn" id="stop" onclick="toggleMotor(false)">Stop</button>
</div>

<button id="brakeBtn" onclick="toggleBrake()">Brake</button>

<script>
let steeringAngle = 90;
let brake = false;
let motorRunning = false;

// Update wheel rotation
function setWheel(angle){
    steeringAngle = angle;
    document.getElementById('wheel').style.transform = 'rotate(' + (angle-90) + 'deg)';
    fetch('/steering?angle=' + angle);
}

// Gear selection
function setGear(g){
    document.querySelectorAll('.gearBtn').forEach(b=>b.classList.remove('active'));
    document.getElementById(g).classList.add('active');
    fetch('/gear?value=' + g);
}

// Brake toggle
function toggleBrake(){
    brake = !brake;
    document.getElementById('brakeBtn').style.background = brake ? '#800' : '#f00';
    fetch('/brake?state=' + (brake ? '1' : '0'));
}

// Motor start/stop
function toggleMotor(state){
    motorRunning = state;
    document.getElementById('start').classList.toggle('active', state);
    document.getElementById('stop').classList.toggle('active', !state);
    fetch('/motor?state=' + (state ? '1' : '0'));
}

// Mouse drag steering
let wheel = document.getElementById('wheel-container');
let dragging = false;
wheel.addEventListener('mousedown', () => dragging=true);
wheel.addEventListener('mouseup', () => dragging=false);
wheel.addEventListener('mouseleave', () => dragging=false);
wheel.addEventListener('mousemove', function(e){
    if(dragging){
        let rect = wheel.getBoundingClientRect();
        let centerX = rect.left + rect.width/2;
        let centerY = rect.top + rect.height/2;
        let x = e.clientX - centerX;
        let y = e.clientY - centerY;
        let angle = Math.atan2(y,x) * 180 / Math.PI + 90;
        if(angle < 0) angle = 0;
        if(angle > 180) angle = 180;
        setWheel(angle);
    }
});
</script>
</body>
</html>


)rawliteral";


void setup(){
  Serial.begin(115200);

  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(MOTOR_DIR_PIN1, OUTPUT);
  pinMode(MOTOR_DIR_PIN2, OUTPUT);

  steeringServo.attach(STEERING_SERVO_PIN);
  steeringServo.write(90);


  WiFi.softAP(ssid, password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());


  server.on("/", [](){ server.send_P(200,"text/html", htmlPage); });
  server.on("/steering", handleSteering);
  server.on("/gear", handleGear);
  server.on("/brake", handleBrake);
  server.on("/motor", handleMotor);

  server.begin();
  Serial.println("Server started");
}


void loop(){
  server.handleClient();


  steeringServo.write(steeringAngle);

 
  if(brakePressed || !motorRunning){
    analogWrite(MOTOR_PWM_PIN,0);
    digitalWrite(MOTOR_DIR_PIN1,LOW);
    digitalWrite(MOTOR_DIR_PIN2,LOW);
  } else {
    if(gear=="D"){
      analogWrite(MOTOR_PWM_PIN,200);
      digitalWrite(MOTOR_DIR_PIN1,HIGH);
      digitalWrite(MOTOR_DIR_PIN2,LOW);
    } else if(gear=="R"){
      analogWrite(MOTOR_PWM_PIN,200);
      digitalWrite(MOTOR_DIR_PIN1,LOW);
      digitalWrite(MOTOR_DIR_PIN2,HIGH);
    } else {
      analogWrite(MOTOR_PWM_PIN,0);
      digitalWrite(MOTOR_DIR_PIN1,LOW);
      digitalWrite(MOTOR_DIR_PIN2,LOW);
    }
  }
}


void handleSteering(){
  if(server.hasArg("angle")){
    steeringAngle = server.arg("angle").toInt();
    if(steeringAngle<0) steeringAngle=0;
    if(steeringAngle>180) steeringAngle=180;
  }
  server.send(200,"text/plain","OK");
}

void handleGear(){
  if(server.hasArg("value")){
    gear = server.arg("value");
  }
  server.send(200,"text/plain","OK");
}

void handleBrake(){
  if(server.hasArg("state")){
    brakePressed = server.arg("state")=="1";
  }
  server.send(200,"text/plain","OK");
}

void handleMotor(){
  if(server.hasArg("state")){
    motorRunning = server.arg("state")=="1";
  }
  server.send(200,"text/plain","OK");
}