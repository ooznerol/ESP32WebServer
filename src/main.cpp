#include <Arduino.h>


// #include <Preferences.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <HTTPClient.h>

#include <Esp32WifiManager.h>

#include "SSD1306.h"
#include "EEPROM.h"

// #include "SimpleDHT.h"
#include "DHTesp.h"


#include <TimeLib.h>

time_t pctime = 0;


String buildScript();
void handleRelay(int RelayIndex,int state);
void handleRoot();
void UPadteLCDDisplay();
String uptime();
void readDHT22();
void GetTimeFromServer();
void handleSubmit();


unsigned long timeNow = 0;
unsigned long timeLast = 0;
int Year = 0;
int Month = 0;
int Day = 0;
int Hour = 0;
int Minute = 0;
int Seconde = 0;

bool  OTA_Request = false;
bool bIPDomoticz_is_OK = true;

const char* host = "esp8266-webupdate";//#


const char* ssid = "OpenWrt24";
const char* password = "MeyrilZoeCharlotte";

// int pinDHT22 = D2;
// SimpleDHT22 dht22;
DHTesp dht;



// Initialize the OLED display using Wire library
SSD1306  display(0x3c,5, 4);



float temperature = 0;
float humidity = 0;

#define NUMBEROFRELAY 4

#define EEPROM_ADDR_DOMOTICZ_IP1       0x00
#define EEPROM_ADDR_DOMOTICZ_IP2       0x01
#define EEPROM_ADDR_DOMOTICZ_IP3       0x02
#define EEPROM_ADDR_DOMOTICZ_IP4       0x03
#define EEPROM_ADDR_DOMOTICZ_ID_DHT    0x04
#define EEPROM_ADDR_Delay_Measure_DHT  0x05
#define EEPROM_ADDR_RELAY              0x06


//https://github.com/nodemcu/nodemcu-devkit-v1.0
//GPIO00 = D3  : LCD I2C
//GPIO01 =     : TXD0
//GPIO02 = D4  : Led Top/TXD1
//GPIO03 =     : RXD0
//GPIO04 = D2  : DHT22
//GPIO05 = D1  : Free
//GPIO06 = NC
//GPIO07 = NC
//GPIO08 = NC
//GPIO09 = SD2 : Relay1
//GPIO10 = SD3 : Relay2
//GPIO11 = NC
//GPIO12 = D6  : Free
//GPIO13 = D7  : TXD2 / Relay3;
//GPIO14 = D5  : LCD I2C
//GPIO15 = D8  : RXD2 / Relay4;
//GPIO16 = D0  : led PIN


int tab_Relay_Pin[NUMBEROFRELAY] = {14 ,12 ,15,13 };

unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 1000;

unsigned long time0; // var created to show uptime more close to zero milliseconds as possible


String ipStr = "000.000.000.000";
String ipDomoticzStr = "000.000.000.000";
#define LISTENPORT 80
WebServer server(LISTENPORT);

//const int LEDPIN = D0;

String UpTimeVar = "";




