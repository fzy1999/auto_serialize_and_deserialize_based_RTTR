#include "batch_to.h"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <thread>
#include <utility>
#include "common.h"
#include "myrttr/instance.h"
#include "ThreadPool/ThreadPool.h"
#include "myrttr/string_view.h"
namespace c2redis {
ID_TYPE TaskDict::generate_key(void* ptr)
{
  char buffer[8];
  sprintf(buffer, "%p", ptr);
  return std::move(buffer);
}

void TaskDict::add_task(void* ptr, TASK_PTR task)
{
  _tasks.emplace_back(task);
  _dict[ptr] = std::move(task);
}

TASK_PTR TaskDict::get_next()
{
  if (walker == tail) {
    tail = _tasks.size();
    level++;
  }
  return _tasks[walker++];
}

// TODO():
void TaskDict::serialize_all()
{
  // ThreadPool pool(thread_num);
  while (has_next()) {
    while (has_unread()) {
    }
  }
}
// check and get
TASK_PTR TaskDict::get_key(const instance& inst)
{
  // auto inst = task->inst;
  if (!inst.is_valid()) {
    return nullptr;
  }
  void* key;
  if (inst.get_derived_type().is_wrapper()) {
    key = inst.get_wrapped_instance().get_object_pointer();
  } else {
    key = inst.get_object_pointer();
  }
  if (key == nullptr) {
    return nullptr;
  }
  GKeyMutex mutex;
  if (!_dict.contains(key)) {
    auto cid = generate_key(key);
    auto task = std::make_shared<Task>(inst);
    task->cid = *cid;
    task->cname = inst.get_derived_type().get_raw_type().get_name().to_string();

    add_task(key, task);
    // _dict[key] = task;
    return task;
  }
  // cid = task_dict[key];
  // std::cout << inst.get_type().get_name().to_string() << " pointer same : " << key << "\n";
  return _dict[key];
}

// only allocate key and set to map
void TaskAllocator::allocate_all(rttr::instance& inst, ID_TYPE& cid)
{
  auto task = dict.get_key(inst);
  if (task == nullptr) {
    cid = std::nullopt;
    return;  // TODO():
  }
  cid = task->cid;
  while (dict.has_next()) {
    auto task = dict.get_next();
    auto obj = get_wrapped(task->inst);
    allocate_instance(obj);
  }
}

void TaskAllocator::serialize_all()
{
  // ThreadPool pool(thread_num);

  while (dict.has_unread()) {
    auto task = dict.get_next_unread();
    StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);
    auto obj = get_wrapped(task->inst);

    serialize_instance(obj, writer);

    std::cout << sb.GetString() << std::endl;
    task->json = sb.GetString();
    if (task->cname.empty()) {
      task->cname = obj.get_derived_type().get_raw_type().get_name().to_string();
    }
  }
}

void TaskAllocator::serialize_variant(const variant& var, PrettyWriter<StringBuffer>& writer)
{
  if (!var.is_valid() || var.get_raw_ptr_tmp() == nullptr) {
    writer.Null();
  }
  auto wrapped_val = get_wrapped(var);
  if (wrapped_val.is_associative_container()) {
    serialize_associative_container(wrapped_val.create_associative_view(), writer);
  } else if (wrapped_val.is_sequential_container()) {
    serialize_sequential_container(wrapped_val.create_sequential_view(), writer);
  } else if (wrapped_val.get_type().is_pointer()) {
    serialize_pointer(wrapped_val, writer);
  } else if (serialize_atomic(wrapped_val, writer)) {
  } else {
    if (wrapped_val.get_type().get_properties().empty()) {
      writer.String(wrapped_val.to_string());
    } else {
      serialize_instance(wrapped_val, writer);
    }
  }
}

void TaskAllocator::serialize_instance(const instance& obj, PrettyWriter<StringBuffer>& writer)
{
  writer.StartObject();
  auto prop_list = obj.get_derived_type().get_properties();
  for (auto prop : prop_list) {
    rttr::variant prop_value = prop.get_value(obj);
    if (!prop_value)
      continue;
    const auto name = prop.get_name();
    writer.String(name.data(), static_cast<rapidjson::SizeType>(name.length()), false);
    serialize_variant(prop_value, writer);
  }
  writer.EndObject();
}

bool TaskAllocator::serialize_pointer(const variant& var, PrettyWriter<StringBuffer>& writer)
{
  auto task = dict.get_key(var);
  if (!task) {
    writer.Null();
  } else {
    writer.StartObject();
    writer.String(task->cid.data(), static_cast<rapidjson::SizeType>(task->cid.length()), false);
    writer.String(task->cname.data(), static_cast<rapidjson::SizeType>(task->cname.length()), false);
    writer.EndObject();
  }
  return true;
}

