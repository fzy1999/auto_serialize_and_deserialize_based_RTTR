#include "batch_from.h"

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include "../redis_helper/redis_helper.h"
#include "c2redis/test/basic_test/initiate.h"
#include "common.h"
#include "myrttr/instance.h"
#include "myrttr/type.h"
#include "myrttr/variant.h"
#include "rapidjson/document.h"
namespace c2redis {
bool is_optional(const type& t)
{
  return t.get_name().to_string().find("optional") != string::npos;
}
variant convert_optinal_to_basic(variant& var)
{
  auto tt = var.get_type();
  if (is_optional(var.get_type())) {
    if (var.is_type<std::optional<bool>>()) {
      return var.get_value<bool>();
    }
    if (var.is_type<std::optional<char>>()) {
      return var.get_value<char>();
    }
    if (var.is_type<std::optional<int8_t>>()) {
      return var.get_value<int8_t>();
    }
    if (var.is_type<std::optional<int16_t>>()) {
      return var.get_value<int16_t>();
    }
    if (var.is_type<std::optional<int32_t>>()) {
      return var.get_value<int32_t>();
    }
    if (var.is_type<std::optional<int64_t>>()) {
      return var.get_value<int64_t>();
    }
    if (var.is_type<std::optional<uint8_t>>()) {
      return var.get_value<uint8_t>();
    }
    if (var.is_type<std::optional<uint16_t>>()) {
      return var.get_value<uint16_t>();
    }
    if (var.is_type<std::optional<uint32_t>>()) {
      return var.get_value<uint32_t>();
    }
    if (var.is_type<std::optional<uint64_t>>()) {
      return var.get_value<uint64_t>();
    }
    if (var.is_type<std::optional<float>>()) {
      return var.get_value<float>();
    }
    if (var.is_type<std::optional<double>>()) {
      return var.get_value<double>();
    }
    return {};
  }
  return {};
}
variant FromTask::extract_basic_types(Value& json_value)
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
      if (json_value.IsInt()) {
        return json_value.GetInt();
      }
      if (json_value.IsUint()) {
        return json_value.GetUint();
      }
      if (json_value.IsDouble()) {
        return json_value.GetDouble();
      }
      if (json_value.IsInt64()) {
        return json_value.GetInt64();
      }
      if (json_value.IsUint64()) {
        return json_value.GetUint64();
      }

      if (json_value.IsFloat()) {
        return json_value.GetFloat();
      }

      break;
    }
    // we handle only the basic types here
    case kObjectType:
    case kArrayType:
      return {};
  }

  return {};
}

//////////// TODO(): change to constexp
#define CREATE_PUSH_TASK(ctype)                                                                \
  do {                                                                                         \
    auto prop_task = std::make_shared<ctype>(prop.get_name().to_string(), shared_from_this()); \
    prop_task->_json_val = json_value.Move();                                                  \
    prop_task->parse_id_type();                                                                \
    tasks.push(prop_task);                                                                     \
  } while (0)
///////////////////////

// only push to queue for now
template <typename T>
static void create_push_task(const string& propname, Value& json_value, const FTASK_PTR& parent,
                             FTaskQue& tasks)
{
  auto prop_task = std::make_shared<T>(propname, parent);
  prop_task->parse_id_type(json_value);
  tasks.push(prop_task);
}

