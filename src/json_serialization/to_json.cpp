#include "common.h"
#include "myrttr/instance.h"
#include "myrttr/variant.h"
#include "myrttr/type"
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>      // rapidjson's DOM-style API
#include <rapidjson/prettywriter.h>  // for stringify JSON

#include "../redis_helper/redis_helper.h"
#include "to_json.h"
#include <uuid/uuid.h>

using namespace rapidjson;
using namespace rttr;
using c2redis::ID_TYPE;

namespace {
void to_json_recursively(const instance& obj, PrettyWriter<StringBuffer>& writer);

ID_TYPE write_variant(const variant& var, PrettyWriter<StringBuffer>& writer);

/////////////////////////////////////////////////////////////////////////////////////////

inline type get_wrapped_type(const type& type2)
{
  return type2.is_wrapper() ? type2.get_wrapped_type().get_raw_type() : type2.get_raw_type();
}

/////////////////////////////////////////////////////////////////////////////////////////

std::map<const void*, ID_TYPE> g_key_storage;

/////////////////////////////////////////////////////////////////////////////////////////

class GKeyMutex
{
  std::mutex _g_key_mutex;

 public:
  GKeyMutex() { _g_key_mutex.lock(); }
  ~GKeyMutex() { _g_key_mutex.unlock(); }
};

ID_TYPE generate_g_key()
{
  uuid_t uuid;
  uuid_generate_time_safe(uuid);
  char str[37];
  uuid_unparse(uuid, str);
  return std::string(str);
}
bool get_g_key(const instance& inst, ID_TYPE& cid)
{
  if (!inst.is_valid()) {
    return true;
  }
  void* key;
  if (inst.get_derived_type().is_wrapper()) {
    key = inst.get_wrapped_instance().get_object_pointer();
  } else {
    key = inst.get_object_pointer();
  }
  if (key == nullptr) {
    return true;
  }
  GKeyMutex mutex;
  if (!g_key_storage.contains(key)) {
    cid = generate_g_key();
    g_key_storage[key] = cid;
    return false;
  }
  cid = g_key_storage[key];
  // std::cout << inst.get_type().get_name().to_string() << " pointer same : " << key << "\n";
  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief for writer not passing in
 *
 * @param obj
 * @return ID_TYPE
 */
ID_TYPE to_json_recursively(const instance& obj);
/////////////////////////////////////////////////////////////////////////////////////////

void write_IdHolder(const ID_TYPE& cid, const variant& var, PrettyWriter<StringBuffer>& writer)
{
  if (cid == std::nullopt) {
    // null pointer
    c2redis::NullHolder nullholder;
    to_json_recursively(nullholder, writer);
    return;
  }
  auto derive_type = instance(var).get_derived_type();
  if (derive_type.is_wrapper()) {
    derive_type = derive_type.get_wrapped_type();
  }
  if (derive_type.is_pointer()) {
    derive_type = derive_type.get_raw_type();
  }
  c2redis::IdHolder holder{*cid, derive_type.get_name().to_string()};
  to_json_recursively(holder, writer);
}

bool is_optional(const type& t)
{
  return t.get_name().to_string().find("optional") != string::npos;
}

bool write_optinal_types_to_json(const type& t, const variant& var, PrettyWriter<StringBuffer>& writer)
{
  if (is_optional(t)) {
    if (var.is_type<std::optional<bool>>())
      writer.Bool(var.get_value<bool>());
    else if (var.is_type<std::optional<char>>())
      writer.Bool(var.get_value<char>());
    else if (var.is_type<std::optional<int8_t>>())
      writer.Int(var.get_value<int8_t>());
    else if (var.is_type<std::optional<int16_t>>())
      writer.Int(var.get_value<int16_t>());
    else if (var.is_type<std::optional<int32_t>>())
      writer.Int(var.get_value<int32_t>());
    else if (var.is_type<std::optional<int64_t>>())
      writer.Int64(var.get_value<int64_t>());
    else if (var.is_type<std::optional<uint8_t>>())
      writer.Uint(var.get_value<uint8_t>());
    else if (var.is_type<std::optional<uint16_t>>())
      writer.Uint(var.get_value<uint16_t>());
    else if (var.is_type<std::optional<uint32_t>>())
      writer.Uint(var.get_value<uint32_t>());
    else if (var.is_type<std::optional<uint64_t>>())
      writer.Uint64(var.get_value<uint64_t>());
    else if (var.is_type<std::optional<float>>())
      writer.Double(var.get_value<float>());
    else if (var.is_type<std::optional<double>>())
      writer.Double(var.get_value<double>());
    else
      return false;

    return true;
  }
  return false;
}

bool write_atomic_types_to_json(const type& t, const variant& var, PrettyWriter<StringBuffer>& writer)
{
  if (t.is_arithmetic()) {
    if (t == type::get<bool>())
      writer.Bool(var.to_bool());
    else if (t == type::get<char>())
      writer.Bool(var.to_bool());
    else if (t == type::get<int8_t>())
      writer.Int(var.to_int8());
    else if (t == type::get<int16_t>())
      writer.Int(var.to_int16());
    else if (t == type::get<int32_t>())
      writer.Int(var.to_int32());
    else if (t == type::get<int64_t>())
      writer.Int64(var.to_int64());
    else if (t == type::get<uint8_t>())
      writer.Uint(var.to_uint8());
    else if (t == type::get<uint16_t>())
      writer.Uint(var.to_uint16());
    else if (t == type::get<uint32_t>())
      writer.Uint(var.to_uint32());
    else if (t == type::get<uint64_t>())
      writer.Uint64(var.to_uint64());
    else if (t == type::get<float>())
      writer.Double(var.to_double());
    else if (t == type::get<double>())
      writer.Double(var.to_double());

    return true;
  } else if (t.is_enumeration()) {
    bool ok = false;
    auto result = var.to_string(&ok);
    if (ok) {
      writer.String(var.to_string());
    } else {
      ok = false;
      auto value = var.to_uint64(&ok);
      if (ok)
        writer.Uint64(value);
      else
        writer.Null();
    }

    return true;
  } else if (t == type::get<std::string>()) {
    // auto str = var.to_string();
    writer.String(var.to_string());
    return true;
  } else if (write_optinal_types_to_json(t, var, writer)) {
    return true;
  }

  return false;
}

/////////////////////////////////////////////////////////////////////////////////////////

static void write_array(const variant_sequential_view& view, PrettyWriter<StringBuffer>& writer)
{
  writer.StartArray();
  for (const auto& item : view) {
    if (item.is_sequential_container()) {
      write_array(item.create_sequential_view(), writer);
    } else {
      // auto xx = item.to_string();  // debug
      variant wrapped_var = item.extract_wrapped_value();
      type value_type = wrapped_var.get_type();
      if (value_type.is_arithmetic() || value_type == type::get<std::string>() || value_type.is_enumeration()) {
        write_atomic_types_to_json(value_type, wrapped_var, writer);
      } else  // object
      {
        auto itype = item.get_type();
        auto id = to_json_recursively(wrapped_var);
        write_IdHolder(id, wrapped_var, writer);
      }
    }
  }
  writer.EndArray();
}

/////////////////////////////////////////////////////////////////////////////////////////

static void write_associative_container(const variant_associative_view& view, PrettyWriter<StringBuffer>& writer)
{
  static const string_view key_name("key");
  static const string_view value_name("value");

  writer.StartArray();

  if (view.is_key_only_type()) {
    for (auto& item : view) {
      write_variant(item.first, writer);
    }
  } else {
    for (auto& item : view) {
      writer.StartObject();
      writer.String(key_name.data(), static_cast<rapidjson::SizeType>(key_name.length()), false);

      write_variant(item.first, writer);

      writer.String(value_name.data(), static_cast<rapidjson::SizeType>(value_name.length()), false);
      write_variant(item.second, writer);

      writer.EndObject();
    }
  }

  writer.EndArray();
}

/////////////////////////////////////////////////////////////////////////////////////////

string get_optional_type(const string& str)
{
  size_t start_pos = str.find("<");
  size_t end_pos = str.find(">", start_pos);
  return str.substr(start_pos + 1, end_pos - start_pos - 1);
}

variant strip_optional(const variant& var)
{
  auto name = get_optional_type(var.get_type().get_name().to_string());
  variant copied = var;
  auto striped = type::get_by_name(name).create();
  striped.swap(copied);
  return std::move(striped);
}

/////////////////////////////////////////////////////////////////////////////////////////

ID_TYPE write_variant(const variant& var, PrettyWriter<StringBuffer>& writer)
{
  auto value_type = var.get_type();
  auto wrapped_type = value_type.is_wrapper() ? value_type.get_wrapped_type() : value_type;
  bool is_wrapper = wrapped_type != value_type;
  ID_TYPE id = "1";  // 1 just represent success
  if (write_atomic_types_to_json(is_wrapper ? wrapped_type : value_type, is_wrapper ? var.extract_wrapped_value() : var,
                                 writer)) {
  } else if (var.is_sequential_container()) {
    write_array(var.create_sequential_view(), writer);
  } else if (var.is_associative_container()) {
    write_associative_container(var.create_associative_view(), writer);
  } else if (!wrapped_type.is_pointer()) {
    // if (is_optional(wrapped_type)) {
    //   // optional
    //   auto striped = strip_optional(var);
    //   to_json_recursively(striped, writer);
    // } else {
    //   // object
    //   to_json_recursively(var, writer);
    // }
    to_json_recursively(var, writer);

  } else {
    // pointer
    auto child_props = is_wrapper ? wrapped_type.get_properties() : value_type.get_properties();
    if (!child_props.empty()) {
      id = to_json_recursively(var);
      write_IdHolder(id, var, writer);

    } else {
      bool ok = false;
      auto text = var.to_string(&ok);
      if (!ok) {
        writer.String(text);
        return "0";  // 0 : failed else id
      }

      writer.String(text);
    }
  }

  return id;
}

/////////////////////////////////////////////////////////////////////////////////////////
// TODO(): 优先级算法-->调用指针次数多的优先
///////
// int after_count = 0;
// int before_count = 0;
int total = 0;
int level = 0;
ID_TYPE to_json_recursively(const instance& obj2)
{
  ID_TYPE cid = std::nullopt;
  if (get_g_key(obj2, cid)) {
    return cid;
  }
  level++;
  StringBuffer sb;
  PrettyWriter<StringBuffer> writer(sb);
  writer.StartObject();
  instance obj = obj2.get_type().get_raw_type().is_wrapper() ? obj2.get_wrapped_instance() : obj2;
  c2redis::debug_log(1, obj.get_type().get_name().to_string());
  auto prop_list = obj.get_derived_type().get_properties();
  for (auto prop : prop_list) {
    if (prop.get_metadata("NO_SERIALIZE"))
      continue;
    c2redis::debug_log(2, "dealing prop: " + prop.get_type().get_name().to_string());
    variant prop_value = prop.get_value(obj);
    if (!prop_value)
      continue;  // cannot serialize, because we cannot retrieve the value

    const auto name = prop.get_name();
    writer.String(name.data(), static_cast<rapidjson::SizeType>(name.length()), false);

    if (write_variant(prop_value, writer) == "0") {
      std::cerr << "cannot serialize property or pointer is null: " << name << std::endl;
    }
  }

  writer.EndObject();
  // std::cout << sb.GetString() << "\n";
  ///// redis storing  /////
  auto aux = RedisAux::GetRedisAux(false, 1);
  auto classname = obj.get_derived_type().get_raw_type().get_name().to_string();
  // TODO(): add lock
  level--;
  if (!aux->hset(classname, *cid, sb.GetString())) {
    std::cerr << "error : hset overwrite with key " + classname << "\n";
  } else {
    c2redis::debug_log(1, "writed:" + classname + " in level:" + std::to_string(level));
    total++;
  }
  return cid;
}

void to_json_recursively(const instance& obj2, PrettyWriter<StringBuffer>& writer)
{
  writer.StartObject();
  instance obj = obj2.get_type().get_raw_type().is_wrapper() ? obj2.get_wrapped_instance() : obj2;

  auto prop_list = obj.get_derived_type().get_properties();
  for (auto prop : prop_list) {
    if (prop.get_metadata("NO_SERIALIZE"))
      continue;

    variant prop_value = prop.get_value(obj);
    if (!prop_value)
      continue;  // cannot serialize, because we cannot retrieve the value

    const auto name = prop.get_name();
    writer.String(name.data(), static_cast<rapidjson::SizeType>(name.length()), false);
    if (write_variant(prop_value, writer) == "0") {
      std::cerr << "cannot serialize property: " << name << std::endl;
    }
  }

  writer.EndObject();
}

}  // namespace

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

namespace io {

/////////////////////////////////////////////////////////////////////////////////////////

ID_TYPE to_json(rttr::instance obj)
{
  if (!obj.is_valid())
    return "0";

  // StringBuffer sb;
  // PrettyWriter<StringBuffer> writer(sb);
  // to_json_recursively(obj);

  // 每次反射存储任务, 每个类加入一个map, 识别指针是否存过
  // 如果已经存在指针的key, 则返回存的cid;
  // 如果不存在, 生成cid存入map, 并继续序列化
  level = 0;
  auto id = to_json_recursively(obj);
  // std::cout << "layer count before : " << before_count << "\n";
  // std::cout << "layer count after : " << after_count << "\n";
  std::cout << "total count : " << total << "\n";
  return *id;
  // return sb.GetString();
}

/////////////////////////////////////////////////////////////////////////////////////////

}  // end namespace io
