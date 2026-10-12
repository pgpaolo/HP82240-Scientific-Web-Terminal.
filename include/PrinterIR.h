#pragma once
#include <Arduino.h>
class PrinterIR {
  uint8_t pin_;
  uint32_t sent_=0;
  void burst(){for(uint8_t i=0;i<7;++i){digitalWrite(pin_,HIGH);delayMicroseconds(15);digitalWrite(pin_,LOW);delayMicroseconds(15);}}
  void half(bool mark){if(mark){burst();delayMicroseconds(217);}else delayMicroseconds(427);}
  void bit(bool v){half(v);half(!v);}
  void byte(uint8_t v){
    // Experimental HP Redeye framing: requires logic analyzer validation.
    noInterrupts();
    half(true);half(true);half(true);
    const uint8_t masks[]={0x78,0xE6,0xD5,0x8B};
    for(uint8_t mask:masks)bit(__builtin_parity((unsigned)(v&mask)));
    for(int i=7;i>=0;--i)bit((v>>i)&1);
    half(false);half(false);half(false);
    interrupts();++sent_;yield();
  }
public:
  explicit PrinterIR(uint8_t pin):pin_(pin){}
  void begin(){pinMode(pin_,OUTPUT);digitalWrite(pin_,LOW);}
  uint32_t sent()const{return sent_;}
  void sendText(const String& text){
    for(size_t i=0;i<text.length();++i){
      uint8_t c=(uint8_t)text[i];
      if(c=='\r')continue;
      if(c=='\n'||(c>=32&&c<=126))byte(c);
      else if(c>=128){byte('?');while(i+1<text.length()&&(((uint8_t)text[i+1]&0xC0)==0x80))++i;}
    }
  }
};