// TODO(): bug: not supporting mutiple layers with pointer
void FromTask::create_sequential_task(const string& propname, rttr::variant& var, Value& json_value,
                                      FTaskQue& tasks)
{
  auto view = var.create_sequential_view();
  const type array_value_type = view.get_rank_type(1);
  if (get_wrapped(array_value_type).is_pointer()) {
    auto prop_task = std::make_shared<FmSequetialTask>(propname, shared_from_this());
    prop_task->parse_id_type(json_value);
    prop_task->_pvar = std::make_shared<variant>(std::move(var));
    tasks.push(prop_task);
  } else if (get_wrapped(array_value_type).is_class()) {
    auto prop_task = std::make_shared<FmSequetialTask>(propname, shared_from_this());
    prop_task->_json_object.CopyFrom(json_value, prop_task->_json_object.GetAllocator());
    prop_task->_pvar = std::make_shared<variant>(std::move(var));
    tasks.push(prop_task);
    // todo
  } else {
    view.set_size(json_value.Size());
    for (SizeType i = 0; i < json_value.Size(); ++i) {
      auto& json_index_value = json_value[i];
      if (json_index_value.IsArray()) {
        std::cerr << "dealing array in array, may cause error: " << propname << "\n";

        auto sub_array = view.get_value(i);
        create_sequential_task(propname, sub_array, json_index_value, tasks);
      } else if (json_index_value.IsObject()) {
        // auto wrapped_value = view.get_value(i).extract_wrapped_value();
        // assert(view.set_value(i, wrapped_value));
      } else {
        variant extracted_value = extract_basic_types(json_index_value);
        if (extracted_value.convert(array_value_type)) {
          assert(view.set_value(i, extracted_value));
        }
      }
    }
  }
}

// TODO(): map<,object> object contains pointer
// cautions: support only <string, other>, cannot parse <,map<>>
void FromTask::create_associative_task(const string& propname, rttr::variant& var,
                                       Value& json_value, FTaskQue& tasks)
{
  auto view = var.create_associative_view();
  if (get_wrapped(view.get_value_type()).is_pointer()) {
    auto prop_task = std::make_shared<FmAssociativeTask>(propname, shared_from_this());
    prop_task->parse_id_type(json_value);
    prop_task->_pvar = std::make_shared<variant>(std::move(var));
    tasks.push(prop_task);
    return;
  }
  for (SizeType i = 0; i < json_value.Size(); ++i) {
    auto& json_index_value = json_value[i];
    if (json_index_value.IsObject())  // a key-value associative view
    {
      Value::MemberIterator key_itr = json_index_value.FindMember("key");
      Value::MemberIterator value_itr = json_index_value.FindMember("value");
      if (key_itr != json_index_value.MemberEnd() && value_itr != json_index_value.MemberEnd()) {
        auto key_jval = extract_basic_types(key_itr->value);
        auto value_jval = extract_basic_types(value_itr->value);
        bool could_convert = value_jval.convert(view.get_value_type());
        could_convert &= key_jval.convert(view.get_key_type());
        // TODO(): supporting object key type
        if (!could_convert && value_itr->value.IsObject()) {
          // TODO(): potentail bug <.., object> , object contain pointer
        }
        if (key_jval && value_jval) {
          view.insert(key_jval, value_jval);
        }
      }
    } else  // a key-only associative view
    {
      variant extracted_value = extract_basic_types(json_index_value);
      if (extracted_value && extracted_value.convert(view.get_key_type())) {
        view.insert(extracted_value);
      }
    }
  }
}

// reflecting class and unfolding json
void FromTask::prealloc(const instance& obj2, Value& json_object, FTaskQue& tasks)
{
  instance obj = get_wrapped(obj2);
  const auto prop_list = obj.get_derived_type().get_properties();
  auto fname = obj.get_type().get_name().to_string();
  for (auto prop : prop_list) {
    Value::MemberIterator ret = json_object.FindMember(prop.get_name().data());
    if (ret == json_object.MemberEnd()) {
      continue;
    }
    auto tname = prop.get_type().get_name().to_string();  // debug
    const type value_t = prop.get_type();
    auto& json_value = ret->value;
    auto prop_name = prop.get_name().to_string();
    switch (json_value.GetType()) {
      case kArrayType: {
        variant var = prop.get_value(obj);
        // TODO(): add muti thread
        if (value_t.is_sequential_container()) {
          // sequential
          create_sequential_task(prop_name, var, json_value, tasks);
        } else {
          create_associative_task(prop_name, var, json_value, tasks);
        }
        prop.set_value(obj, var);
        break;
      }
      case kObjectType: {
        variant var = prop.get_value(obj);  // TODO(): removable
        auto prop_task = std::make_shared<FmObjectTask>(prop_name, shared_from_this());

        if (get_wrapped(var.get_type()).is_pointer()) {
          // create_push_task<FmObjectTask>(prop_name, json_value, shared_from_this(), tasks);
          prop_task->parse_id_type(json_value);
        } else {
          prop_task->_json_object.CopyFrom(json_value, prop_task->_json_object.GetAllocator());
          prop_task->_raw_type = get_wrapped(prop.get_type()).get_raw_type().get_name().to_string();
          prop_task->_pvar = std::make_shared<variant>(std::move(var));
          // prop_task->_pvar = std::make_shared<variant>(std::move(var));
          // assert(prop.set_value(obj, var));
        }
        _need++;
        tasks.push(prop_task);
        break;
      }
      default: {
        variant extracted_value = extract_basic_types(json_value);
        auto extt = extracted_value.get_type().get_name().to_string();
        if (extracted_value.convert(value_t) || is_optional(value_t)) {
          assert(prop.set_value(obj, extracted_value));
        }
      }
    }
  }
}

