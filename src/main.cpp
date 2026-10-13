#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <Ticker.h>
#include <Hash.h>
#include <bearssl/bearssl.h>
#include "PrinterIR.h"
#include "PrintQueue.h"
#include "ConfigStore.h"

ESP8266WebServer server(80);
ConfigStore config;
PrintQueue jobs;
PrinterIR printer(D2);
bool apMode=false;
String apPassword;
unsigned long bootMs=0;
static const char* AUTH_HEADER="X-API-Key";

String jsonEscape(String s) {
  s.replace("\\","\\\\"); s.replace("\"","\\\"");
  s.replace("\n","\\n"); s.replace("\r","\\r");
  return s;
}
void reply(int code,const String& json){server.sendHeader("Cache-Control","no-store");server.sendHeader("X-Content-Type-Options","nosniff");server.send(code,"application/json; charset=utf-8",json);}
bool auth() {
  String supplied=server.header(AUTH_HEADER);
  const String& expected=config.apiKey;
  if(supplied.length()!=expected.length() || expected.length()!=32){reply(401,"{\"error\":\"unauthorized\"}");return false;}
  uint8_t diff=0;
  for(size_t i=0;i<expected.length();++i)diff|=(uint8_t)(supplied[i]^expected[i]);
  if(diff){reply(401,"{\"error\":\"unauthorized\"}");return false;}
  return true;
}
bool trustedOrigin() {
  // Requests without Origin are allowed only when carrying the API key.
  String origin=server.header("Origin");
  if(origin.isEmpty())return true;
  String host=server.hostHeader();
  if(origin==("http://"+host))return true;
  reply(403,"{\"error\":\"cross-origin denied\"}");return false;
}
bool guard(){return trustedOrigin()&&auth();}
void startAP() {
  apMode=true;
  WiFi.mode(WIFI_AP_STA);
  uint32_t r=ESP.random();
  char p[17];snprintf(p,sizeof(p),"%08lX%08lX",(unsigned long)r,(unsigned long)ESP.random());
  apPassword=p;
  String name="HP82240-"+String(ESP.getChipId(),HEX);
  WiFi.softAP(name.c_str(),apPassword.c_str());
  Serial.printf("SETUP SSID: %s\nSETUP PASSWORD: %s\n",name.c_str(),apPassword.c_str());
}
void connectNetwork(){
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  if(config.ssid.length()){
    WiFi.begin(config.ssid.c_str(),config.password.c_str());
    unsigned long t=millis();
    while(WiFi.status()!=WL_CONNECTED && millis()-t<12000){delay(100);}
  }
  if(WiFi.status()!=WL_CONNECTED)startAP();
}
void setupRoutes(){
  server.on("/",HTTP_GET,[](){
    File f=LittleFS.open("/index.html","r");
    if(!f){server.send(500,"text/plain","Upload filesystem");return;}
    server.sendHeader("Cache-Control","no-store");
    server.streamFile(f,"text/html; charset=utf-8");f.close();
  });
  server.on("/api/status",HTTP_GET,[](){
    if(!guard())return;
    reply(200,"{\"connected\":"+String(WiFi.status()==WL_CONNECTED?"true":"false")
      +",\"ip\":\""+WiFi.localIP().toString()+"\",\"ap_mode\":"+String(apMode?"true":"false")
      +",\"ssid\":\""+jsonEscape(config.ssid)+"\",\"queued\":"+String(jobs.count())
      +",\"sent_bytes\":"+String(printer.sent())+",\"free_heap\":"+String(ESP.getFreeHeap())+"}");
  });
  server.on("/api/wifi/scan",HTTP_GET,[](){
    if(!guard())return;
    int n=WiFi.scanNetworks(false,true);
    String s="[";
    for(int i=0;i<n && i<30;++i){
      if(i)s+=",";
      s+="{\"ssid\":\""+jsonEscape(WiFi.SSID(i))+"\",\"rssi\":"+String(WiFi.RSSI(i))
        +",\"secure\":"+String(WiFi.encryptionType(i)==ENC_TYPE_NONE?"false":"true")+"}";
    }
    WiFi.scanDelete();reply(200,s+"]");
  });
  server.on("/api/wifi",HTTP_POST,[](){
    if(!guard())return;
    String s=server.arg("ssid"),p=server.arg("password");
    if(s.isEmpty()||s.length()>32||p.length()>63){reply(400,"{\"error\":\"invalid Wi-Fi credentials\"}");return;}
    if(!config.saveWifi(s,p)){reply(500,"{\"error\":\"save failed\"}");return;}
    reply(200,"{\"saved\":true,\"restart\":true}");delay(400);ESP.restart();
  });
  server.on("/api/print",HTTP_POST,[](){
    if(!guard())return;
    String t=server.arg("plain");
    if(t.isEmpty()&&server.hasArg("text"))t=server.arg("text");
    if(t.isEmpty()||t.length()>1024){reply(400,"{\"error\":\"text must be 1..1024 bytes\"}");return;}
    if(!jobs.enqueue(t)){reply(429,"{\"error\":\"queue full\"}");return;}
    reply(202,"{\"queued\":true,\"verified_on_printer\":false,\"queue_depth\":"+String(jobs.count())+"}");
  });
  server.on("/api/ir/test",HTTP_POST,[](){
    if(!guard())return;
    if(!jobs.enqueue("HP 82240 TEST\n1234567890\n")){reply(429,"{\"error\":\"queue full\"}");return;}
    reply(202,"{\"queued\":true,\"verified_on_printer\":false}");
  });
  server.onNotFound([](){reply(404,"{\"error\":\"not found\"}");});
}
void setup(){
  Serial.begin(115200);delay(150);
  LittleFS.begin();
  config.begin();
  Serial.println("\nHP 82240 terminal (experimental IR)");
  Serial.println("API KEY (keep private): "+config.apiKey);
  printer.begin();
  connectNetwork();
  server.collectHeaders("X-API-Key", "Origin");
  setupRoutes();server.begin();
  Serial.println("STA IP: "+WiFi.localIP().toString());
  bootMs=millis();
}
void loop(){
  server.handleClient();
  // Keep HTTP responsive between jobs; IR sender remains experimental.
  if(jobs.count()){
    String job=jobs.dequeue();
    printer.sendText(job);
  }
  delay(1);
}
