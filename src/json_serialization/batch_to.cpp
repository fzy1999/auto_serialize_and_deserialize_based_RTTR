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
#include "../redis_helper/redis_helper.h"
namespace c2redis {
ID_TYPE TaskDict::generate_key(void* ptr)
{
  char buffer[8];
  sprintf(buffer, "%p", ptr);
  return std::move(buffer);
}

void TaskDict::add_task(void* ptr, TASK_PTR task)
{
  assert(task);
  _dict[ptr] = task;
  _tasks.push_back(task);
}

inline instance TaskDict::get_wrapped(const instance& inst)
{
  return inst.get_type().get_raw_type().is_wrapper() ? inst.get_wrapped_instance() : inst;
}

TASK_PTR TaskDict::get_next()
{
  if (walker == tail) {
    tail = _tasks.size();
    level++;
  }
  assert(_tasks[walker]);
  return _tasks[walker++];
}

// TODO():
void TaskDict::serialize_all()
{
}

TASK_PTR TaskDict::get_next_unstore()
{
  assert(keeper < reader);
  return _tasks[keeper++];
}

inline TASK_PTR TaskDict::get_next_unread()
{
  assert(reader < walker);
  return _tasks[reader++];
}
// check and get
TASK_PTR TaskDict::add_key(const instance& inst)
{
  // auto inst = task->inst;
  if (!inst.is_valid()) {
    return nullptr;
  }

  void* key;
  if (inst.get_derived_type().get_raw_type().is_wrapper()) {
    key = inst.get_wrapped_instance().get_object_pointer();
  } else {
    key = inst.get_object_pointer();
  }
  // void* key = wrapped_inst.get_object_pointer();
  if (key == nullptr) {
    return nullptr;
  }
  GKeyMutex mutex;
  if (!_dict.contains(key)) {
    auto cid = generate_key(key);
    auto task = std::make_shared<Task>(inst);
    task->cid = *cid;
    task->cname = get_wrapped(inst).get_derived_type().get_raw_type().get_name().to_string();
    add_task(key, task);
    return task;
  }
  assert(_dict[key]);
  return _dict[key];
}

TASK_PTR TaskDict::get_key(const instance& inst)
{
  if (!inst.is_valid()) {
    return nullptr;
  }
  void* key;
  if (inst.get_derived_type().get_raw_type().is_wrapper()) {
    key = inst.get_wrapped_instance().get_object_pointer();
  } else {
    key = inst.get_object_pointer();
  }
  if (key == nullptr) {
    return nullptr;
  }
  assert(_dict.contains(key));
  // if (!_dict.contains(key)) {
  //   return std::make_shared<Task>();
  // }
  assert(_dict[key]);
  return _dict[key];
}

// only allocate key and set to map
void TaskAllocator::allocate_all(rttr::instance& inst, ID_TYPE& cid)
{
  auto start = std::chrono::high_resolution_clock::now();

  auto root = dict.add_key(inst);
  if (root == nullptr) {
    cid = std::nullopt;
    return;
  }
  cid = root->cid;
  int total = 0;
  while (dict.has_next()) {
    auto task = dict.get_next();
    assert(task);
    allocate_instance(task->inst);
    total++;
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "alloc total:" << total << "const time: " << duration(start, end) << std::endl << std::flush;
  alloc_finished = true;
}

void TaskAllocator::serialize_all()
{
  auto start = std::chrono::high_resolution_clock::now();

  // ThreadPool pool(thread_num);
  auto serialize_task_f = [this](auto& task) {
    StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);
    // auto obj = dict.get_wrapped(task->inst);
    serialize_instance(task->inst, writer);
    task->json = sb.GetString();
    //////////////
    // std::cout << task->cname << std::endl;
    // std::cout << sb.GetString() << std::endl;
    //////////////
    if (task->cname.empty()) {
      task->cname = dict.get_wrapped(task->inst).get_derived_type().get_raw_type().get_name().to_string();
    }
  };
  int seri_count = 0;
  while (!alloc_finished || dict.has_unread()) {
    while (dict.has_unread()) {
      auto task = dict.get_next_unread();
      assert(task);
      serialize_task_f(task);
      // pool.enqueue(serialize_task_f, task);
      seri_count++;
    }
  }
  read_finished = true;
  auto end = std::chrono::high_resolution_clock::now();

  std::cout << "serialization total:" << seri_count << "const time: " << duration(start, end) << std::endl
            << std::flush;
}

void TaskAllocator::store_all()
{
  auto start = std::chrono::high_resolution_clock::now();

  auto aux = RedisAux::GetRedisAux();
  int store_count = 0;
  string last_name;
  while (!read_finished || dict.has_unstore()) {
    while (dict.has_unstore()) {
      auto task = dict.get_next_unstore();
      aux->hset_piped(task->cname, task->cid, task->json);
      // std::cout << task->cname << ":" << task->cid << std::endl;
      store_count++;
    }
  }
  aux->exec_pipe();

  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "stored total:" << store_count << "const time: " << duration(start, end) << std::endl << std::flush;
}

void TaskAllocator::serialize_variant(const variant& var, PrettyWriter<StringBuffer>& writer)
{
  if (!var.is_valid() || var.get_raw_ptr_tmp() == nullptr) {
    writer.Null();
  }
  auto wrapped_val = dict.get_wrapped(var);
  if (wrapped_val.is_associative_container()) {
    serialize_associative_container(wrapped_val.create_associative_view(), writer);
  } else if (wrapped_val.is_sequential_container()) {
    serialize_sequential_container(wrapped_val.create_sequential_view(), writer);
  } else if (dict.get_wrapped(wrapped_val.get_type()).is_pointer()) {
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
  auto wrapped = dict.get_wrapped(obj);
  auto prop_list = wrapped.get_derived_type().get_properties();
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
      if (!dict.get_wrapped(item.first.get_type()).is_pointer()) {
        return;
      }
      create_task(item.first.extract_wrapped_value());
    }
  } else {
    for (auto& item : view) {
      if (dict.get_wrapped(item.first.get_type()).is_pointer()) {
        create_task(item.first.extract_wrapped_value());
      }
      if (dict.get_wrapped(item.second.get_type()).is_pointer()) {
        create_task(item.second.extract_wrapped_value());
      }
    }
  }
}

