#include <sw/redis++/redis++.h>

#include <any>
#include <cstdio>
#include <iostream>
#include <memory>
#include "myrttr/property.h"
#include "myrttr/registration"
#include "myrttr/registration_friend"
#include "myrttr/type"
#include "myrttr/instance.h"
#include "myrttr/type.h"
#include "myrttr/variant.h"
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

// clang-format false
// RTTR_REGISTRATION
// {
//   // serialization
//   rttr::registration::class_<io::IdHolder>("IdHolder").property("id", &io::IdHolder::id);
//   // test
//   registration::class_<SecondClass>("SecondClass")
//       .constructor<>()
//       .property("secname", &SecondClass::name)
//       .property("y", &SecondClass::y)
//       .property("bottom_map", &SecondClass::bottom_map)
//       .property("bottom", &SecondClass::bottom);
//   registration::class_<TopClass>("TopClass")
//       .constructor<>()
//       .property("topname", &TopClass::name)
//       .property("x", &TopClass::x)
//       .property("secondP", &TopClass::second)
//       .property("secplist", &TopClass::secplist)
//       .method("set_second", &TopClass::set_second);
//   registration::class_<BottomClass>("BottomClass").constructor<>().property("btmname", &BottomClass::name);
//   // .property("secname", &BottomClass::second);
// }
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
  auto topid = io::to_json(top.second);
  if (topid == "0") {
    cout << "to_json error \n";
    return 1;
  }
  // cout << topid << endl;
  TopClass cli_top(99);
  SecondClass cli_sec;
  io::from_key(topid, cli_sec);
  auto* bot_base = dynamic_cast<BottomClass*>(cli_sec.bases[1]);
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

int main()
{
  TopClass top(99);
  SecondClass second;
  BottomClass bottom;
  Base base;
  top.top.emplace_back();
  top.tplt = TpltClass<int32_t>();
  bottom.name = "changed";
  bottom.bx = 66;
  second.bottom = &bottom;
  second.bottom_map["btm1"] = &bottom;
  top.set_second(&second);
  top.secplist.push_back(&second);
  top.secplist.push_back(&second);
  second.bases.push_back(&base);
  second.bases.push_back(&bottom);
  // bottom.second = &second;
  // test_clang(top);
  test_json(top);
  // test_base(second);
  return 0;
}