String buildHTMLPage()
{
  Serial.println("*********buildHTMLPage Start*******");
  EEPROM.begin(128);// Initialising the UI will init the display too.
  //  String page = "<html lang=fr-FR><head><meta http-equiv='refresh' content='10'/>";
  String page = "<html lang=fr-FR><head><content='10'/>";
  page += "<html><head><title>ESP8266 IoT</title></head><meta name=viewport content='width=500'>\n<style type='text/css'>\nbutton {line-height: 1.8em; margin: 5px; padding: 3px 7px;}";
  page += "body {text-align:center;}\ndiv {border:solid 1px; margin: 3px; width:150px;}\n.center { margin: auto; width: 400px; border: 3px solid #73AD21; padding: 3px;}\nhr {width:400px;}\n</style></head>";
  //  page += "</style></head>\n<h3 style=\"height: 15px; margin-top: 0px;\"><a href='/'>STM32 & ENC28J60 SWITCH</h3><h3 style=\"height: 15px;\">\n";
  page += "</b>";
  page += "<div class='center'>";
  page += "<h3>DHT22</h3>";
  page += "Temperature : ";
  page += temperature;
  page += "C\n";
  page += "Humidite : ";
  page += (uint16_t)humidity;
  page +="%RH"; 
  page += "<br/></div><hr>\n";

  page += "UpTime : ";
  page += UpTimeVar;
  page += "</b><hr>\n";

  page += "<div class='center'>";
  for (uint8_t RelayIndex = 1 ; RelayIndex <= NUMBEROFRELAY ; RelayIndex++)
  {
    page += "<label>\n";
    uint8_t RelayStatus;
    RelayStatus = digitalRead(tab_Relay_Pin[RelayIndex - 1]);

    page += "<input type=\"checkbox\" id=\"RELAY";
    page += String(RelayIndex);


    if (RelayStatus == 1)
      page += "\" value=\"checkbox1\" onClick=\"handleClick(this)\" >\n";
    else
      page += "\" value=\"checkbox1\" onClick=\"handleClick(this)\" checked=\"checked\">\n";
    page += "RELAY";
    page += String(RelayIndex);
    page += "</label>\n";

  }
  page += "<br/></div><hr>\n";



  //input IP domoticz
  page += "<FORM ACTION='/' method=get >";
  page += "IP Domoticz: <INPUT TYPE=TEXT NAME='IPDomoticz' VALUE='";
  uint8_t IP[4];
  IP[0] = EEPROM.read(EEPROM_ADDR_DOMOTICZ_IP1);
 
  IP[1] = EEPROM.read(EEPROM_ADDR_DOMOTICZ_IP2);
 
  IP[2] = EEPROM.read(EEPROM_ADDR_DOMOTICZ_IP3);
  
  IP[3] = EEPROM.read(EEPROM_ADDR_DOMOTICZ_IP4);
  

  ipDomoticzStr = String(IP[0]) + '.' + String(IP[1]) + '.' + String(IP[2]) + '.' + String(IP[3]);
  if (ipDomoticzStr.startsWith("255."))
   bIPDomoticz_is_OK = false;
  if (ipDomoticzStr.startsWith("0.0.0.0"))
  bIPDomoticz_is_OK = false;

  Serial.print("IP Domoticz=");Serial.println(ipDomoticzStr);
  if (!bIPDomoticz_is_OK)
  Serial.println("IP Domoticz problem detected");

  // Serial.print("IP0 EEPROM READ=");Serial.println(IP[0]);
  // Serial.print("IP1 EEPROM READ=");Serial.println(IP[1]);
  // Serial.print("IP2 EEPROM READ=");Serial.println(IP[2]);
  // Serial.print("IP3 EEPROM READ=");Serial.println(IP[3]);

  char buffer [4];
  String IPstring = itoa(IP[0], buffer, 10);
  IPstring += ".";
  IPstring += itoa(IP[1], buffer, 10);
  IPstring += ".";
  IPstring += itoa(IP[2], buffer, 10);
  IPstring += ".";
  IPstring += itoa(IP[3], buffer, 10);
  page += IPstring;


  page += "' SIZE='25' MAXLENGTH='50'><BR>";
  page += "Device ID: <INPUT TYPE=TEXT NAME='ID' VALUE='";
  uint8_t buffer1 = EEPROM.read(EEPROM_ADDR_DOMOTICZ_ID_DHT);

  // Serial.print("ID EEPROM READ=");Serial.println(buffer1);

  page += buffer1;
  page += "' SIZE='5' MAXLENGTH='50'><BR>";

  page += "Delay: <INPUT TYPE=TEXT NAME='Delay' VALUE='";
  uint8_t bufferDelay = EEPROM.read(EEPROM_ADDR_Delay_Measure_DHT);
  page += bufferDelay;
  page += "' SIZE='5' MAXLENGTH='50'><BR>";
  page += "<INPUT TYPE=SUBMIT NAME='submit' VALUE='Save Config'>";
  page += "</FORM>";

  //button OTA Request
  page += "<FORM ACTION='/' method=get >";
  page += "<INPUT TYPE=SUBMIT NAME='submit' VALUE='OTA_Request'>";
  page += "</FORM>";

  // page += "<h3>IP Domoticz</h3>";
  // page += "<h3>Device ID</h3>";
  // page += "<h3>Delay</h3>";
  //  page += "<br><br><p><a hrf='https://www.projetsdiy.fr'>www.projetsdiy.fr</p>";
  page += "</body></html>";

  page += buildScript();
  EEPROM.end();
  Serial.println("*********buildHTMLPage end*******");



  return page;
}

