#include <sw/redis++/redis++.h>

#include <any>
#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "c2redis/src/json_serialization/batch_from.h"
#include "c2redis/test/basic_test/initiate.h"
#include "common.h"
#include "from_to_redis.h"
#include "myrttr/argument.h"
#include "myrttr/instance.h"
#include "myrttr/property.h"
#include "myrttr/registration"
#include "myrttr/registration_friend"
#include "myrttr/string_view.h"
#include "myrttr/type"
#include "myrttr/type.h"
#include "myrttr/variant.h"
// #include "initiate.h"
#include "generated/registor.h"
#include "rapidjson/document.h"

using namespace rttr;

using namespace sw::redis;
using namespace rapidjson;
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
  io::from_key(*cid, cli_sec);
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

int test_batch_to(TopClass& top)
{
  // io::to_json(top.second);
  c2redis::ToRedis to_redis;
  auto cid = to_redis(top);
  return 0;
}

int test_option2(SecondClass& second)
{
  StringBuffer sb;
  PrettyWriter<StringBuffer> writer(sb);
  writer.StartObject();
  string name("test");
  writer.String(name.data(), static_cast<rapidjson::SizeType>(name.length()), false);
  instance inst(second);
  auto var = inst.get_type().get_property("opt_bot").get_value(inst);
  // writer.Int(var.get_value<int32_t>());
  variant varcl(var);
  auto xx = varcl.get_value<BottomClass>();
  writer.EndObject();
  cout << sb.GetString() << endl;

  return 0;
}

int test_shared(TopClass& top)
{
  instance inst(top.second);
  auto var = inst.get_type().get_property("bases");
  auto vt = var.get_type().get_raw_type().get_wrapped_type();
  auto secv = var.get_value(inst);
  auto view = secv.create_sequential_view();
  auto c = view.get_size();
  auto sp = std::make_shared<Base>();
  instance spinst(sp);
  for (const auto& v : view) {
    variant wrapped_var = v.extract_wrapped_value();
    instance vinst(wrapped_var);
    void* key;
    if (vinst.get_derived_type().get_raw_type().is_wrapper()) {
      key = vinst.get_wrapped_instance().get_object_pointer();
    } else {
      key = vinst.get_object_pointer();
    }
    auto xx = vinst.get_wrapped_instance().get_derived_type();
    auto wr = vinst.get_type().get_raw_type().is_wrapper() ? vinst.get_wrapped_instance() : vinst;
    auto task = std::make_shared<c2redis::Task>(vinst);
    auto x = 0;
  }
  return 1;
}
variant test_rv()
{
  return {};
}
int test_variant(BottomClass& bottom)
{
  variant var = test_rv();
  auto x = var.is_valid();
  variant var_valid(bottom);
  auto y = var_valid.is_valid();
  instance inst1(bottom);
  instance inst2(inst1);
  auto xx = inst2.get_type().get_property("name").get_value(inst2).to_string();
  return 0;
}

int test_from_batch(TopClass& top)
{
  // c2redis::ToRedis to_redis;
  // auto cid = to_redis(top);
  c2redis::FromRedis from_redis;
  TopClass top_new;
  from_redis(top_new, "0x7fffffffe250");
  // io::from_key("0x7fffffffe180", new_sec);
  return 0;
}

int test_json()
{
  Value out;
  {
    const char* json = R"({
                "cid": "0x7fffffffe138",
                "type": "BottomClass",
                "oj": {
                  "cid": "0x7fffffffe138",
                  "type": "BottomClass"
                }
            })";

    // Parse JSON data
    Document document;
    document.Parse(json);

    // Traverse JSON object
    if (document.IsObject()) {
      for (Value::MemberIterator member = document.MemberBegin(); member != document.MemberEnd();
           ++member) {
        if (member->value.IsObject()) {
          auto& val = member->value;
          val.Swap(out);
        } else {
          std::string key = member->value.GetString();  // Get key/name
          std::cout << "v: " << key << std::endl;
        }
      }
    }
  }
  for (Value::MemberIterator member = out.MemberBegin(); member != out.MemberEnd(); ++member) {
    std::string key = member->value.GetString();  // Get key/name
    std::cout << "outv: " << key << std::endl;
  }

  return 0;
}

int test_move(SecondClass& sec)
{
  SecondClass sec_new;
  instance inst(sec_new);

  instance inst_rep(inst);
  auto bot = type::get_by_name("BottomClass").create();
  instance bot_inst(bot);
  auto sec_t = c2redis::get_wrapped(bot_inst.get_type().get_raw_type());
  auto sec_prop = sec_t.get_property("second");

  auto ok1 = sec_prop.set_value_raw_ptr(bot_inst, inst.get_object_pointer());

  auto bot_prop = inst.get_type().get_property("bottom");
  auto valid = bot_prop.get_type().get_name().to_string();
  auto ext = bot.extract_wrapped_value().get_type();

  auto ok2 = bot_prop.set_value(inst_rep, bot.extract_wrapped_value());
  auto x = 1;
  return 0;
}

template <typename C, typename A>
class TPoint
{
};

template <typename C, typename A>
class TPoint<C, A(C::*)>
{
 public:
  A x;
  bool set_ptr(void* p)
  {
    if constexpr (std::is_pointer<A>::value) {
      x = reinterpret_cast<A>(p);
      return true;
    } else {
      return false;
    }
  }
};

int test_argument(SecondClass& sec)
{
  int x = 99;
  int* xp = &x;
  auto vxp = reinterpret_cast<void*>(xp);
  TPoint<SecondClass, decltype(&SecondClass::y)> tint;
  tint.x = 1;
  auto ret = tint.set_ptr(vxp);

  TPoint<SecondClass, decltype(&SecondClass::bottom)> tbot;
  auto ret2 = tbot.set_ptr(vxp);
  return 0;
}

int test_shared()
{
  auto sh = std::make_shared<int>(88);
  *sh = 99;
  return 0;
}

int main()
{
  TopClass top(99);
  SecondClass second;
  SecondClass secnull;
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
  top.secplist.push_back(&secnull);
  second.bases.push_back(std::make_shared<Base>());
  second.bases.push_back(std::make_shared<BottomClass>());
  second.opt_bot = std::make_shared<BottomClass>();
  second.opt_int = 996;
  second.bot_inst.name = "ccchanged";
  second.bot_inst.second = nullptr;
  second.bottom->second = &second;
  // bottom.second = &second;
  // test_clang(top);
  // test_json(top);
  // test_base(second);
  // test_optional(second);
  // test_batch_to(top);
  // test_shared(top);
  // test_option2(second);
  // test_variant(bottom);
  // test_json();
  test_from_batch(top);
  // test_move(second);
  // test_argument(second);
  // test_shared();
  return 0;
}