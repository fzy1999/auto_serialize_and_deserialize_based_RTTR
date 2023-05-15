/************************************************************************************
 *                                                                                   *
 *   Copyright (c) 2014 - 2018 Axel Menzel <info@rttr.org>                           *
 *                                                                                   *
 *   This file is part of RTTR (Run Time Type Reflection)                            *
 *   License: MIT License                                                            *
 *                                                                                   *
 *   Permission is hereby granted, free of charge, to any person obtaining           *
 *   a copy of this software and associated documentation files (the "Software"),    *
 *   to deal in the Software without restriction, including without limitation       *
 *   the rights to use, copy, modify, merge, publish, distribute, sublicense,        *
 *   and/or sell copies of the Software, and to permit persons to whom the           *
 *   Software is furnished to do so, subject to the following conditions:            *
 *                                                                                   *
 *   The above copyright notice and this permission notice shall be included in      *
 *   all copies or substantial portions of the Software.                             *
 *                                                                                   *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
 *   SOFTWARE.                                                                       *
 *                                                                                   *
 *************************************************************************************/

#include "common.h"
#include "myrttr/variant.h"
#include "myrttr/type"
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>      // rapidjson's DOM-style API
#include <rapidjson/prettywriter.h>  // for stringify JSON

#include "../redis_helper/redis_helper.h"
#include "to_json.h"
#include <uuid/uuid.h>

using namespace rapidjson;
using namespace rttr;

namespace {

std::map<const void*, ID_TYPE> g_key_storage;
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
bool get_g_key(const void* key, ID_TYPE& cid)
{
  GKeyMutex mutex;
  if (!g_key_storage.contains(key)) {
    cid = generate_g_key();
    g_key_storage[key] = cid;
    return false;
  }
  cid = g_key_storage[key];
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

/**
 * @brief for writer passing in
 *
 * @param obj
 * @param writer
 */
void to_json_recursively(const instance& obj, PrettyWriter<StringBuffer>& writer);

ID_TYPE write_variant(const variant& var, PrettyWriter<StringBuffer>& writer);

void write_IdHolder(const ID_TYPE cid, PrettyWriter<StringBuffer>& writer)
{
  io::IdHolder id_holder{cid};
  to_json_recursively(id_holder, writer);
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
      auto xx = item.to_string();  // debug
      variant wrapped_var = item.extract_wrapped_value();
      type value_type = wrapped_var.get_type();
      if (value_type.is_arithmetic() || value_type == type::get<std::string>() || value_type.is_enumeration()) {
        write_atomic_types_to_json(value_type, wrapped_var, writer);
      } else  // object
      {
        // TODO(): 处理id
        auto id = to_json_recursively(wrapped_var);
        write_IdHolder(id, writer);
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
  } else {
    // pointer
    auto child_props = is_wrapper ? wrapped_type.get_properties() : value_type.get_properties();
    if (!child_props.empty()) {
      // TODO(): 处理id
      id = to_json_recursively(var);
      write_IdHolder(id, writer);
      // writer.String(std::to_string(id));
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

ID_TYPE to_json_recursively(const instance& obj2)
{
  ID_TYPE cid;
  auto op = obj2.get_object_pointer();
  if (get_g_key(obj2.get_object_pointer(), cid)) {
    return cid;
  }

  StringBuffer sb;
  PrettyWriter<StringBuffer> writer(sb);
  writer.StartObject();
  instance obj = obj2.get_type().get_raw_type().is_wrapper() ? obj2.get_wrapped_instance() : obj2;
  debug_log(2, obj.get_type().get_name().to_string());
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
      std::cerr << "cannot serialize property or pointer is null: " << name << std::endl;
    }
  }

  writer.EndObject();
  // std::cout << sb.GetString() << "\n";
  ///// redis storing  /////
  auto aux = RedisAux::GetRedisAux();
  auto classname = obj.get_type().get_raw_type().get_name().to_string();
  // TODO(): add lock
  auto val = sb.GetString();
  aux->hset(classname, cid, val);
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
  auto id = to_json_recursively(obj);

  return id;
  // return sb.GetString();
}

/////////////////////////////////////////////////////////////////////////////////////////

}  // end namespace io