String buildScript()
{
  Serial.println("*********buildScript Start*******");
  String page = "<script type=\"Text/JavaScript\">\n";
  page += "function handleClick(MycheckBox) {\n";
  //strcat(buffer,"display(\"Click, new value = \" + MycheckBox.checked);\n")); // for debug display command on html
  page += "if(MycheckBox.checked){\n";
  //build the cmd for javascipt ://client.println(F("url = 'http://192.168.0.151:8081' + '/?' + MycheckBox.id + '=ON';"));
  page += "url = 'http://" + ipStr + ":" + String(LISTENPORT);
  page += "/?' + MycheckBox.id + '=ON';\n";
  //client.println(F(httpcmd.c_str()));
  page += "}\n";

  page += "else{\n";
  //build the cmd for javascipt :client.println(F("url = 'http://192.168.0.151:8081' + '/?' + MycheckBox.id + '=OFF';"));
  page += "url = 'http://" + ipStr + ":" + String(LISTENPORT);
  page += "/?' + MycheckBox.id + '=OFF';\n";
  page += "}\n";
  page += "window.location.assign(url);\n";

  page += "}\n";
  page += "</script>\n";
  Serial.println("*********buildScript End*******");


  return page;

}





void handleRoot() {
  Serial.println("*********handleRoot Start*******");

  int numofargs = server.args();
  Serial.print("handleRoot number of args ");Serial.println(numofargs);
  //debug : print all args
  for (int i = 0 ; i < numofargs ; i ++)
  {
    Serial.print(server.argName(i));Serial.print("=");Serial.println(server.arg(i));
  }

  
  bool relayfound = false;
  for (int Relayindex = 1 ; Relayindex < sizeof(tab_Relay_Pin); Relayindex++)
  {
    String RelayName = "RELAY" + String(Relayindex);
    if ( server.hasArg(RelayName) ) {

      String PinState;
      //  LEDValue = server.arg("RELAY1");
      PinState = server.arg(RelayName);

      Serial.print("handleRoot Set Pin:"); 
      Serial.print(tab_Relay_Pin[Relayindex-1]);
      Serial.print( " to ");
      Serial.println(PinState) ;

      Serial.print("toggle RelayName= ");Serial.print(RelayName);Serial.print(" Pin=");Serial.println(tab_Relay_Pin[Relayindex-1]);
      if (PinState == "ON")
        handleRelay(Relayindex-1,1);
        else
        handleRelay(Relayindex-1,0);
      
      relayfound = true;
      break;
    }
  }
  if (relayfound ==false)
  {
    if ( server.hasArg("submit") ) {
      handleSubmit();
    }
    else {
      Serial.println("send page");
      server.send ( 200, "text/html", buildHTMLPage() );
    }
  }

  // if ( server.hasArg("RELAY1") ) {
  //   handleRelay(1);
  // }
  // else if ( server.hasArg("RELAY2") ) {
  //   handleRelay(2);
  // }
  // else if ( server.hasArg("RELAY3") ) {
  //   handleRelay(3);
  // }
  // else if ( server.hasArg("RELAY4") ) {
  //   handleRelay(4);
  // }
  // else if ( server.hasArg("submit") ) {
  //   handleSubmit();
  // }
  // else {
  //   Serial.println("send page");
  //   server.send ( 200, "text/html", buildHTMLPage() );
  // }
  Serial.println("*********handleRoot End*******");
  //UPadteLCDDisplay();
}

