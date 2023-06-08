#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "common.h"
#include "myrttr/instance.h"
#include "myrttr/type"
#include "myrttr/variant.h"
#include "myrttr/variant_sequential_view.h"
#include "rttr/property.h"
#include "rttr/variant_associative_view.h"
#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>      // rapidjson's DOM-style API
#include <rapidjson/prettywriter.h>  // for stringify JSON

namespace c2redis {
using namespace rapidjson;

using rttr::instance;
using rttr::variant;
using std::shared_ptr;
using std::string;
using std::vector;

struct FTaskQue;
class FromTask;
using FTASK_PTR = shared_ptr<FromTask>;
struct Architecture
{
  int _level = 0;
  int _ceil = 0;
  vector<string> _construction;
};

class FromTask : public std::enable_shared_from_this<FromTask>
{
 public:
  virtual ~FromTask() = default;
  FromTask(string prop_name, FTASK_PTR parent)
      : _parent(std::move(parent)), _prop_name(std::move(prop_name)){};
  FTASK_PTR _parent;
  shared_ptr<variant> _pvar;
  FTASK_PTR _ptask;
  string _prop_name;
  string _raw_type;
  // Value _json_val;
  string _cid;
  string _json;
  size_t _need = 0;
  Document _json_object;

  variant extract_basic_types(Value& json_value);
  virtual void request_json(){};
  virtual void collect(FTaskQue& tasks){};
  void prealloc(const instance& obj2, Value& json_object, FTaskQue& tasks);
  virtual void assemble_to_parent(const FTASK_PTR& pparent, const string& prop_name){};
  virtual void assemble_to_parent(){};
  bool already_restore_object(FTaskQue& tasks, const string& cid);
  virtual void parse_id_type(Value&){};
  virtual bool set_value(const shared_ptr<variant>& pvar, const string& prop_name)
  {
    return false;
  };
  virtual bool set_value_raw_ptr(void* pvar, const string& prop_name) { return false; };
  void create_sequential_task(const string& propname, rttr::variant& var, Value& json_value,
                              FTaskQue& tasks);
  void create_associative_task(const string& propname, rttr::variant& var, Value& json_value,
                               FTaskQue& tasks);
  virtual bool set_value(const shared_ptr<variant>& pvar, int index) { return false; };
  virtual bool set_value_raw_ptr(void* pvar, int index) { return false; };
  virtual bool set_kv(const shared_ptr<variant>& pvar, const string& key) { return false; };
  virtual bool set_kv_raw_ptr(void* pvar, const string& key) { return false; };
  void free_vec(std::vector<string>& vec);
  void free_str(string& str);
  virtual void free_all();

  variant get_extracted(const shared_ptr<variant>& pvar, const rttr::type& ptype)
  {
    auto vart = pvar->get_type();
    if (vart == ptype) {
      return *pvar;
    }
    if (vart.is_wrapper() && ptype.is_wrapper()
        && get_wrapped(vart).is_derived_from(get_wrapped(ptype))) {
      return *pvar;
    }
    variant exvar = *pvar;
    if (vart.is_wrapper()) {
      exvar = pvar->extract_wrapped_value();
    }
    return std::move(exvar);
  }
};

class FmRootTaks : public FromTask
{
 public:
  FmRootTaks(const instance& inst) : FromTask("root", nullptr), _inst(inst){};
  instance _inst;
  void request_json() override;
  void collect(FTaskQue& tasks) override;
  void assemble_to_parent(const FTASK_PTR& pparent, const string& prop_name) override;
  void assemble_to_parent() override;
  void parse_id_type(Value& var) override;
  bool set_value(const shared_ptr<variant>& pvar, const string& prop_name) override;
};

class FmObjectTask : public FromTask
{
 public:
  FmObjectTask(string prop_name, FTASK_PTR parent)
      : FromTask(std::move(prop_name), std::move(parent)){};
  void request_json() override;
  void collect(FTaskQue& tasks) override;
  void parse_id_type(Value&) override;
  void assemble_to_parent(const FTASK_PTR& pparent, const string& prop_name) override;
  void assemble_to_parent() override;
  bool set_value(const shared_ptr<variant>& pvar, const string& prop_name) override;
  bool set_value_raw_ptr(void* pvar, const string& prop_name) override;
};

class FmSequetialObjectTask : public FmObjectTask
{
 public:
  FmSequetialObjectTask(int index, FTASK_PTR parent)
      : FmObjectTask("", std::move(parent)), _index(index){};
  void collect(FTaskQue& tasks) override;
  // void assemble_to_parent(const FTASK_PTR& pparent, const string& prop_name) override;
  void assemble_to_parent() override;

