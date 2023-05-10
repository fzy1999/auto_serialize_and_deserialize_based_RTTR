#pragma once
#include "myrttr/registration"
#include "myrttr/registration_friend"
#include <string>
#include <unordered_map>
#include <vector>
using std::string;
class SecondClass;
class BottomClass
{
 public:
  string name = "Buttom";
  // SecondClass* second;
};

class SecondClass
{
 public:
  BottomClass* bottom;
  string name = "Second";
  int32_t y = 88;
  std::unordered_map<string, BottomClass*> bottom_map;
  // RTTR_ENABLE()
  RTTR_REGISTRATION_FRIEND
};

class TopClass
{
 public:
  void set_second(SecondClass* _second) { second = _second; }
  std::vector<SecondClass*> secplist;
  SecondClass* second;

 private:
  string name = "top";
  int32_t x = 99;

  RTTR_REGISTRATION_FRIEND
};