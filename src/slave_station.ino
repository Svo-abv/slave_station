//String location = "Второй этаж, спальня";
String location= "Первый этаж, кухня";
//---------------------------------------Firmware mode-----------------------------------------------//
#include "init.h"

#define DEBUG(x) if(dev==1) (x)
#define DEBUG_MSG(x) if(dev==1) Serial.print(x)
#define DEBUG_MSGLN(x) if(dev==1) Serial.println(x)

ADC_MODE(ADC_VCC);

//---------------------------------------Wi-Fi------------------------------------------------------//

#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#define LED 2

int status = WL_IDLE_STATUS;

WiFiUDP ntpUDP;

//---------------------------------------EPPROM------------------------------------------------------//

#include <EEPROM.h>

//---------------------------------------Time------------------------------------------------------//

#include <NTPClient.h>

const long utcOffsetInSeconds = 18000;

NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//---------------------------------------WEB Server-------------------------------------------------//

#include <ESP8266WebServer.h>
#define SERVER_PORT      80         // Порт для входа, он стандартный 80 это порт http

ESP8266WebServer HttpServer(SERVER_PORT);

//---------------------------------------OTA Update-------------------------------------------------//
#include <ArduinoOTA.h>
#include <ESP8266HTTPUpdateServer.h>

ESP8266HTTPUpdateServer httpUpdater;


//-----------------------------------JSON----------------------------------------------------------//

#include <Arduino_JSON.h>


//------------------------------------Ticker-----------------------------------------------------//

int lastExec = 0;


//------------------------------------DTH Sensor--------------------------------------------------//

#include "DHT.h"

#define DHT_PIN 13
DHT dht1(DHT_PIN, DHT22);


void handleNotFound() {
  HttpServer.send(404, "text/plain", "404: Not found");
}

void handleReboot()
{
  String result = "Succeed! Now rebooting...";
  HttpServer.send(200, "text/html", result);
  delay(1000);
  ESP.restart();
}

void handleGetMainStatus(){
  JSONVar json;
  byte ar[6];
  WiFi.macAddress(ar);
  char macAddr[18];
  sprintf(macAddr, "%2X:%2X:%2X:%2X:%2X:%2X", ar[0], ar[1], ar[2], ar[3], ar[4], ar[5]);

  json["text"] = "in progress...";
  json["IP"] = WiFi.localIP().toString();
  json["MAC"] = macAddr;
  json["dev_mode"] = dev;
  json["SDK"] = ESP.getSdkVersion();
  json["FlashChipSpeed"] = String(ESP.getFlashChipSpeed());
  json["FlashChipSize"] = String(ESP.getFlashChipSize());
  json["FreeHeap"] = String(ESP.getFreeHeap());
  json["Vcc"] = ((float)ESP.getVcc()) / 1000;
  json["Last_cycle_time"] = lastExec;
  json["location"] = location;
  timeClient.update();
  json["Time"] = timeClient.getFormattedTime();
  HttpServer.send(200, "application/json", JSON.stringify(json));
  delay(1000);
}

void handleGetData() {

  digitalWrite(LED, HIGH);
  unsigned int msecLast = millis();
 
  JSONVar json;

  float h = dht1.readHumidity();
  float t = dht1.readTemperature();
  float i = dht1.computeHeatIndex(h, t, false);

  json["temperature"] = t;
  json["humidity"] = h;
  json["heatIndex"] = i;
  HttpServer.send(200, "application/json", JSON.stringify(json));
  
  unsigned int nowMsec = millis();
  lastExec = (nowMsec - msecLast) / 1000;
  digitalWrite(LED, LOW);
}

void loop() {

  HttpServer.handleClient();

}

void setup() {


  pinMode(LED, OUTPUT);

  Serial.begin(115200);
  delay(1000);

  DEBUG(Serial.setDebugOutput(true));
  DEBUG_MSGLN("Start init device...");
  DEBUG_MSGLN("Wi-Fi...");


  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);

  if (WiFi.status() == WL_CONNECTED){
    DEBUG_MSGLN("Connected");

  }else{
    DEBUG_MSGLN("Connect error!");
  }

  httpUpdater.setup(&HttpServer, OTA_PATH, OTA_USER, OTA_PASSWORD);
  
  HttpServer.on("/", handleGetMainStatus);
  HttpServer.on("/reboot", handleReboot);
  HttpServer.on("/data", handleGetData);
  HttpServer.onNotFound(handleNotFound);

  HttpServer.begin();
  dht1.begin();
  timeClient.begin();
}