// set own property value to parent, only derived class to use
inline void FmRootTaks::assemble_to_parent()
{
  assemble_to_parent(_parent, _prop_name);
}

// for already task , passing parent`s pvar, this propname
void FmRootTaks::assemble_to_parent(const FTASK_PTR& pparent, const string& prop_name)
{
  pparent->set_value_raw_ptr(_inst.get_object_pointer(), prop_name);
}

inline void FmObjectTask::assemble_to_parent()
{
  // assemble_to_parent(_parent, _prop_name);
  _parent->set_value(_pvar, _prop_name);
}

void FmObjectTask::assemble_to_parent(const FTASK_PTR& pparent, const string& prop_name)
{
  // only root take effect !!
  // pparent->set_value(_pvar, prop_name);
}

void FmSequetialTask::assemble_to_parent()
{
  _parent->set_value(_pvar, _prop_name);
}

// void FmSequetialObjectTask::assemble_to_parent(const FTASK_PTR& pparent, const string&
// prop_name)
// {
//   // TODO(): 这个可能是oject任务调用, 所以不能使用set_i设置
//   // 不用重载了
// }

void FmSequetialObjectTask::assemble_to_parent()
{
  _parent->set_value(_pvar, _index);
}

void FmAssociativeObjectTask::assemble_to_parent()
{
  _parent->set_kv(_pvar, _key);
}

void FmAssociativeTask::assemble_to_parent()
{
  _parent->set_value(_pvar, _prop_name);
}

// check if already restored
bool FromTask::already_restore_object(FTaskQue& tasks, const string& cid)
{
  if (tasks.contains(cid)) {
    _ptask = tasks.get_task(cid);
    return true;
  }
  return false;
}

bool FmSequetialTask::set_value(const shared_ptr<variant>& pvar, int index)
{
  bool ok = true;
  if (pvar) {
    ok = _view.set_value(index, *pvar);
  }
  --_need;
  if (_need == 0) {
    assemble_to_parent();
  }
  return ok;
}

bool FmSequetialTask::set_value_raw_ptr(void* pvar, int index)
{
  // dont need to implemented!!
  assert(false);
}

bool FmRootTaks::set_value(const shared_ptr<variant>& pvar, const string& prop_name)
{
  auto prop = get_wrapped(_inst).get_type().get_property(prop_name);
  auto isv = pvar->get_type();
  auto ptype = prop.get_type();
  auto exvar = get_extracted(pvar, ptype);
  auto ok = prop.set_value(_inst, exvar);
  return ok;
}

bool FmObjectTask::set_value(const shared_ptr<variant>& pvar, const string& prop_name)
{
  instance inst(*_pvar);
  auto prop = get_wrapped(inst).get_type().get_property(prop_name);
  auto ptype = prop.get_type();

  auto exvar = get_extracted(pvar, ptype);
  exvar.convert(prop.get_type());
  auto ok = prop.set_value(inst, exvar);
  --_need;
  if (_need == 0) {
    assemble_to_parent();
  }
  return ok;
}

// provide only for FmObjectTask
bool FmObjectTask::set_value_raw_ptr(void* pvar, const string& prop_name)
{
  instance inst(*_pvar);
  auto prop = get_wrapped(inst).get_type().get_property(prop_name);
  auto ok = prop.set_value_raw_ptr(inst, pvar);
  return ok;
}