void handleSubmit()
{
  //Serial.println("handle Submit beging ");

  Serial.println("*********handleSubmit Start*******");
  if (server.arg(0) == "OTA_Request")

  {
    OTA_Request = true;
    ArduinoOTA.begin();                                               //#
    
  // Print le reseau et IP address                                                    //#
    Serial.print("Reseau:");  Serial.print(WiFi.SSID());  Serial.print(WiFi.RSSI()); Serial.println("dB"); //#

  }
  if (server.arg(3) == "Save Config")
  {
    EEPROM.begin(128);// Initialising the UI will init the display too.
    Serial.println("Saving Configuration");
    //char buffer[1];
    //save domoticz IP
    String IPDomototicz = server.arg(0);
    Serial.print("DomoticzIP=");Serial.println(IPDomototicz);

    uint8_t ip[4];
    char buf[15]; //make this the size of the String
    IPDomototicz.toCharArray(buf, 15);
    char * item = strtok(buf, ".");
    uint8_t index = 0;
    while (item != NULL) {
      if ((*item >= '0') && (*item <= '9')) {
        ip[index] = atoi(item);
        Serial.print("IP Write in EEPROM=");Serial.println(ip[index]);
        EEPROM.write(EEPROM_ADDR_DOMOTICZ_IP1 + index, ip[index]);
        //Serial.print("IP Read from EEPROM=");Serial.println(ip[index]);
        index++;
      }
      item = strtok(NULL, ".");
    }


    //save ID
    String ID= server.arg(1);
    uint8_t buffer1 = atoi(ID.c_str());
    //ID.toCharArray(buffer,1);
    Serial.print("ID Write EEPROM=");Serial.println(buffer1);
    EEPROM.write(EEPROM_ADDR_DOMOTICZ_ID_DHT, buffer1);

    //save delay
    String Delay= server.arg(2);
    uint8_t bufferDelay = atoi(Delay.c_str());
    //ID.toCharArray(buffer,1);
    Serial.print("Refresh interval Write EEPROM=");Serial.println(bufferDelay);
    EEPROM.write(EEPROM_ADDR_Delay_Measure_DHT, bufferDelay);
    EEPROM.end();



  }                                                    //#

  Serial.println("*********handleSubmit End*******");


  server.send ( 200, "text/html", buildHTMLPage() );
}




//exmaple http://192.168.0.24/?LED=1
void handleRelay(int RelayIndex,int state) {
  // Actualise le GPIO / Update GPIO

  digitalWrite(tab_Relay_Pin[RelayIndex], state);
  

  Serial.print("Set GPIO"); Serial.print(tab_Relay_Pin[RelayIndex]) ;Serial.print(" to ");Serial.println(state);
  // if ( PinValue == "ON" )
  // {
  //   digitalWrite(tab_Relay_Pin[RelayIndex], 0);
  // }
  // else if ( PinValue == "OFF" )
  // {
  //   digitalWrite(tab_Relay_Pin[RelayIndex], 1);
  // }
  server.send ( 200, "text/html", buildHTMLPage() );
  UPadteLCDDisplay();
}

//void handleRoot() {
//  digitalWrite(led, 1);
//  server.send(200, "text/plain", "hello from esp8266!");
//  digitalWrite(led, 0);
//}



void handleNotFound() {
  //digitalWrite(LEDPIN, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  //digitalWrite(LEDPIN, 0);
}


void loop(void) {

  server.handleClient();


  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    if (digitalRead(16) == LOW)
      digitalWrite(16, HIGH);// turn the LED off.(Note that LOW is the voltage level but actually 
    else
    {
      digitalWrite(16, LOW);
    }
    

    previousMillis = currentMillis;
    //readDHT22();

    UPadteLCDDisplay();

    
//    sendHTTP();

  }

   ArduinoOTA.handle();



}

