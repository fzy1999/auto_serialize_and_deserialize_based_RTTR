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

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <array>

#include <iostream>
#include "myrttr/instance.h"
#include "myrttr/property.h"
#include "myrttr/type.h"
#include "myrttr/variant.h"

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/prettywriter.h>  // for stringify JSON
#include <rapidjson/document.h>      // rapidjson's DOM-style API
#include "myrttr/type"
#include "../redis_helper/redis_helper.h"
#include "from_json.h"

using namespace rapidjson;
using namespace rttr;
using std::string;
namespace {
std::unordered_map<ID_TYPE, variant> g_key_storage;
class GKeyMutex
{
  std::mutex _g_key_mutex;

 public:
  GKeyMutex() { _g_key_mutex.lock(); }
  ~GKeyMutex() { _g_key_mutex.unlock(); }
};

auto get_g_key(const ID_TYPE& cid)
{
  GKeyMutex mutex;
  if (g_key_storage.contains(cid)) {
    variant var = g_key_storage[cid];
    return std::make_pair(true, var);
  }
  return std::make_pair(false, variant());
}

bool set_g_key(const ID_TYPE& cid, const variant& var)
{
  GKeyMutex mutex;
  if (!g_key_storage.contains(cid)) {
    if (var.is_valid()) {
      g_key_storage[cid] = var;
      return true;
    }
  }
  return false;
}
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

inline instance check_get_wrapped(instance& obj2)
{
  return obj2.get_type().get_raw_type().is_wrapper() ? obj2.get_wrapped_instance() : obj2;
}

/////////////////////////////////////////////////////////////////////////////////////////

inline type get_wrapped_type(const type& type2)
{
  return type2.is_wrapper() ? type2.get_wrapped_type().get_raw_type() : type2.get_raw_type();
}

/////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief TODO(): 最后应用后需要有一个free_all_map进行内存释放
 *
 * @param var
 * @return whether malloc is ready & variant
 */
auto malloc_class(const rttr::type& t, const ID_TYPE& cid)
{
  auto [has, var] = get_g_key(cid);
  if (has) {
    if (!var.is_valid()) {
      std::cerr << "error: get variant with cid failed \n";
      exit(1);
    }
    return std::make_pair(true, var);
  }
  auto _type = get_wrapped_type(t).get_name().to_string();
  var = type::get_by_name(_type).create();
  if (!set_g_key(cid, var)) {
    std::cerr << "error: set variant with cid failed \n";
    exit(1);
  }
  debug_log(2, " malloc class: " + var.get_type().get_name().to_string());
  return std::make_pair(false, var);
}

/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////

void fromjson_recursively(instance obj, Value& json_object);

void fromjson_recursively(instance obj2, const ID_TYPE cid);

/////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief caution!!: 返回值variant持有shared_ptr,保证malloc_class申请的内存空间
 *
 * @param var
 * @param json_value
 * @return variant
 */
variant restore_object(const rttr::type& t, rapidjson::GenericValue<rapidjson::UTF8<>>& json_value)
{
  io::IdHolder idholder;  // for getting cid
  fromjson_recursively(idholder, json_value);
  auto theid = idholder.id;
  auto [ready, var] = malloc_class(t, theid);  //!!!!
  if (!ready) {
    fromjson_recursively(var, theid);
  }
  // var = var.get_type().is_wrapper() ? var.extract_wrapped_value() : var;

  // instance tmp_inst_unwarpper
  //     = tmp_inst.get_type().get_raw_type().is_wrapper() ? tmp_inst.get_wrapped_instance() : tmp_inst;
  return var.get_type().is_wrapper() ? var.extract_wrapped_value() : var;
}

/////////////////////////////////////////////////////////////////////////////////////////

variant extract_basic_types(Value& json_value)
{
  switch (json_value.GetType()) {
    case kStringType: {
      return std::string(json_value.GetString());
      break;
    }
    case kNullType:
      break;
    case kFalseType:
    case kTrueType: {
      return json_value.GetBool();
      break;
    }
    case kNumberType: {
      if (json_value.IsInt())
        return json_value.GetInt();
      else if (json_value.IsDouble())
        return json_value.GetDouble();
      else if (json_value.IsUint())
        return json_value.GetUint();
      else if (json_value.IsInt64())
        return json_value.GetInt64();
      else if (json_value.IsUint64())
        return json_value.GetUint64();
      break;
    }
    // we handle only the basic types here
    case kObjectType:
    case kArrayType:
      return {};
  }

  return {};
}

/////////////////////////////////////////////////////////////////////////////////////////

static void write_array_recursively(variant_sequential_view& view, Value& json_array_value)
{
  view.set_size(json_array_value.Size());
  const type array_value_type = view.get_rank_type(1);

  for (SizeType i = 0; i < json_array_value.Size(); ++i) {
    auto& json_index_value = json_array_value[i];
    if (json_index_value.IsArray()) {
      auto sub_array_view = view.get_value(i).create_sequential_view();
      write_array_recursively(sub_array_view, json_index_value);
    } else if (json_index_value.IsObject()) {
      auto _type = array_value_type;
      // variant var_tmp = view.get_value(i);
      // variant wrapped_var = var_tmp.extract_wrapped_value();
      // fromjson_recursively(wrapped_var, json_index_value);
      auto wrapped_var = restore_object(_type, json_index_value);
      if (!view.set_value(i, wrapped_var)) {
        std::cerr << "write_array_recursively set_value failed!\n";
        exit(1);
      }

    } else {
      variant extracted_value = extract_basic_types(json_index_value);
      if (extracted_value.convert(array_value_type))
        view.set_value(i, extracted_value);
    }
  }
}

variant extract_value(Value::MemberIterator& itr, const type& t)
{
  auto& json_value = itr->value;
  variant extracted_value = extract_basic_types(json_value);
  const bool could_convert = extracted_value.convert(t);
  if (!could_convert) {
    if (json_value.IsObject()) {
      // constructor ctor = t.get_constructor();
      // for (auto& item : t.get_constructors()) {
      //   if (item.get_instantiated_type() == t)
      //     ctor = item;
      // }
      // extracted_value = ctor.invoke();
      // fromjson_recursively(extracted_value, json_value);
      auto xx = t.get_name();
      extracted_value = restore_object(t, json_value);
    }
  }

  return extracted_value;
}

static void write_associative_view_recursively(variant_associative_view& view, Value& json_array_value)
{
  for (SizeType i = 0; i < json_array_value.Size(); ++i) {
    auto& json_index_value = json_array_value[i];
    if (json_index_value.IsObject())  // a key-value associative view
    {
      Value::MemberIterator key_itr = json_index_value.FindMember("key");
      Value::MemberIterator value_itr = json_index_value.FindMember("value");

      if (key_itr != json_index_value.MemberEnd() && value_itr != json_index_value.MemberEnd()) {
        auto key_var = extract_value(key_itr, view.get_key_type());
        auto value_var = extract_value(value_itr, view.get_value_type());
        if (key_var && value_var) {
          view.insert(key_var, value_var);
        }
      }
    } else  // a key-only associative view
    {
      variant extracted_value = extract_basic_types(json_index_value);
      if (extracted_value && extracted_value.convert(view.get_key_type()))
        view.insert(extracted_value);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////

void fromjson_recursively(instance obj2, Value& json_object)
{
  instance obj = obj2.get_type().get_raw_type().is_wrapper() ? obj2.get_wrapped_instance() : obj2;
  const auto prop_list = obj.get_derived_type().get_properties();

  for (auto prop : prop_list) {
    Value::MemberIterator ret = json_object.FindMember(prop.get_name().data());
    if (ret == json_object.MemberEnd())
      continue;
    const type value_t = prop.get_type();

    auto& json_value = ret->value;
    switch (json_value.GetType()) {
      case kArrayType: {
        variant var;
        if (value_t.is_sequential_container()) {
          var = prop.get_value(obj);
          auto view = var.create_sequential_view();
          write_array_recursively(view, json_value);
        } else if (value_t.is_associative_container()) {
          var = prop.get_value(obj);
          auto associative_view = var.create_associative_view();
          write_associative_view_recursively(associative_view, json_value);
        }

        prop.set_value(obj, var);
        break;
      }
      case kObjectType: {
        variant var = prop.get_value(obj);
        fromjson_recursively(var, json_value);
        prop.set_value(obj, var);
        break;
      }
      default: {
        variant extracted_value = extract_basic_types(json_value);
        if (extracted_value.convert(
                value_t))  // REMARK: CONVERSION WORKS ONLY WITH "const type", check whether this is correct or not!
          prop.set_value(obj, extracted_value);
      }
    }
  }
}

/**
 * @brief restore from redis
 *
 * @param obj2
 * @param cid
 */
void fromjson_recursively(instance obj2, const ID_TYPE cid)
{
  instance obj = obj2.get_type().get_raw_type().is_wrapper() ? obj2.get_wrapped_instance() : obj2;
  // get this json from redis
  auto aux = RedisAux::GetRedisAux();
  auto classname = obj.get_type().get_raw_type().get_name().to_string();
  debug_log(1, "parsing class: " + classname);
  string json(aux->hget(classname, cid));
  Document json_object;

  if (json_object.Parse(json.c_str()).HasParseError()) {
    std::cerr << "error in document.Parse\n";
  }

  const auto prop_list = obj.get_derived_type().get_properties();

  for (auto prop : prop_list) {
    Value::MemberIterator ret = json_object.FindMember(prop.get_name().data());
    if (ret == json_object.MemberEnd())
      continue;
    const type value_t = prop.get_type();
    debug_log(2, "- parsing prop: " + value_t.get_name().to_string());
    auto& json_value = ret->value;
    switch (json_value.GetType()) {
      case kArrayType: {
        variant var;
        if (value_t.is_sequential_container()) {
          var = prop.get_value(obj);
          auto view = var.create_sequential_view();
          write_array_recursively(view, json_value);
        } else if (value_t.is_associative_container()) {
          var = prop.get_value(obj);
          auto associative_view = var.create_associative_view();
          write_associative_view_recursively(associative_view, json_value);
        }

        prop.set_value(obj, var);
        break;
      }
      case kObjectType: {
        // TODO(): 处理并非指针的情况!!
        // 从id还原回来指针, variant 发生了数据拷贝
        // variant var = prop.get_value(obj);
        auto var = restore_object(prop.get_type(), json_value);
        // io::IdHolder idholder;  // for getting cid
        // fromjson_recursively(idholder, json_value);
        // auto theid = idholder.id;
        // auto ready = malloc_class(var, theid);  //!!!!
        // fromjson_recursively(var, theid);

        // prop.set_value_raw_ptr(obj, tmp_inst_unwarpper.get_object_pointer());
        auto ok = prop.set_value(obj, var);
        debug_log(2, "- prop set object: " + prop.get_value(obj).to_string());
        break;
      }
      default: {
        variant extracted_value = extract_basic_types(json_value);
        if (extracted_value.convert(
                value_t))  // REMARK: CONVERSION WORKS ONLY WITH "const type", check whether this is correct or not!
        {
          prop.set_value(obj, extracted_value);
          debug_log(2, "- prop set value: " + extracted_value.to_string());
        }
      }
    }
  }
}
}  // namespace

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

namespace io {

bool from_json(const std::string& json, rttr::instance obj)
{
  Document document;  // Default template parameter uses UTF8 and MemoryPoolAllocator.

  // "normal" parsing, decode strings to new buffers. Can use other input stream via ParseStream().
  if (document.Parse(json.c_str()).HasParseError())
    return 1;

  fromjson_recursively(obj, document);

  return true;
}

/**
 * @brief TODO(): 需要保证obj持有的类中,包括指向的指针已经分配好内存??
 *
 * @param json
 * @param obj
 * @return true
 * @return false
 */
bool from_key(const std::string& key, rttr::instance obj)
{
  fromjson_recursively(obj, key);
  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////

}  // end namespace io
