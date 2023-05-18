#include <sw/redis++/redis++.h>

#include <any>
#include <cstdio>
#include <iostream>
#include <memory>
#include "myrttr/property.h"
#include "myrttr/registration"
#include "myrttr/registration_friend"
#include "myrttr/string_view.h"
#include "myrttr/type"
#include "myrttr/instance.h"
#include "myrttr/type.h"
#include "myrttr/variant.h"
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "to_json.h"
#include "from_json.h"
#include "initiate.h"
using namespace rttr;

using namespace sw::redis;
using std::cout;
using std::endl;
using std::string;

///////////////////////////

template <typename T>
struct is_optional : std::false_type
{
};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type
{
};

#include "generated/registor.h"

//clang-format on

int test_clang(TopClass& top)
{
  instance inst(top);

  auto tplt = inst.get_type().get_property("tplt").get_type();
  auto tpltname = tplt.get_property("num").get_type();

  auto toplist = inst.get_type().get_property("top").get_type();
  auto type = inst.get_type();
  return 0;
}

int test_json(TopClass& top)
{
  auto cid = io::to_json(top.second);
  if (cid == "0") {
    cout << "to_json error \n";
    return 1;
  }
  // cout << topid << endl;
  TopClass cli_top(99);
  SecondClass cli_sec;
  io::from_key(cid, cli_sec);
  return 0;
}

int test_base(SecondClass& second)
{
  instance inst(second);
  auto bases = inst.get_derived_type().get_property("bases").get_value(inst);
  auto sect = bases.get_type();
  auto view = bases.create_sequential_view();
  for (const auto& item : view) {
    auto it = item.get_type().get_wrapped_type().get_raw_type();
    auto det = instance(item).get_wrapped_instance().get_derived_type();
    auto vt = item.can_convert(rttr::type::get_by_name("BottomClass"));
    auto x = vt;
  }
  return 0;
}

string get_optional_type(string str)
{
  size_t start_pos = str.find("<");
  size_t end_pos = str.find(">", start_pos);
  return str.substr(start_pos + 1, end_pos - start_pos - 1);
}

int test_optional(SecondClass& second)
{
  instance inst(second);
  auto opt = inst.get_derived_type().get_property("opt_bot");
  auto opt_inst = instance(opt).get_object_pointer();
  auto bot = static_cast<BottomClass*>(opt_inst);

  auto otype_name = get_optional_type(opt.get_type().get_name().to_string());
  auto t = rttr::type::get_by_name(otype_name);
  return 0;
}

int main()
{
  TopClass top(99);
  SecondClass second;
  BottomClass bottom;
  Base base;
  top.top.emplace_back();
  top.tplt = TpltClass<int32_t>{99};
  bottom.name = "changed";
  bottom.bx = 66;
  second.bottom = &bottom;
  second.bottom_map["btm1"] = &bottom;
  top.set_second(&second);
  top.secplist.push_back(&second);
  top.secplist.push_back(&second);
  second.bases.push_back(&base);
  second.bases.push_back(&bottom);
  second.opt_bot = bottom;
  second.opt_int = 996;
  second.bot_inst.name = "ccchanged";
  second.bot_inst.second = nullptr;
  second.bottom->second = &second;
  // bottom.second = &second;
  // test_clang(top);
  test_json(top);
  // test_base(second);
  // test_optional(second);
  return 0;
}