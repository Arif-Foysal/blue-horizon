/*
  Flow of data transfer via ESP-Now
  1 - Put WiFi in STA Mode
  2 - Intialize ESPNOW
  3 - Set Role to Combo
  4 - Add peer device
  5 - Define Send Callback Function
  6 - Define Receive Callback Function
*/

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <LiquidCrystal_I2C.h>  

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
//define all the pins
//pot pin
#define potPin A0

//Button pins
#define btnFw D0
#define btnBw D3
#define btnLeft D4
#define btnRight D5
#define btnTl D7
#define btnTr D6
#define btnSink 3  //GPIO 3

//Menu VariableS
  byte menuItem = 1;
  byte menuState = 0;

//prev button states in menu
  bool prevBtnFwState = LOW;
  bool prevBtnBwState = LOW;
  bool prevBtnTlState = LOW;
  bool prevBtnTrState = LOW;

  bool prevBtnTlTrState = LOW; //prev status of Tl and Tr at the same time
//prev button states in menu end

String modes[] = { "NORMAL", "SPORTS", "CUSTOM" };
byte mode = 0;  //Normal Mode on startup

String hoverModes[] = { "OFF", "ON" };  //sets the rover hover on water.
byte hoverMode = 1;                     //Auto hover is "ON" on startup

String ledModes[] = { "OFF", "ON" };
byte ledMode = 0;  //LED OFF on startup

//Thrusts
byte hoverSpeed;//for sending over espnow
byte xySpeed;//for sending over espnow

byte hovThrust = 30; //same for normal mode and sports mode. Changable if modes[mode] == "CUSTOM"

byte xyThrustNormal = 40; // Not changable
byte xyThrustSports = 50; // Not changable

byte hovThrustCustom = hovThrust;// Changable if modes[mode] == "CUSTOM"
byte xyThrustCustom = xyThrustNormal;  // Changable if modes[mode] == "CUSTOM"
//Menu Variables end

//connection
bool connected = false;


// REPLACE WITH THE MAC Address of your receiver 2C:F4:32:70:13:54
uint8_t broadcastAddress[] = { 0x48, 0x3F, 0xDA, 0x5F, 0xDF, 0xEA };
//below structure is outgoing
typedef struct struct_commands {
  //pot value
  byte potvalue;
  //button status
  // Variable to store the state of the pushbutton
  bool btnFwPressed = 0;
  bool btnBwPressed = 0;
  bool btnLeftPressed = 0;
  bool btnRightPressed = 0;
  bool btnTlPressed = 0;
  bool btnTrPressed = 0;
  bool btnSinkPressed = 0;
  //Modes
  byte mode;
  byte hoverMode;
  //led
  byte led;
  //thrust speeds
  byte hoverSpeed;
  byte xySpeed;
} struct_commands;
// Create a struct_commands called outgoingCommands to hold outgoing data
// Create a struct_commands called incomingData to hold incoming data
typedef struct struct_data {
  byte data;
  byte temp;

} struct_data;

struct_commands outgoingCommands;
struct_data inData;

// Variable to store if sending data was successful
String success;
// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  if (sendStatus == 0) {
    // Serial.println("Delivery success");
    connected = true;
  }
  else {
    connected = false;
    // menuState=0;
    Serial.println("Delivery fail");
    lcd.clear();
    lcd.setCursor(2, 0);  //Set cursor to character 2nd column on line 0
    lcd.print("Connecting...");
  }
}

// Callback when data is received