void UPadteLCDDisplay()
{
  display.clear();

  //    display.drawString(0, 42,  "1:Off 2:Off 3:Off 4:Off");
  display.drawString(10, 0,  String(Day) + "/" + String(Month) + "/" + String(Year));
  char dataString[50] = {0};
  Day = day();
  Month = month();
  Year = year();
  Minute = minute();
  Seconde = second();
  Hour = hour();
  sprintf(dataString, "%02d:%02d:%02d", Hour, Minute, Seconde);
  display.drawString(63, 0,  dataString);

  display.drawString(0, 10,  "IP=" + ipStr);

  int32_t RSSI = WiFi.RSSI();
  display.drawString(90, 10, String(RSSI) + "dBm");

  // 
  // Serial.print( ":IP=" + ipStr );
  // Serial.print( " Status=" + String(WiFi.status()) );
  // Serial.println( " RSSI=" + String(RSSI) + "dBm");

  display.drawString(0, 20,  "Up=" + uptime());
  display.drawString(0, 30,  "T=" + String(temperature,1) + "°C");
  display.drawString(64, 30,  "Hum=" + String(humidity,0) + "%");


  display.drawLine(0, 41, 128, 41);
  display.drawLine(0, 53, 128, 53);
  display.drawLine(0, 63, 128, 63);
  int numcol = 6;
  //int stepx = 128 / numcol;
  for (int i = 0 ; i < numcol ; i++)
  {
    int x = 128 / numcol * i;

    display.drawLine(x, 41, x , 63);
    display.drawString(x + 3, 42 ,  String(tab_Relay_Pin[i]));
    if (i < NUMBEROFRELAY)
    {
      uint8_t RelayStatus;
      RelayStatus = digitalRead(tab_Relay_Pin[i]);
      //Serial.println("Pin" + String(i) + "=" + RelayStatus);
      //Serial.println(dataString );
      if (RelayStatus == 1)
        display.drawString(x + 3, 52 ,  "Off");
      else
        display.drawString(x + 3, 52 ,  "On");
    }
    else
      display.drawString(x + 3, 52 ,  "-");
  }
  display.drawLine(127, 41, 127 , 63);
  display.display();
}

void readDHT22()
{
  // start working...
  //  Serial.println("=================================");
  //  Serial.println("Sample DHT22...");

  // read without samples.
  // @remark We use read2 to get a float data, such as 10.1*C
  //    if user doesn't care about the accurate data, use read to get a byte data, such as 10*C.

  // int err = SimpleDHTErrSuccess;
  // if ((err = dht22.read2(pinDHT22, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
  //   Serial.print("Read DHT22 failed, err="); Serial.println(err); delay(2000);

  //   display.clear();
  //   display.drawString(0, 0,  ipStr);
  //   display.drawString(0, 10,  "Read DHT22 failed");
  //   display.drawString(0, 20,   "err=" + String(err));
  //   display.display();
  //   return;
  // }

  delay(dht.getMinimumSamplingPeriod());

   humidity = dht.getHumidity();
   temperature = dht.getTemperature();

  Serial.print("DHT22 status:");

  Serial.print(dht.getStatusString());
  Serial.print(" Humidity=");
  Serial.print(humidity, 1);
  Serial.print(" Temp=");
  Serial.print(temperature, 1);
  Serial.println("°C");
  // Serial.print("\t\t");
  // Serial.print(dht.toFahrenheit(temperature), 1);
  // Serial.print("\t\t");
  // Serial.print(dht.computeHeatIndex(temperature, humidity, false), 1);
  // Serial.print("\t\t");
  // Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true), 1);

  //  Serial.print("Sample OK: ");
  //  Serial.print((float)temperature); Serial.print(" *C, ");
  //  Serial.print((float)humidity); Serial.println(" RH%");



  // DHT22 sampling rate is 0.5HZ.
  //delay(2500);
}