bool TaskAllocator::serialize_atomic(const variant& var, PrettyWriter<StringBuffer>& writer)
{
  auto t = var.get_type();
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
  } else if (write_optinal_types_to_json(var, writer)) {
    return true;
  }

  return false;
}

void TaskAllocator::serialize_associative_container(const variant_associative_view& view,
                                                    PrettyWriter<StringBuffer>& writer)
{
  static const string_view key_name("key");
  static const string_view value_name("value");
  writer.StartArray();

  if (view.is_key_only_type()) {
    for (auto& item : view) {
      serialize_variant(item.first, writer);
    }
  } else {
    for (auto& item : view) {
      writer.StartObject();
      writer.String(key_name.data(), static_cast<rapidjson::SizeType>(key_name.length()), false);
      serialize_variant(item.first, writer);

      writer.String(value_name.data(), static_cast<rapidjson::SizeType>(value_name.length()), false);
      serialize_variant(item.second, writer);

      writer.EndObject();
    }
  }

  writer.EndArray();
}

void TaskAllocator::serialize_sequential_container(const variant_sequential_view& view,
                                                   PrettyWriter<StringBuffer>& writer)
{
  writer.StartArray();
  for (const auto& item : view) {
    serialize_variant(item, writer);
  }
  writer.EndArray();
}

// TODO():
void TaskAllocator::allocate_associative_container(const rttr::variant_associative_view& view)
{
  if (view.is_key_only_type()) {
    for (const auto& item : view) {
      if (!get_wrapped(item.first).get_type().is_pointer()) {
        continue;
      }
      create_task(item.first);
    }
  } else {
    for (auto& item : view) {
      if (!get_wrapped(item.first).get_type().is_pointer()) {
        continue;
      }
      create_task(item.first);
      create_task(item.second);
    }
  }
}

void TaskAllocator::allocate_sequential_container(const rttr::variant_sequential_view& view)
{
  for (const auto& item : view) {
    if (item.is_sequential_container()) {
      allocate_sequential_container(item.create_sequential_view());
    } else if (item.is_associative_container()) {
      allocate_associative_container(item.create_associative_view());
    } else {
      if (item.get_type().is_pointer()) {
        create_task(item);
      }
    }
  }
}

void TaskAllocator::allocate_instance(const instance& obj)
{
  auto prop_list = obj.get_derived_type().get_properties();
  for (auto prop : prop_list) {
    rttr::variant prop_value = prop.get_value(obj);
    if (!prop_value)
      continue;
    auto wrapped_val = get_wrapped(prop_value);
    if (wrapped_val.is_associative_container()) {
      allocate_associative_container(wrapped_val.create_associative_view());
    } else if (wrapped_val.is_sequential_container()) {
      allocate_sequential_container(wrapped_val.create_sequential_view());
    } else if (wrapped_val.get_type().is_pointer()) {
      create_task(wrapped_val);
    } else if (wrapped_val.get_type().is_class()) {
      allocate_instance(wrapped_val);
    }
  }
}

rttr::instance TaskAllocator::get_wrapped(const rttr::instance& inst)
{
  return inst.get_type().get_raw_type().is_wrapper() ? inst.get_wrapped_instance() : inst;
}

rttr::variant TaskAllocator::get_wrapped(const rttr::variant& var)
{
  auto value_type = var.get_type();
  auto wrapped_type = value_type.is_wrapper() ? value_type.get_wrapped_type() : value_type;
  bool is_wrapper = wrapped_type != value_type;
  return is_wrapper ? var.extract_wrapped_value() : var;
}

void TaskAllocator::create_task(const rttr::variant& val)
{
  dict.get_key(val);  // dont we need return?
}

bool TaskAllocator::write_optinal_types_to_json(const variant& var, PrettyWriter<StringBuffer>& writer)
{
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
  else if (var.is_type<std::optional<std::string>>())
    writer.String(var.get_value<std::string>());
  else
    return false;

  return true;
}

ID_TYPE ToRedis::operator()(instance inst)
{
  ID_TYPE cid;
  std::thread alloc_thread(&ToRedis::call_alloc, this, std::ref(inst), std::ref(cid));
  std::thread serialize_thread(&ToRedis::call_serialize_all, this);
  alloc_thread.join();
  serialize_thread.join();
  return cid;
}

inline void ToRedis::call_alloc(rttr::instance& inst, ID_TYPE& cid)
{
  allocator.allocate_all(inst, cid);
}

void ToRedis::call_serialize_all()
{
  allocator.serialize_all();
}

}  // namespace c2redis