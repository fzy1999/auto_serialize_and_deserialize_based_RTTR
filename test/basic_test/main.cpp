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
RTTR_REGISTRATION
{
  // serialization
  rttr::registration::class_<io::IdHolder>("IdHolder").property("id", &io::IdHolder::id);
  // test
  registration::class_<SecondClass>("SecondClass")
      .constructor<>()
      .property("secname", &SecondClass::name)
      .property("y", &SecondClass::y)
      .property("bottom_map", &SecondClass::bottom_map)
      .property("bottom", &SecondClass::bottom);
  registration::class_<TopClass>("TopClass")
      .constructor<>()
      .property("topname", &TopClass::name)
      .property("x", &TopClass::x)
      .property("secondP", &TopClass::second)
      .property("secplist", &TopClass::secplist)
      .method("set_second", &TopClass::set_second);
  registration::class_<BottomClass>("BottomClass").constructor<>().property("btmname", &BottomClass::name);
  // .property("secname", &BottomClass::second);
}

//clang-format on

///////////////////////
std::unordered_map<string, std::any> CLASSMAP;  // for store all pointer class in classes
void init_class_map()
{
  CLASSMAP["SecondClass"] = SecondClass();
}

template <typename T>
void recursion(T& attr);
template <>
void recursion(SecondClass& attr);
template <>
void recursion(TopClass& attr);
/**
 * @brief pname必须是rttr注册的类下的属性名, pclass是实际的c++的类型
 *
 */
#define DESCEND(pname, pclass)                                                               \
  do {                                                                                       \
    auto child = t.get_property(pname).get_value(attr).template get_wrapped_value<pclass>(); \
    recursion(child);                                                                        \
  } while (0)

void save_redis(string val)
{
  std::cout << "->redis: " << val << " \n";
}

void save_normal(type& t)
{
  for (auto& prop : t.get_properties()) {
    if (prop.get_type().is_pointer()) {
      std::cout << "point wait: " << prop.get_type().get_name() << "\n";
    } else {
      save_redis(prop.get_name().to_string());
    }
  }
}

int test_clang(TopClass& top)
{
  instance inst(top);
  auto prop = inst.get_type().get_property("topname");
  bool ok = true;
  auto val = prop.get_value(inst).to_string(&ok);
  auto type = prop.get_type();
  return 0;
}

int test_json(TopClass& top)
{
  auto topid = io::to_json(top);
  if (topid == "0") {
    cout << "to_json error \n";
    return 1;
  }
  // cout << topid << endl;
  TopClass cli_top;
  SecondClass cli_sec;
  io::from_key(topid, cli_top);
  // auto seclp = cli_top.secplist[0]->name;
  return 0;
}

int main()
{
  TopClass top;
  SecondClass second;
  BottomClass bottom;
  bottom.name = "changed";
  second.bottom = &bottom;
  second.bottom_map["btm1"] = &bottom;
  top.set_second(&second);
  top.secplist.push_back(&second);
  top.secplist.push_back(&second);
  // bottom.second = &second;
  // test_clang(top);
  test_json(top);
  return 0;
}