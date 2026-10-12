#pragma once
#include <Arduino.h>
class PrintQueue {
  static constexpr uint8_t CAPACITY=4;
  String items_[CAPACITY];
  uint8_t head_=0,tail_=0,count_=0;
public:
  bool enqueue(const String& text){if(count_==CAPACITY)return false;items_[tail_]=text;tail_=(tail_+1)%CAPACITY;++count_;return true;}
  String dequeue(){if(!count_)return "";String out=items_[head_];items_[head_]="";head_=(head_+1)%CAPACITY;--count_;return out;}
  uint8_t count()const{return count_;}
};
