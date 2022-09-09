#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#include "RunningAverage.h"
#define GREEN_LED D0
#define RED_LED D3
#define AC_OUT_RELAY D1  
#define INVERTER_ON_RELAY D2  
#define CHANGEOVER_OUT D5

//#define GOOD_BAT_START 13
#define SOLAR_ABLE 13.6
#include "webpage.h" //Our HTML webpage contents with javascripts

//SSID and Password of your WiFi router
const char* ssid = "home";
const char* password = "23456789";
//...................................//
bool manualOveride = false;
float LOW_BAT_CUTOFF = 12.1;
float GOOD_BAT_START = 12.4;
bool SafeToStartInverter = false;
bool InverterStarted =false;
float batV = 0;
float batVR=0;
float ADC_READ = 0;
int ChangeOverState = 0;
String SYSstate = "Started";
bool LowBattery = false;
unsigned long previousMillis = 0;
unsigned long interval = 30000;
unsigned long InverterUp = 0;
unsigned long runTime = 0;
unsigned long resetInterval = 0;

const int ADD_AC_OUT_RELAY_STATE = 0;
const int ADD_INVERTER_ON_RELAY_STATE = 1;
char inverterRelayPreviousState = 0;
char ACOutRelayPreviousState = 0;
//......................................//

ESP8266WebServer server(80); //Server on port 80


RunningAverage myRA(300);

void setup() {
 Serial.begin(115200);
 WiFi.begin(ssid, password);     //Connect to your WiFi router
 EEPROM.begin(2);
 Serial.println("");
 myRA.clear();
 pinMode(GREEN_LED,OUTPUT);
 pinMode(RED_LED,OUTPUT);
 pinMode(AC_OUT_RELAY,OUTPUT);
 pinMode(INVERTER_ON_RELAY,OUTPUT);

 pinMode(A0,INPUT);
 pinMode(CHANGEOVER_OUT,INPUT);
 int t=0;
 while (WiFi.status() != WL_CONNECTED) {
    ON_GREEN();
    Serial.print(".");
    delay(5);
    t++;
    if (t>5000){
      break;
    }
  }
  OFF_GREEN();
  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
 
  server.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  server.on("/setInverter", handleInverter);
  server.on("/readADC", handleADC);
  server.on("/readSYS", handleSYS);
  server.onNotFound(handleNotFound);   
  server.begin();                  //Start server
  Serial.println("HTTP server started");
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  manualOveride = false;

  ACOutRelayPreviousState = EEPROM.read(ADD_AC_OUT_RELAY_STATE);
  inverterRelayPreviousState = EEPROM.read(ADD_INVERTER_ON_RELAY_STATE);
}
void serverRun(){
  server.handleClient();
  //Serial.println("server running.....");
}

void serverRestart(){
  server.stop();
  server.close();
  Serial.println("server restart");
  delay(10);
  server.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  server.on("/setInverter", handleInverter);
  server.on("/readADC", handleADC);
  server.on("/readSYS", handleSYS);
  server.onNotFound(handleNotFound); 
  server.begin(); 
}