bool FmAssociativeTask::set_kv(const shared_ptr<variant>& pvar, const string& key)
{
  auto vtype = _view.get_value_type();
  auto exvar = get_extracted(pvar, vtype);
  auto [_, ok] = _view.insert(key, exvar);
  --_need;
  if (_need == 0) {
    assemble_to_parent();
  }
  return ok;
}

bool FmAssociativeTask::set_kv_raw_ptr(void* pvar, const string& key)
{
  // dont need to implemented!!
  assert(false);
}

void TaskResumer::alloc_all(const instance& inst, const string& cid)
{
  auto root = std::make_shared<FmRootTaks>(inst);
  root->_raw_type = get_wrapped(inst.get_derived_type()).get_raw_type().get_name().to_string();
  root->_cid = cid;
  _tasks.push(root);
  int count = 0;
  while (!_tasks.empty()) {
    ++count;
    auto task = _tasks.pop();
    task->collect(_tasks);
  }
  std::cout << "restored about " << count << " time instance\n" << std::flush;
}

void FromRedis::operator()(const instance& inst, const string& cid)
{
  resumer.alloc_all(inst, cid);
}

// only for root class
void FmRootTaks::collect(FTaskQue& tasks)
{
  tasks.push_dict(shared_from_this(), _cid);
  request_json();
  Document json_object;
  if (json_object.Parse(_json.c_str()).HasParseError()) {
    std::cerr << "error in document.Parse\n";
  }
  prealloc(_inst, json_object, tasks);
}

// restoring for object. aka pointer
void FmObjectTask::collect(FTaskQue& tasks)
{
  if (_json_object.IsNull()) {
    request_json();
    if (!already_restore_object(tasks, _cid)) {
      // push to dict before actual data restoring
      tasks.push_dict(shared_from_this(), _cid);
      Document json_object;
      if (json_object.Parse(_json.c_str()).HasParseError()) {
        std::cerr << "error in document.Parse\n";
      }
      auto var = type::get_by_name(_raw_type).create();
      _pvar = std::make_shared<variant>(std::move(var));
      prealloc(*_pvar, json_object, tasks);
    } else {
      // for only when this var is root, to set with raw ptr
      _ptask->assemble_to_parent(_parent, _prop_name);
      _pvar = _ptask->_pvar;
    }
  } else {
    // auto var = type::get_by_name(_raw_type).create();
    // _pvar = std::make_shared<variant>(std::move(var));
    prealloc(*_pvar, _json_object, tasks);
  }
  if (_need == 0) {
    assemble_to_parent();
  }
}
// TODO(): duplicate code? any other solution?
void FmSequetialObjectTask::collect(FTaskQue& tasks)
{
  if (_json_object.IsNull()) {
    if (!already_restore_object(tasks, _cid)) {
      tasks.push_dict(shared_from_this(), _cid);
      Document json_object;
      if (json_object.Parse(_json.c_str()).HasParseError()) {
        std::cerr << "error in document.Parse\n";
      }
      auto var = type::get_by_name(_raw_type).create();
      _pvar = std::make_shared<variant>(std::move(var));
      prealloc(*_pvar, json_object, tasks);

    } else {
      // bug: not support root
      _pvar = _ptask->_pvar;
    }
  } else {
    prealloc(*_pvar, _json_object, tasks);
  }

  // nullptr? or root?
  assert(_pvar);
  if (_need == 0) {
    assemble_to_parent();
  }
}

void FmSequetialTask::collect(FTaskQue& tasks)
{
  _view = _pvar->create_sequential_view();
  if (_json_object.IsNull()) {
    // pointer
    // TODO(): 先请求, 重复请求了?
    request_json();

    _view.set_size(_jsons.size());
    _need = _jsons.size();
    size_t i = 0;
    for (const auto& json : _jsons) {
      // assert(json != std::nullopt);
      if (json->empty()) {
        auto ok = set_value(nullptr, i);
        continue;
      }
      auto prop_task = std::make_shared<FmSequetialObjectTask>(i, shared_from_this());
      prop_task->_cid = _cids[i];
      prop_task->_raw_type = _types[i];
      prop_task->_json = *json;
      ++i;
      tasks.push(prop_task);
    }
  } else {
    // object
    size_t size = _json_object.Size();
    _view.set_size(size);
    _need = size;
    for (size_t i = 0; i < size; ++i) {
      auto& json_index_value = _json_object[i];
      auto wrapped_value = _view.get_value(i).extract_wrapped_value();
      auto prop_task = std::make_shared<FmSequetialObjectTask>(i, shared_from_this());
      prop_task->_json_object.CopyFrom(json_index_value, prop_task->_json_object.GetAllocator());
      prop_task->_pvar = std::make_shared<variant>(std::move(wrapped_value));
      tasks.push(prop_task);
    }
  }

  // assemble_to_parent();
}