void setup(void) {

  EEPROM.begin(128);// Initialising the UI will init the display too.

  dht.setup(4, DHTesp::DHT22); // Connect DHT sensor to GPIO 04 : pin D2
  
  display.init();

  // clear the display
  display.clear();

  display.flipScreenVertically();


  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Starting");
  display.display();


  //pinMode(LEDPIN, OUTPUT);
  //digitalWrite(LEDPIN, 0);

  for (int i = 0 ; i < NUMBEROFRELAY ; i++)
  {
    digitalWrite(tab_Relay_Pin[i], 1);
    pinMode(tab_Relay_Pin[i], OUTPUT);
  }

  Serial.begin(115200);

  display.drawString(0, 10, "try to connect to wifi :");
  display.drawString(0, 20, String(ssid));
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();

    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    //wifiManager.autoConnect("AutoConnectAP");
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();


    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    //delay(2000);;
#if 1
    display.clear();
    display.drawString(0, 0,  "connected...yeey :)");
    
  IPAddress ip = WiFi.localIP();
  ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  display.drawString(0, 20,  ipStr);
  display.display();

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  int32_t RSSI = WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.println(String(RSSI));



  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }


  server.on("/", handleRoot);

  


  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  //here the list of headers to be recorded
  const char * headerkeys[] = {"User-Agent", "Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
  //ask server to track these headers
  server.collectHeaders(headerkeys, headerkeyssize );


  server.begin();
  Serial.println("HTTP server started");

  for (int i = 0 ; i < NUMBEROFRELAY ; i++)
  {
    Serial.print("Relay_Pin["); Serial.print(i) ; Serial.print("]= ") ; Serial.println(tab_Relay_Pin[i]) ;
  }


  //send first page
  server.send( 200, "text/html", buildHTMLPage() );
  GetTimeFromServer();
#endif
  Serial.println("OTA Init!");
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\n OTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  //in dev phase OTA always on
  ArduinoOTA.begin();                                               //#
  Serial.println("OTA pret!");
}

String uptime()
{
  long currentmillis = millis();
  int days = 0;
  int hours = 0;
  int mins = 0;
  int secs = 0;
  secs = currentmillis / 1000; //convect milliseconds to seconds
  mins = secs / 60; //convert seconds to minutes
  hours = mins / 60; //convert minutes to hours
  days = hours / 24; //convert hours to days
  secs = secs - (mins * 60); //subtract the coverted seconds to minutes in order to display 59 secs max
  mins = mins - (hours * 60); //subtract the coverted minutes to hours in order to display 59 minutes max
  hours = hours - (days * 24); //subtract the coverted hours to days in order to display 23 hours max
  //Display results
  // Serial.println("Running Time");
  // Serial.println("------------");
  char dataString[50] = {0};
  sprintf(dataString, "%03dd:%02dh:%02dmn:%02ds", days, hours, mins, secs);
  String UpTimeString = dataString;
  UpTimeVar = UpTimeString;
  return UpTimeString;
  //   if (days>0) // days will displayed only if value is greater than zero
  // {
  //   Serial.print(days);
  //   Serial.print(" days and :");
  // }
  // Serial.print(hours);
  // Serial.print(":");
  // Serial.print(mins);
  // Serial.print(":");
  // Serial.println(secs);
  //Serial.println(UpTimeVar);

}

//bool findDateAndTimeInResponseHeaders() {
//// date and time string starts with Date: and ends with GMT
//// example: Date: Sun, 29 May 2016 10:00:14 GMT
//client.setTimeout(HTTP_TIMEOUT);
//char header[85];
//size_t length = client.readBytes(header, 85);
//header[length] = 0;
//String headerString = String(header);
//int ds = headerString.indexOf("Date: ");
//int de = headerString.indexOf("GMT");
//dateAndTime = headerString.substring(ds+6, de);
//// date and time: Sun, 29 May 2016 10:00:14
//if( debug ) {
//Serial.print("HTTP response header ");
//Serial.println(headerString.c_str());
//Serial.print("index start date ");
//Serial.println(ds);
//Serial.print("index end time ");
//Serial.println(de);
//Serial.println(  );
//Serial.print("date and time: ");
//Serial.println(dateAndTime.c_str());
//}
//return dateAndTime.length();
//}

