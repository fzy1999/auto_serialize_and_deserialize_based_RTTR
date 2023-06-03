#include "batch_from.h"

#include <cassert>
#include <cstdio>
#include <memory>
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

//////////// TODO(): change to constexp
#define CREATE_PUSH_TASK(ctype)                                                                \
  do {                                                                                         \
    auto prop_task = std::make_shared<ctype>(prop.get_name().to_string(), shared_from_this()); \
    prop_task->_json_val = json_value.Move();                                                  \
    prop_task->parse_id_type();                                                                \
    tasks.push(prop_task);                                                                     \
  } while (0)
///////////////////////

void FromTask::prealloc(const instance& obj2, Value& json_object, FTaskQue& tasks)
{
  instance obj = get_wrapped(obj2);
  const auto prop_list = obj.get_derived_type().get_properties();
  auto tname = obj.get_type().get_name().to_string();
  for (auto prop : prop_list) {
    Value::MemberIterator ret = json_object.FindMember(prop.get_name().data());
    if (ret == json_object.MemberEnd()) {
      continue;
    }
    auto tname = prop.get_type().get_name().to_string();  // debug
    const type value_t = prop.get_type();
    auto& json_value = ret->value;
    switch (json_value.GetType()) {
      case kArrayType: {
        variant var = prop.get_value(obj);
        // TODO(): add muti thread
        if (value_t.is_sequential_container()) {
          CREATE_PUSH_TASK(FmSequetialTask);
        } else {
          // TODO():
          // auto associative_view = var.create_associative_view();
          CREATE_PUSH_TASK(FmAssociativeTask);
        }
        break;
      }
      case kObjectType: {
        variant var = prop.get_value(obj);
        if (get_wrapped(var.get_type()).is_pointer()) {
          CREATE_PUSH_TASK(FmObjectTask);
          // auto prop_task = std ::make_shared<FmObjectTask>(prop.get_name().to_string(), obj);
          // prop_task->_pvar = std ::make_shared<variant>(std ::move(var));
          // // json_value.Swap(prop_task->_json_val);
          // prop_task->_json_val = json_value.Move();
          // prop_task->parse_id_type();
          // tasks.push(prop_task);
        } else {
          prealloc(var, json_value, tasks);
          prop.set_value(obj, var);
        }
        break;
      }
      default: {
        variant extracted_value = extract_basic_types(json_value);
        if (extracted_value.is_valid()) {
          // TODO(): may cause error for wrong type ?
          prop.set_value(obj, extracted_value);
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
  assemble_to_parent(_parent, _prop_name);
}

void FmObjectTask::assemble_to_parent(const FTASK_PTR& pparent, const string& prop_name)
{
  pparent->set_value(_pvar, prop_name);
}

// check if already restored
bool FromTask::already_restore_object(FTaskQue& tasks, const string& cid)
{
  // TODO()
  if (tasks.contains(cid)) {
    // _pvar = tasks.get_task(cid)->_pvar;
    _ptask = tasks.get_task(cid);
    return true;
  }
  return false;
}

bool FmRootTaks::set_value(const shared_ptr<variant>& pvar, const string& prop_name)
{
  auto prop = get_wrapped(_inst).get_type().get_property(prop_name);
  auto isv = pvar->get_type();
  // TODO(): is copy?? lower class`s restore can actually take effect?
  auto ok = prop.set_value(_inst, pvar->extract_wrapped_value());
  return ok;
}

bool FmObjectTask::set_value(const shared_ptr<variant>& pvar, const string& prop_name)
{
  instance inst(*_pvar);
  auto prop = get_wrapped(inst).get_type().get_property(prop_name);
  auto isv = pvar->get_type();
  auto ok = prop.set_value(inst, pvar->extract_wrapped_value());
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

void TaskResumer::alloc_all(const instance& inst, const string& cid)
{
  auto root = std::make_shared<FmRootTaks>(inst);
  root->_raw_type = get_wrapped(inst.get_derived_type()).get_raw_type().get_name().to_string();
  root->_cid = cid;
  _tasks.push(root);

  while (!_tasks.empty()) {
    auto task = _tasks.pop();
    task->collect(_tasks);
  }
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
    assemble_to_parent();
  } else {
    _ptask->assemble_to_parent(_parent, _prop_name);
  }
}

void FmRootTaks::parse_id_type()
{
  auto iter = _json_val.MemberBegin();
  _raw_type = iter->value.GetString();
  _cid = iter->name.GetString();
}

// set _cid & _raw_type
void FmObjectTask::parse_id_type()
{
  auto iter = _json_val.MemberBegin();
  _raw_type = iter->value.GetString();
  _cid = iter->name.GetString();
}

void FmSequetialTask::collect(FTaskQue& tasks)
{
  // TODO()
}

void FmSequetialTask::parse_id_type()
{
  // TODO():
  // auto iter = _json_val.Begin();
  // for (; iter != _json_val.End(); iter++) {
  //   _cids.emplace_back(iter->MemberBegin()->name.GetString());
  //   _types.emplace_back(iter->MemberBegin()->value.GetString());
  // }
}

void FmAssociativeTask::request_json()
{
  // TODO()
}

void FmAssociativeTask::collect(FTaskQue& tasks)
{
  // TODO()
}

void FmAssociativeTask::parse_id_type()
{
  auto iter = _json_val.Begin();
  for (; iter != _json_val.End(); iter++) {
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
  _jsons = aux->hget_piped(_raw_type, _cids);
}

void FmObjectTask::request_json()
{
  auto aux = RedisAux::GetRedisAux();
  _json = aux->hget(_raw_type, _cid);
}

}  // namespace c2redis