void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&inData, incomingData, sizeof(inData));
  // Serial.print("Bytes received: ");
  // Serial.println(len);
  // Serial.print("Temp: ");
  // Serial.println(inData.data);
  // Serial.print(inData.temp);
  // Serial.println(" C");
  //clear the lcd screen and print the sensor data
  if (menuItem == 0 && menuState == 0) {  //if data received and not on any menu
    lcd.clear();
    //print sensor data
  }
}
void setup() {
  // Init Serial Monitor
  Serial.begin(9600);

  //set pinmode of button pins as INPUT
  pinMode(btnFw, INPUT);
  pinMode(btnBw, INPUT_PULLUP);
  pinMode(btnLeft, INPUT_PULLUP);
  pinMode(btnRight, INPUT);
  pinMode(btnTl, INPUT);
  pinMode(btnTr, INPUT);
  pinMode(btnSink, INPUT);

  //set pinmode of pot pin
  pinMode(potPin, INPUT);

  //lcd display
  lcd.init();       //initialize the
  lcd.clear();      //clar the lcd screen
  lcd.backlight();  // Make sure backlight is on


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
//delay management
unsigned long previousMillis_pot = 0;  // Store the previous time
const long sendInterval = 50;          // Set the sendInterval in milliseconds
//menu update interval
unsigned long previousMillis_display = 0;
const long updateDisplayInterval = 500;
//press & hold change in menu interval
unsigned long previousMillis_change=0;
const long updateValueInterval=300;
//serial event 
unsigned long previousMillis_serial=0;
const long serialInterval=400;
//Note: previousMillis and interval should be different for every delay event
void loop() {
  unsigned long currentMillis = millis();


  //menu toggle
  if ((!digitalRead(btnLeft) && digitalRead(btnRight)) && prevBtnTlTrState == LOW) {
    toggleMenu();
  }
  prevBtnTlTrState= (!digitalRead(btnLeft) && digitalRead(btnRight));

  if (connected && menuState) {
    if (currentMillis - previousMillis_display > updateDisplayInterval) {
      updateMenu();
      previousMillis_display = currentMillis;
    }
  } else if (connected && !menuState) {  // if not in menu state, print this on display
    if (currentMillis - previousMillis_display > updateDisplayInterval) {
      lcd.clear();
      //first row
      lcd.setCursor(0, 0);
      lcd.print("TEMP:");
      lcd.print(inData.temp);
      lcd.print("C");
      lcd.setCursor(10, 0);
      lcd.print("MODE:");
      lcd.print(modes[mode].charAt(0));  //replace with actual Mode
      //second row
      lcd.setCursor(0, 1);
      lcd.print("TH:");
      lcd.print("H=");  //replace with axis
      if(modes[mode]=="CUSTOM"){
        lcd.print(String(hovThrustCustom));
        
      }
      else{
        lcd.print(String(hovThrust));
      }
      lcd.print("%");
      lcd.setCursor(10, 1);
      lcd.print("XY:");
      if(modes[mode]=="CUSTOM"){
        lcd.print(String(xyThrustCustom));
      }
      else if(modes[mode]=="NORMAL"){
        lcd.print(String(xyThrustNormal));
      }
      else if(modes[mode]=="SPORTS"){
        lcd.print(String(xyThrustSports));
      }
      lcd.print("%");
      previousMillis_display = currentMillis;
    }
  }

//Menu Navigation
  //Bw button funciton in menu
  if (menuState && (!digitalRead(btnBw)) && prevBtnBwState == LOW) {
    menuItem++;
  }
  prevBtnBwState=!digitalRead(btnBw);
  //Fw button funciton in menu
  if (menuState && (digitalRead(btnFw)) && prevBtnFwState == LOW) {
    menuItem--;
  }
  prevBtnFwState = digitalRead(btnFw);
  //Modes navigation in menu
  if (menuState && menuItem == 1) {
    if (digitalRead(btnTr) && prevBtnTrState == LOW) {
      mode++;
    } else if (digitalRead(btnTl) && prevBtnTlState == LOW) {
      mode--;
    }
    prevBtnTrState = digitalRead(btnTr);
    prevBtnTlState = digitalRead(btnTl);
  }
  //Auto Hover navigation in menu
  if (menuState && menuItem == 2) {
    if (digitalRead(btnTr) && prevBtnTrState == LOW) {
      hoverMode++;
    } else if (digitalRead(btnTl) && prevBtnTlState == LOW) {
      hoverMode--;
    }
    prevBtnTrState = digitalRead(btnTr);
    prevBtnTlState = digitalRead(btnTl);
  }
  //LED navigation in menu
  if (menuState && menuItem == 3) {
    if (digitalRead(btnTr) && prevBtnTrState == LOW) {
      ledMode++;
    } else if (digitalRead(btnTl) && prevBtnTlState == LOW) {
      ledMode--;
    }
    prevBtnTrState = digitalRead(btnTr);
    prevBtnTlState = digitalRead(btnTl);
  }
  //hovThrustCustom navigation in menu
  if (menuState && menuItem == 4 && (currentMillis - previousMillis_change > updateValueInterval)){
        if (digitalRead(btnTr)) {
             hovThrustCustom++;
          }
        else if (digitalRead(btnTl)) {
      hovThrustCustom--;
    }
    previousMillis_change=currentMillis;
  }
  //xyThrustCustom navigation in menu
  if (menuState && menuItem == 5 && (currentMillis - previousMillis_change > updateValueInterval)){
        if (digitalRead(btnTr)) {
             xyThrustCustom++;
          }
        else if (digitalRead(btnTl)) {
      xyThrustCustom--;
    }
    previousMillis_change=currentMillis;
  }

  // Check if it's time to send the data
  if (currentMillis - previousMillis_pot >= sendInterval) {
    previousMillis_pot = currentMillis;  // Save the last time you sent the data

    // Your code to send data goes here
    // strcpy(outgoingCommands.msg, "Hello from NodeMCU");
//set payload
    outgoingCommands.potvalue = map(analogRead(potPin), 10, 1023, 0, 65);  //read pot value
    outgoingCommands.mode = mode;
    outgoingCommands.hoverMode = hoverMode;
    //led
    outgoingCommands.led=ledMode;
        //THRUST SPEEDS
        if(modes[mode]=="CUSTOM"){
        hoverSpeed=map(hovThrustCustom, 0, 100, 0, 180);
        outgoingCommands.hoverSpeed=hoverSpeed;
        xySpeed=map(xyThrustCustom, 0, 100, 0, 180);
        outgoingCommands.xySpeed=xySpeed;
        }
        else if(modes[mode]=="NORMAL"){
          hoverSpeed=map(hovThrust, 0, 100, 0, 180);
          outgoingCommands.hoverSpeed=hoverSpeed;
          xySpeed=map(xyThrustNormal, 0, 100, 0, 180);
          outgoingCommands.xySpeed=xySpeed;
        }
        else if(modes[mode]=="SPORTS"){
          hoverSpeed=map(hovThrust, 0, 100, 0, 180);
          outgoingCommands.hoverSpeed=hoverSpeed;
          xySpeed=map(xyThrustSports, 0, 100, 0, 180);
          outgoingCommands.xySpeed=xySpeed;
        }
    if(menuState){
      outgoingCommands.btnFwPressed = 0;
      outgoingCommands.btnBwPressed = 0;
      outgoingCommands.btnLeftPressed = 0;
      outgoingCommands.btnRightPressed = 0;
      outgoingCommands.btnTlPressed = 0;
      outgoingCommands.btnTrPressed = 0;
      outgoingCommands.btnSinkPressed = 0;
    }
    else{
      outgoingCommands.btnFwPressed = digitalRead(btnFw);
      outgoingCommands.btnBwPressed = !digitalRead(btnBw);
      outgoingCommands.btnLeftPressed = !digitalRead(btnLeft);
      outgoingCommands.btnRightPressed = digitalRead(btnRight);
      outgoingCommands.btnTlPressed = digitalRead(btnTl);
      outgoingCommands.btnTrPressed = digitalRead(btnTr);
      outgoingCommands.btnSinkPressed = digitalRead(btnSink);
    }

    //set payload end
   
    esp_now_send(broadcastAddress, (uint8_t *)&outgoingCommands, sizeof(outgoingCommands));
    
  }
   if (currentMillis - previousMillis_serial > serialInterval) {
      printOnSerial();
      previousMillis_serial=currentMillis;
    }
}

void toggleMenu() {  //toggles menu variables
  // if(!menuItem){menuItem=1;}
  // else{menuItem=0;}

  if (!menuState) {
    menuState = 1;
  } else if (menuState) {
    menuItem = 1;
    menuState = 0;
  }
}

void updateMenu() {
  //menuItem out of line
  if(mode[modes]=="CUSTOM"){
  if (menuItem < 1) { menuItem = 5; }
  if (menuItem > 5) { menuItem = 1; }
  }
  else{
      if (menuItem < 1) { menuItem = 3; }
      if (menuItem > 3) { menuItem = 1; }
  }
  //mode out of line
  if (mode < 0) { mode = 2; }
  if (mode > 2) { mode = 0; }
  //autohover out of line
  if (hoverMode < 0) { hoverMode = 0; }
  else if (hoverMode > 1) { hoverMode = 0; }
  //ledMode out of line
  if (ledMode < 0) { ledMode = 0; }
  else if(ledMode > 1){ ledMode = 0; }
  //Thrusts out of line
  if(hovThrustCustom>99){hovThrustCustom=0;}
  if(xyThrustCustom>99){xyThrustCustom=0;}

  switch (menuItem) {
    case 1:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">MODE   <" + modes[mode] + ">");  //item1
      lcd.setCursor(0, 1);
      lcd.print(" A.HOVER   <" + hoverModes[hoverMode] + ">");  //item2
      break;
    case 2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" MODE   <" + modes[mode] + ">");  //item1
      lcd.setCursor(0, 1);
      lcd.print(">A.HOVER   <" + hoverModes[hoverMode] + ">");  //item2
      break;
    case 3:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">LED       <" + ledModes[ledMode] + ">");//item3
      if(modes[mode]=="CUSTOM"){
        lcd.setCursor(0, 1);
        lcd.print(" HOV.THRUST<"+String(hovThrustCustom)+"%>");//item4
      }
      break;
    case 4:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" LED       <" + ledModes[ledMode] + ">");//item3
      if(modes[mode]=="CUSTOM"){
        lcd.setCursor(0, 1);
        lcd.print(">HOV.THRUST<"+String(hovThrustCustom)+"%>");//item4
      }
      break;
    case 5:
      lcd.clear();
      if(modes[mode]=="CUSTOM"){
      lcd.setCursor(0, 0);
      lcd.print(">XY.THRUST <"+ String(xyThrustCustom) +"%>");//item5
      }
      break;
      // lcd.setCursor(0, 1);
      // lcd.print(" ITEM 6");//item6
  }
}

void printOnSerial(){
    Serial.println("pot value:");
    Serial.println(outgoingCommands.potvalue);
    Serial.print("MenuState:");
    Serial.println(menuState);
    Serial.print("HoverItem: ");
    Serial.println(menuItem);
    Serial.print("Mode:");
    Serial.println(mode);
    Serial.print("Hover Speed: ");
    Serial.println(hoverSpeed);
    Serial.print("XY speed: ");
    Serial.println(xySpeed);
}
