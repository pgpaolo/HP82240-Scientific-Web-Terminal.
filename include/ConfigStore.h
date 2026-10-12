#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
class ConfigStore {
public:
  String ssid,password,apiKey;
  void begin(){
    File f=LittleFS.open("/wifi.txt","r");
    if(f){ssid=f.readStringUntil('\n');ssid.trim();password=f.readStringUntil('\n');password.trim();f.close();}
    f=LittleFS.open("/api.key","r");
    if(f){apiKey=f.readStringUntil('\n');apiKey.trim();f.close();}
    if(apiKey.length()!=32){
      char b[33];
      snprintf(b,sizeof(b),"%08lX%08lX%08lX%08lX",(unsigned long)ESP.random(),(unsigned long)ESP.random(),(unsigned long)ESP.random(),(unsigned long)ESP.random());
      apiKey=b;
      f=LittleFS.open("/api.key","w");if(f){f.println(apiKey);f.close();}
    }
  }
  bool saveWifi(const String& s,const String& p){
    File f=LittleFS.open("/wifi.txt","w");if(!f)return false;
    f.println(s);f.println(p);f.close();ssid=s;password=p;return true;
  }
};
