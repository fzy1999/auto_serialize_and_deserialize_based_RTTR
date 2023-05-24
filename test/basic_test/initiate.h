#pragma once
#include "myrttr/registration_friend"
#include "myrttr/rttr_enable.h"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#define RTTR_REFLECT(...)
using std::string;
class SecondClass;
RTTR_REFLECT(WithNonPublic)
class Base
{
  RTTR_ENABLE()
 public:
  int bx = 1;
};
RTTR_REFLECT(WithNonPublic)
class BottomClass : public Base
{
  RTTR_ENABLE(Base)
 public:
  string name = "Buttom";
  SecondClass* second;
};

RTTR_REFLECT(WithNonPublic)
class SecondClass
{
 public:
  BottomClass* bottom;
  string name = "Second";
  int32_t y = 88;
  std::unordered_map<string, BottomClass*> bottom_map;
  std::vector<Base*> bases;
  std::optional<BottomClass*> opt_bot;
  std::optional<int> opt_int;
  BottomClass bot_inst;
  // RTTR_ENABLE()
  RTTR_REGISTRATION_FRIEND
};

template <typename T>
class TpltClass
{
 public:
  RTTR_REGISTRATION_FRIEND
  T num;
};

RTTR_REFLECT(WithNonPublic)
class TopClass
{
 public:
  TopClass() = default;
  TopClass(int _x) : x(_x){};
  class Top
  {
    RTTR_REGISTRATION_FRIEND
    int x = 998;
  };
  void set_second(SecondClass* _second) { second = _second; }
  std::vector<SecondClass*> secplist;
  SecondClass* second;
  std::vector<Top> top;
  TpltClass<int32_t> tplt;

 private:
  string name = "top";
  int32_t x = 99;
  RTTR_REGISTRATION_FRIEND
};