#include<DHT.h>
#include <Servo.h>
//Sensors
  //DHT Sensor
  #define DHTPIN 2     // Digital pin connected to the DHT sensor
  #define DHTTYPE DHT11   // DHT 11
  DHT dht(DHTPIN, DHTTYPE);  // Initialize DHT sensor.

  //ESCs
  Servo ESC1;
  Servo ESC2;
  Servo ESC3;
  Servo ESC4;
  Servo ESC5;
  Servo ESC6;

  Servo Arm;

  int ESC1Speed;
  int ESC2Speed;
  int ESC3Speed;
  int ESC4Speed;
  int ESC5Speed;
  int ESC6Speed;

  int ArmAngle;
//Control Modes
  byte hoverMode;
//Relays
  #define relayPin1 10
  #define relayPin2 11

//Delay Management
  //current millis
  unsigned long currentMillis;
  //send sensor data to nodemcu
  unsigned long previousMillis_send = 0;
  const long interval_send = 1000;
//Delay Management End


void setup() {
  Serial.begin(115200);
  ESC1.attach(3,1000,2000);
  ESC2.attach(4,1000,2000);
  ESC3.attach(5,1000,2000);
  ESC4.attach(6,1000,2000);
  ESC5.attach(7,1000,2000);
  ESC6.attach(8,1000,2000);

  Arm.attach(9);
  dht.begin();

//relay startup mode
  pinMode(relayPin1,OUTPUT);
  digitalWrite(relayPin1,HIGH);

  delay(2000);
}

void loop() {
  currentMillis = millis();
  String receivedString; // string that is received from nodemcu through serial
  if (Serial.available() > 0) { //if rx is connected to mcu's tx
    // Read the incoming data and store it in a local variable
    receivedString = Serial.readStringUntil('\n');
    
    // Print the received string
    if (receivedString.startsWith("P")) {
      // Print and store the received string
      Serial.println(receivedString);
  //set speed of motors, actuators 
      ArmAngle=parsePot(receivedString);

      if(parseSink(receivedString)){
        ESC1Speed=parseHoverspeed(receivedString) + 20;
        ESC2Speed=ESC1Speed;
        ESC3Speed=ESC1Speed;
        ESC4Speed=ESC1Speed;
      }
      else if(parseTiltl(receivedString)){
        ESC1Speed=parseHoverspeed(receivedString) + 10;
        ESC3Speed=ESC1Speed;
        ESC2Speed=parseHoverspeed(receivedString) - 10;
        ESC4Speed=ESC2Speed;
      }
      else if(parseTiltr(receivedString)){
        ESC1Speed=parseHoverspeed(receivedString) - 10;
        ESC3Speed=ESC1Speed;
        ESC2Speed=parseHoverspeed(receivedString) + 10;
        ESC4Speed=ESC2Speed;
      }
      else if(parseHovermode(receivedString)){
      ESC1Speed=parseHoverspeed(receivedString);
      ESC2Speed=ESC1Speed;
      ESC3Speed=ESC1Speed;
      ESC4Speed=ESC1Speed;
      }
      else{
      ESC1Speed=0;
      ESC2Speed=0;
      ESC3Speed=0;
      ESC4Speed=0;
      }

      if(parseFw(receivedString)){
        //adjust relay for fw direciton
        //relay code
        digitalWrite(relayPin1,HIGH);
        //relay code end
      //adjust speed
      ESC5Speed=parseXyspeed(receivedString);
      ESC6Speed=ESC5Speed;
      }
      else if(parseBw(receivedString)){
          //relay code
        digitalWrite(relayPin1,LOW);
          //relay code end
      ESC5Speed=parseXyspeed(receivedString);
      ESC6Speed=ESC5Speed;
      }
      else if(parseLeft(receivedString)){
      //relay logic(optional)
      // ESC5Speed=parseXyspeed(receivedString); //uncomment if you managed to reverse it's dir
      ESC6Speed=parseXyspeed(receivedString);
      }
      else if(parseRight(receivedString)){
        ESC5Speed=parseXyspeed(receivedString);
        // ESC5Speed=parseXyspeed(receivedString); //uncomment if you managed to reverse it's dir
    }
    else{
       ESC5Speed=0;
      ESC6Speed=0;
    }
  }

//send sensor data to nodemcu via serial
//Sample data:

  if(currentMillis-previousMillis_send > interval_send){
    String sensorData;
    byte temp= dht.readTemperature();
//append to string
    sensorData+="S:";
    sensorData+="T";
    sensorData+=temp;
    sensorData+="E"; //End of the string
    
    Serial.println(sensorData);
    sendonSerial();
    previousMillis_send = currentMillis;
  }



// write speed of motors
    Arm.write(ArmAngle);

    ESC1.write(ESC1Speed);
    ESC2.write(ESC2Speed);
    ESC3.write(ESC3Speed);
    ESC4.write(ESC4Speed);
    ESC5.write(ESC5Speed);
    ESC6.write(ESC6Speed);
}
}

