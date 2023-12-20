//==changes made==//
// - 
/*
  Flow of the code
  1 - Put WiFi in STA Mode
  2 - Intialize ESPNOW
  3 - Set Role to Combo
  4 - Add peer device
  5 - Define Send Callback Function
  6 - Define Receive Callback Function
*/
#include <ESP8266WiFi.h>
#include <espnow.h>


//define all the pins 



//delay management//
  //current millis
  unsigned long currentMillis;
  //send on controller delay
    unsigned long previousMillis_send = 0; //last time sent on controller
    const long interval_send = 100;
  //send on serial delay
    unsigned long previousMillis_send_serial=0;
    const long interval_send_serial=100;
//delay management end//
//Note: previousMillis and interval should be different for every delay event


// REPLACE WITH THE MAC Address of your receiver
uint8_t broadcastAddress[] = {0xC8, 0x2B, 0x96, 0x1C, 0xFE, 0x67};
//structure of incoming packet
typedef struct struct_commands {
  //pot value
  byte potvalue;
//button status
// Variable to store the state of the pushbutton
  bool btnFwPressed=0;
  bool btnBwPressed=0;
  bool btnLeftPressed=0;
  bool btnRightPressed=0;
  bool btnTlPressed=0;
  bool btnTrPressed=0;
  bool btnSinkPressed=0; 
  //Mode
  byte mode;
  byte hoverMode;
   //led
  byte led;
  //thrust speeds
  byte hoverSpeed;
  byte xySpeed;

} struct_commands;

// structure of the outgoing packet
typedef struct struct_data {
byte incInt;
byte temp;
} struct_data;

// Create a struct_commands called outgoingData to hold outgoing data
struct_data outgoingData;


// Create a struct_commands called incomingCommands to hold incoming data
struct_commands incomingCommands;
// Variable to store if sending data was successful
String success;
// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  // Serial.print("Last Packet Send Status: "); 
  if (sendStatus == 0) {
    // Serial.println("Delivery success");
  }

  else {
    Serial.println("Delivery fail");
  }
}

// Callback when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&incomingCommands, incomingData, sizeof(incomingCommands));
//print no. of bytes received
  // Serial.print("Bytes received: ");
  // Serial.println(len);

//send incoming data to serial on an interval
 if (currentMillis - previousMillis_send_serial >= interval_send_serial) {
  sendonSerial();

}

}
void setup() {

  // Init Serial Monitor
  Serial.begin(115200);

  //set pinmode

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 0, NULL, 0);
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  currentMillis = millis();

  String sensorData; //Sensor data from arduino

//send on controller start
  outgoingData.incInt=194;
  if (currentMillis - previousMillis_send >= interval_send) {
  previousMillis_send=currentMillis;
  esp_now_send(broadcastAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));
  }
//send on controller end

  if (Serial.available() > 0) {
    sensorData = Serial.readStringUntil('\n');
    //print the sensor data
    if(sensorData.startsWith("S")){
        // Serial.println("Serial data received");
        // Serial.println(sensorData);
        //send to controller
        outgoingData.temp=parseTemp(sensorData);
    }
  }


// print random data on serial to test if debugging possible (start)

}

void sendonSerial(){
// == Flow of this Function ==
// - Create a string containing all the values
// - Send the string via serial 
// - Sample Structure of the string: "P0F0B0L0R0Tl0Tr0S0M2ld0Hm0Hs72xy171E"

//read incoming data and append it to string
  String SerialCmd;

  //adding pot
  SerialCmd+="P";
  SerialCmd+=incomingCommands.potvalue;
  //adding fw
  SerialCmd+="F";
  SerialCmd+=incomingCommands.btnFwPressed;
  //adding bw
  SerialCmd+="B";
  SerialCmd+=incomingCommands.btnBwPressed;
  //adding left
  SerialCmd+="L";
  SerialCmd+=incomingCommands.btnLeftPressed;
  //adding right
  SerialCmd+="R";
  SerialCmd+=incomingCommands.btnRightPressed;
  //adding tilt left
  SerialCmd+="Tl";
  SerialCmd+=incomingCommands.btnTlPressed;
  //adding tilt right
  SerialCmd+="Tr";
  SerialCmd+=incomingCommands.btnTrPressed;
  //adding sink
  SerialCmd+="S";
  SerialCmd+=incomingCommands.btnSinkPressed;
  //adding mode
  SerialCmd+="M";
  SerialCmd+=incomingCommands.mode;
  //adding led
  SerialCmd+="ld";
  SerialCmd+=incomingCommands.led;
  //adding hover mode
  SerialCmd+="Hm";
  SerialCmd+=incomingCommands.hoverMode;
  //adding hover Speed
  SerialCmd+="Hs";
  SerialCmd+=incomingCommands.hoverSpeed;
  //adding xy speed
  SerialCmd+="xy";
  SerialCmd+=incomingCommands.xySpeed;
  //End of the string
  SerialCmd+="E";
Serial.println(SerialCmd);

}

//parse sensor data funcitons
int parseTemp(String cmd){
  cmd.remove(0,cmd.indexOf("T")+1);
  cmd.remove(cmd.indexOf("E"));//end of the string
  return cmd.toInt();
}