void wifiReconnet(){
  WiFi.disconnect();
  WiFi.begin(ssid,password);
  Serial.println("server restart");
  server.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  server.on("/setInverter", handleInverter);
  server.on("/readADC", handleADC);
  server.on("/readSYS", handleSYS);
  server.onNotFound(handleNotFound);   
  server.begin(); 
}
void loop() {
  
int cl_state = server.client();
 if (cl_state == 0){
    resetInterval = 500000;
   
 }
 else {
  resetInterval = 2400000;
 }
 if (millis() - runTime > resetInterval){
  runTime = millis();
  //wifiReconnet();
  //serverRestart();
  Serial.println("server restart");
  EEPROM.write(ADD_AC_OUT_RELAY_STATE, ACOutRelayPreviousState);
  EEPROM.write(ADD_INVERTER_ON_RELAY_STATE, inverterRelayPreviousState);
  if (EEPROM.commit()){
    ESP.reset();
  }
 }
  else{
    serverRun();
  }
  
  ADC_READ = analogRead(A0);
  delay(2);
  int state = 0;
  int acc =0;
  for(int i = 0; i <15;i++){
  state = digitalRead(CHANGEOVER_OUT);
  acc = acc + state;
  }
  if (acc == 15 ){
    ChangeOverState = 1;
  }
  else if (acc < 8){
    ChangeOverState = 0;
  }
  //Serial.println(ChangeOverState);
  
  batV =0;
   
  batVR =  ADC_READ/30;
  myRA.addValue(batVR);
  batV = myRA.getAverage();

  
  if (batV > GOOD_BAT_START){
    ON_GREEN();
    OFF_RED();
    SafeToStartInverter = true;
    LowBattery = false;
    //SYSstate = "System good, start inverter";
    //
  }
  else if (batV < LOW_BAT_CUTOFF){
    ON_RED();
    OFF_GREEN();
    OFF_INVERTER();
    OFF_AC();
    LowBattery = true;
    SafeToStartInverter = false;
    InverterStarted = false;
    SYSstate = "Low battery, charge batery, inverter is Auto OFF";
    inverterRelayPreviousState =0;
   // delay(2000);
  }
  if(inverterRelayPreviousState ==1){
    InverterStarted = true;
    ON_INVERTER();
    SYSstate = "Inverter Running";
  }

 if ((ChangeOverState) & (SafeToStartInverter)& (!InverterStarted) & (!manualOveride) & (!LowBattery)){
  InverterUp = millis();
  LOW_BAT_CUTOFF = 11.75;      //to get the  changerover turned
  if(batV > SOLAR_ABLE){
    //LOW_BAT_CUTOFF = 12.55;
    GOOD_BAT_START = 13.6;
  }
  else{
    //LOW_BAT_CUTOFF = 12.25;
    GOOD_BAT_START = 12.7;
  }
      InverterStarted = true;
      ON_INVERTER();
      SYSstate = "Inverter Running";
      inverterRelayPreviousState =1;
  }
    
 if ((!ChangeOverState)& (!manualOveride)) {      //removed inverter started check - 
    OFF_INVERTER();
    InverterStarted = false;
    SYSstate = "Inverter Stopped, System good!";
    inverterRelayPreviousState =0;
  }
if ((millis() - InverterUp) > 10000)  {
  if (GOOD_BAT_START == 14){
    LOW_BAT_CUTOFF = 12.55;
  }
  else if (GOOD_BAT_START == 13){
    LOW_BAT_CUTOFF = 12.25;
  }
}

unsigned long currentMillis = millis();
// if WiFi is down, try reconnecting
if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
  //Serial.print(millis());
  //Serial.println("Reconnecting to WiFi...");
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  previousMillis = currentMillis;
}
}


void ON_GREEN(){
  digitalWrite(GREEN_LED,HIGH);
}
void OFF_GREEN(){
  digitalWrite(GREEN_LED,LOW);
}
void ON_RED(){
  digitalWrite(RED_LED,HIGH);
}
void OFF_RED(){
  digitalWrite(RED_LED,LOW);
}
void ON_AC(){
  digitalWrite(AC_OUT_RELAY,HIGH);
}
void OFF_AC(){
  digitalWrite(AC_OUT_RELAY,LOW);
}
void ON_INVERTER(){
  digitalWrite(INVERTER_ON_RELAY,HIGH);
}
void OFF_INVERTER(){
  digitalWrite(INVERTER_ON_RELAY,LOW);
}

//===============================================================
// This routine is executed when you open its IP in browser
//===============================================================
void handleRoot() {
 String s = MAIN_page; //Read HTML contents
 server.send(200, "text/html", s); //Send web page
}

void handleADC() {
 float ADC_READ2 = batV;
 
 String adcValue = String(ADC_READ2);
 server.send(200, "text/plane", adcValue); //Send ADC value only to client ajax request
}
void handleSYS() {
 server.send(200, "text/plane", SYSstate); //Send ADC value only to client ajax request
}
void handleInverter() {
 String Inverterstate = "OFF";
 String t_state = server.arg("Inverterstate"); //Refer  xhttp.open("GET", "setLED?LEDstate="+led, true);
 //Serial.println(t_state);
 if(t_state == "1")
 {
  manualOveride = true;
  ON_INVERTER();
  InverterStarted = true;
  Inverterstate = "Inverter ON"; //Feedback parameter
  SYSstate = "Remote operation";
 }
 else if(t_state == "0")
 {
  manualOveride = true;
  InverterStarted = false;
  OFF_INVERTER();
  Inverterstate = "Inverter OFF"; //Feedback parameter  
  SYSstate = "Remote operation";
  inverterRelayPreviousState =0;
 }
  else if(t_state == "2")
 {
  ON_AC();
  Inverterstate = "Grid ON"; //Feedback parameter  
  SYSstate = "Remote operation";
 }

 else if(t_state == "3")
 {
  OFF_AC();
  Inverterstate = "Grid OFF"; //Feedback parameter  
  SYSstate = "Remote operation";
 }
 else if(t_state == "4")
 {
  manualOveride = false;
//  if (InverterStarted){
//    OFF_INVERTER();
//    delay(1000);
//  }
  OFF_AC();
  Inverterstate = "Auto"; //Feedback parameter  
  SYSstate = "Auto mode active";
 }
 
 server.send(200, "text/plane", Inverterstate); //Send web page
} 


void handleNotFound(){
  server.send(404, "text/plain", "404: Not found error"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