  int _index = -1;
};

class FmAssociativeObjectTask : public FmObjectTask
{
 public:
  FmAssociativeObjectTask(string& key, FTASK_PTR parent)
      : FmObjectTask("", std::move(parent)), _key(key){};
  void collect(FTaskQue& tasks) override;
  void assemble_to_parent() override;

  string _key;
};

class FmSequetialTask : public FromTask
{
 public:
  FmSequetialTask(string prop_name, FTASK_PTR parent)
      : FromTask(std::move(prop_name), std::move(parent)){};
  vector<string> _cids;
  vector<string> _jsons;
  vector<string> _types;
  rttr::variant_sequential_view _view;
  // need how many to be init

  void request_json() override;
  void collect(FTaskQue& tasks) override;
  void parse_id_type(Value& var) override;
  void assemble_to_parent() override;
  bool set_value(const shared_ptr<variant>& pvar, int index) override;
  bool set_value_raw_ptr(void* pvar, int index) override;
  void free_all() override;
};

// TODO(): support only type like <basic, object>
class FmAssociativeTask : public FromTask
{
 public:
  FmAssociativeTask(string prop_name, FTASK_PTR parent)
      : FromTask(std::move(prop_name), std::move(parent)){};
  vector<string> _cids;
  vector<string> _jsons;
  vector<string> _types;
  vector<string> _keys;
  rttr::variant_associative_view _view;

  void request_json() override;
  void collect(FTaskQue& tasks) override;
  void parse_id_type(Value&) override;
  // TODO():
  void assemble_to_parent() override;
  bool set_kv(const shared_ptr<variant>& pvar, const string& key) override;
  bool set_kv_raw_ptr(void* pvar, const string& key) override;
  void free_all() override;
};

struct FTaskQue
{
  using VARIANT_PTR = shared_ptr<variant>;
  std::unordered_map<string, FTASK_PTR> _dict;
  std::queue<FTASK_PTR> _que;

  FTASK_PTR get_task(const string& cid) { return _dict[cid]; }
  VARIANT_PTR get_variant(const string& cid) { return _dict[cid]->_pvar; }
  bool contains(const string& cid) const { return _dict.contains(cid); }
  void push(const FTASK_PTR& ptr, const string& cid)
  {
    _que.push(ptr);
    _dict[cid] = ptr;
  }
  void push(const FTASK_PTR& ptr) { _que.push(ptr); }
  void push_dict(const FTASK_PTR& ptr, const string& cid) { _dict[cid] = ptr; }
  FTASK_PTR pop()
  {
    auto ptr = _que.front();
    _que.pop();
    return ptr;
  }
  [[nodiscard]] bool empty() const { return _que.empty(); }
};

class TaskResumer
{
 public:
  void alloc_all(const instance& inst, const string& cid);
  // void prealloc(const instance &obj2, Value &json_object);
  // template <typename T>
  // void collect(const shared_ptr<T> &task){};
  // template <>
  // void collect(const shared_ptr<FmObjectTask> &task);
  // template <>
  // void collect(const shared_ptr<FmSequetialTask> &task);

 private:
  Architecture _archi;
  FTaskQue _tasks;
};

class FromRedis
{
 public:
  void operator()(const instance& inst, const string& cid);

 private:
  TaskResumer resumer;
};
}  // namespace c2redis