void GetTimeFromServer()
{
if((WiFi.status() == WL_CONNECTED)) {
  if (bIPDomoticz_is_OK)
  {
        HTTPClient http;

        Serial.print("[HTTP] begin...\n");
        // configure traged server and url
        //see list of json action here : https://www.domoticz.com/wiki/Domoticz_API/JSON_URL%27s#Temperature.2Fhumidity
        //domoticz example http://192.168.1.2:8080/json.htm?type=command&param=udevice&idx=$idx&nvalue=0&svalue=79
        //Get Server Time : /json.htm?type=command&param=getSunRiseSet

        String url = "http://" + ipDomoticzStr + ":8080/json.htm?type=command&param=getSunRiseSet";
        http.begin(url);
        //http.begin("http://192.168.0.100:8080/json.htm?type=command&param=getSunRiseSet"); //HTTP

        Serial.print("[HTTP] GET time from domoticz server : " + url + "\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                Serial.println(payload);

                // try fo find ServerTime
                String stringTofind = "\"ServerTime\" :";
                int ind1 = payload.indexOf(stringTofind);  
                int ind2 = payload.indexOf(",",ind1);  
                String ServerTime = payload.substring(ind1 + stringTofind.length() + 1, ind2);  
                Serial.println("ServerTime =" + ServerTime);
                // try fo find year
                ind1 = ServerTime.indexOf("\"");
                ind2 = ServerTime.indexOf("-");
                Year = ServerTime.substring(ind1 + 1 , ind2).toInt();
                //Serial.println("Year =" + ServerTime.substring(ind1 + 1 , ind2));
                Serial.println("Year =" + String(Year));
                ind1= ind2;
                // try fo find month
                ind2 = ServerTime.indexOf("-",ind1 + 1);
                Month = ServerTime.substring(ind1 + 1 , ind2).toInt();
                Serial.println("Month =" + String(Month));
                // try fo find day
                ind1= ind2;
                ind2 = ServerTime.indexOf(" ",ind1 + 1);
                Day = ServerTime.substring(ind1 + 1 , ind2).toInt();
                Serial.println("Day =" + String(Day));
                // try fo find hour
                ind1= ind2;
                ind2 = ServerTime.indexOf(":",ind1 + 1);
                Hour = ServerTime.substring(ind1 + 1 , ind2).toInt();
                Serial.println("Hour =" + String(Hour));
                // try fo find Minute
                ind1= ind2;
                ind2 = ServerTime.indexOf(":",ind1 + 1);
                Minute = ServerTime.substring(ind1 + 1 , ind2).toInt();
                Serial.println("Minute =" + String(Minute));
                // try fo find seconde
                ind1= ind2;
                ind2 = ServerTime.indexOf("\"",ind1 + 1);
                Seconde = ServerTime.substring(ind1 + 1 , ind2).toInt();
                Serial.println("Seconde =" + String(Seconde));

                //Set system time with retrieve value
                setTime(Hour,Minute,Seconde,Day,Month,Year);

                //setTime(pctime);

            }
        } else {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }
  }
  else
  {
    Serial.printf("DomoticZ IP seems wrong..... \n");
  }


}
void Calculatetime()
{
  #if 0
    // put your main code here, to run repeatedly:


timeNow = millis()/1000; // the number of milliseconds that have passed since boot
seconds = timeNow - timeLast;//the number of seconds that have passed since the last time 60 seconds was reached.



if (seconds == 60) {
  timeLast = timeNow;
  minutes = minutes + 1;
}

//if one minute has passed, start counting milliseconds from zero again and add one minute to the clock.

if (minutes == 60){
  minutes = 0;
  hours = hours + 1;
}

// if one hour has passed, start counting minutes from zero and add one hour to the clock

if (hours == 24){
  hours = 0;
  days = days + 1;
  }

  //if 24 hours have passed , add one day

//if (hours ==(24 - startingHour) && correctedToday == 0){
//  delay(dailyErrorFast*1000);
//  seconds = seconds + dailyErrorBehind;
//  correctedToday = 1;
//}

//every time 24 hours have passed since the initial starting time and it has not been reset this day before, add milliseconds or delay the progran with some milliseconds.
//Change these varialbes according to the error of your board.
// The only way to find out how far off your boards internal clock is, is by uploading this sketch at exactly the same time as the real time, letting it run for a few days
// and then determine how many seconds slow/fast your boards internal clock is on a daily average. (24 hours).

//if (hours == 24 - startingHour + 2) {
//  correctedToday = 0;
//}
#endif
  }