void TaskAllocator::allocate_sequential_container(const rttr::variant_sequential_view& view)
{
  auto n = view.get_size();
  for (const auto& item : view) {
    if (item.is_sequential_container()) {
      allocate_sequential_container(item.create_sequential_view());
    } else if (item.is_associative_container()) {
      allocate_associative_container(item.create_associative_view());
    } else {
      auto type = dict.get_wrapped(item.get_type());
      if (type.is_pointer()) {
        create_task(item.extract_wrapped_value());
      }
    }
  }
}

void TaskAllocator::allocate_instance(const instance& obj)
{
  auto name = obj.get_type().get_name().to_string();
  auto wrapped = dict.get_wrapped(obj);
  auto prop_list = wrapped.get_derived_type().get_properties();
  auto n = prop_list.size();
  for (auto prop : prop_list) {
    rttr::variant prop_value = prop.get_value(obj);
    if (!prop_value)
      continue;
    auto wrapped_val = dict.get_wrapped(prop_value);
    auto propname = dict.get_wrapped(wrapped_val.get_type());

    auto name_prop = wrapped_val.get_type().get_name().to_string();
    if (wrapped_val.is_associative_container()) {
      allocate_associative_container(wrapped_val.create_associative_view());
    } else if (wrapped_val.is_sequential_container()) {
      allocate_sequential_container(wrapped_val.create_sequential_view());
    } else if (dict.get_wrapped(wrapped_val.get_type()).is_pointer()) {
      create_task(wrapped_val);
    } else if (dict.get_wrapped(wrapped_val.get_type()).is_class()) {
      allocate_instance(wrapped_val);
    }
  }
}

rttr::variant TaskDict::get_wrapped(const rttr::variant& var)
{
  auto value_type = var.get_type();
  auto wrapped_type = value_type.is_wrapper() ? value_type.get_wrapped_type() : value_type;
  bool is_wrapper = wrapped_type != value_type;
  return is_wrapper ? var.extract_wrapped_value() : var;
}

type TaskDict::get_wrapped(const type& value_type)
{
  // auto wrapped_type = value_type.is_wrapper() ? value_type.get_wrapped_type() : value_type;
  auto wrapped_type = value_type;
  while (wrapped_type.is_wrapper()) {
    wrapped_type = wrapped_type.get_wrapped_type();
  }
  return wrapped_type;
}

void TaskAllocator::create_task(const rttr::variant& val)
{
  // caution!!! when pass a item like val in view , extract_wrapped_value is nessary!
  dict.add_key(val);  // dont we need return?
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
  alloc_thread.join();
  std::thread serialize_thread(&ToRedis::call_serialize_all, this);
  serialize_thread.join();
  std::thread store_thread(&ToRedis::call_store, this);
  store_thread.join();
  return cid;
}

inline void ToRedis::call_alloc(rttr::instance& inst, ID_TYPE& cid)
{
  allocator.allocate_all(inst, cid);
}

inline void ToRedis::call_serialize_all()
{
  allocator.serialize_all();
}

inline void ToRedis::call_store()
{
  allocator.store_all();
}

}  // namespace c2redis