void sendonSerial(){
  Serial.println("esc1:");
  Serial.println(ESC1Speed);
    Serial.println("esc2:");
  Serial.println(ESC2Speed);
    Serial.println("esc3:");
  Serial.println(ESC3Speed);
    Serial.println("esc4:");
  Serial.println(ESC4Speed);
    Serial.println("esc5:");
  Serial.println(ESC5Speed);
      Serial.println("esc6:");
  Serial.println(ESC6Speed);
    Serial.println("arm:");
  Serial.println(ArmAngle);
}

//Parsing Functions for Commands
//  String sCommands="P0F0B0L0R0Tl0Tr0S0M2ld0Hm0Hs72xy171E";
int parsePot(String cmd){
  cmd.remove(cmd.indexOf("F"));
  cmd.remove(cmd.indexOf("P"),1);
  return cmd.toInt();
}

int parseFw(String cmd){
  cmd.remove(0,cmd.indexOf("F")+1);
  cmd.remove(cmd.indexOf("B"));
  return cmd.toInt();
}
int parseBw(String cmd){
  cmd.remove(0,cmd.indexOf("B")+1); //Previous letter:B
  cmd.remove(cmd.indexOf("L"));     //Letter After:L
  return cmd.toInt();
}

int parseLeft(String cmd){
  cmd.remove(0,cmd.indexOf("L")+1);
  cmd.remove(cmd.indexOf("R"));
  return cmd.toInt();
}
int parseRight(String cmd){
    cmd.remove(0,cmd.indexOf("R")+1);
  cmd.remove(cmd.indexOf("Tl"));
  return cmd.toInt();
}

int parseTiltl(String cmd){
 cmd.remove(0,cmd.indexOf("Tl")+2);
  cmd.remove(cmd.indexOf("Tr"));
  return cmd.toInt();
}
int parseTiltr(String cmd){
  cmd.remove(0,cmd.indexOf("Tr")+2);
  cmd.remove(cmd.indexOf("S"));
  return cmd.toInt();
}
int parseSink(String cmd){
 cmd.remove(0,cmd.indexOf("S")+1); //Previous letter:B
  cmd.remove(cmd.indexOf("M"));     //Letter After:L
  return cmd.toInt();
}
//  String sCommands="P0F0B0L0R0Tl0Tr0S0M2ld0Hm0Hs72xy171E";
int parseMode(String cmd){
    cmd.remove(0,cmd.indexOf("M")+1);
    cmd.remove(cmd.indexOf("ld"));
  return cmd.toInt();
}
int parseLed(String cmd){
  cmd.remove(0,cmd.indexOf("ld")+2);
  cmd.remove(cmd.indexOf("Hm"));
  return cmd.toInt();
}
int parseHovermode(String cmd){
  cmd.remove(0,cmd.indexOf("Hm")+2);
  cmd.remove(cmd.indexOf("Hs"));
  return cmd.toInt();
}
int parseHoverspeed(String cmd){
  cmd.remove(0,cmd.indexOf("Hs")+2);
  cmd.remove(cmd.indexOf("xy"));
  return cmd.toInt();
}
int parseXyspeed(String cmd){
  cmd.remove(0,cmd.indexOf("xy")+2);
  cmd.remove(cmd.indexOf("E"));
  return cmd.toInt();
}



//Parsing Functions for Commands End