void FmAssociativeTask::collect(FTaskQue& tasks)
{
  request_json();
  _view = _pvar->create_associative_view();
  _need = _jsons.size();
  size_t i = 0;
  for (const auto& json : _jsons) {
    assert(json != std::nullopt);
    auto prop_task = std::make_shared<FmAssociativeObjectTask>(_keys[i], shared_from_this());
    prop_task->_cid = _cids[i];
    prop_task->_raw_type = _types[i];
    prop_task->_json = *json;
    ++i;
    tasks.push(prop_task);
  }
}

// TODO(): duplicate code? any other solution?
void FmAssociativeObjectTask::collect(FTaskQue& tasks)
{
  if (!already_restore_object(tasks, _cid)) {
    tasks.push_dict(shared_from_this(), _cid);
    Document json_object;
    if (json_object.Parse(_json.c_str()).HasParseError()) {
      std::cerr << "error in document.Parse\n";
    }
    auto var = type::get_by_name(_raw_type).create();
    _pvar = std::make_shared<variant>(std::move(var));
    prealloc(*_pvar, json_object, tasks);
  } else {
    // bug: not support root
    _pvar = _ptask->_pvar;
  }
  // nullptr? or root?
  assert(_pvar);
  assemble_to_parent();
}

void FmRootTaks::parse_id_type(Value& json_val)
{
  auto iter = json_val.MemberBegin();
  _raw_type = iter->value.GetString();
  _cid = iter->name.GetString();
}

// set _cid & _raw_type
void FmObjectTask::parse_id_type(Value& json_val)
{
  auto iter = json_val.MemberBegin();
  _raw_type = iter->value.GetString();
  _cid = iter->name.GetString();
}

void FmSequetialTask::parse_id_type(Value& json_val)
{
  // TODO():
  auto iter = json_val.Begin();
  for (; iter != json_val.End(); iter++) {
    if (iter->IsNull()) {
      std::cerr << "parse_id_type failed (empty) in " << _prop_name
                << "index:" << iter - json_val.Begin() << std::endl;
      _cids.emplace_back("");
      _types.emplace_back("");
    } else {
      _cids.emplace_back(iter->MemberBegin()->name.GetString());
      _types.emplace_back(iter->MemberBegin()->value.GetString());
    }
  }
}

void FmAssociativeTask::parse_id_type(Value& json_val)
{
  auto iter = json_val.Begin();
  for (; iter != json_val.End(); iter++) {
    // TODO(): if iter == empty

    auto key_member = iter->FindMember("key");
    _keys.emplace_back(key_member->value.GetString());
    auto value_member = iter->FindMember("value");
    auto id_member = value_member->value.MemberBegin();
    _cids.emplace_back(id_member->name.GetString());
    _types.emplace_back(id_member->value.GetString());
  }
}

void FmRootTaks::request_json()
{
  auto aux = RedisAux::GetRedisAux();
  _json = aux->hget(_raw_type, _cid);
}
void FmSequetialTask::request_json()
{
  auto aux = RedisAux::GetRedisAux();
  _jsons = aux->hget_piped(_types, _cids);
}

void FmObjectTask::request_json()
{
  auto aux = RedisAux::GetRedisAux();
  _json = aux->hget(_raw_type, _cid);
}

void FmAssociativeTask::request_json()
{
  auto aux = RedisAux::GetRedisAux();
  _jsons = aux->hget_piped(_types, _cids);
}

}  // namespace c2redis