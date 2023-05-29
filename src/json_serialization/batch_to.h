#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>      // rapidjson's DOM-style API
#include <rapidjson/prettywriter.h>  // for stringify JSON

#include "myrttr/instance.h"
#include "myrttr/property.h"
#include "myrttr/type"
#include "common.h"
#include "myrttr/variant.h"
using std::shared_ptr;
using namespace rapidjson;
using namespace rttr;
namespace c2redis {
auto duration = [](auto s, auto e) { return std::chrono::duration_cast<std::chrono::seconds>(e - s).count(); };
struct Task;
using TASK_PTR = std::shared_ptr<Task>;

struct Task
{
  Task() = default;
  explicit Task(instance& _inst) : inst(_inst){};
  explicit Task(const instance& _inst) : inst(_inst){};
  // std::shared_ptr<Task> parent;
  // int need = 0;
  // std::vector<std::string> cids;
  // looks like "{"id":"type"}"
  std::string cid;
  std::string cname;
  instance inst;
  std::string json;

  bool to_json();
  bool store();
};

struct TaskDict
{
 private:
  std::unordered_map<void*, TASK_PTR> _dict;
  std::vector<TASK_PTR> _tasks;
  size_t level = 0;
  size_t walker = 0;
  size_t tail = 0;
  size_t reader = 0;
  size_t keeper = 0;
  const size_t capacity = 45000000;
  class GKeyMutex
  {
    std::mutex _td_mutex;

   public:
    GKeyMutex() { _td_mutex.lock(); }
    ~GKeyMutex() { _td_mutex.unlock(); }
  };
  GKeyMutex _mutex;
  ID_TYPE generate_key(void* ptr);
  void add_task(void* ptr, TASK_PTR task);

 public:
  TaskDict()
  {
    _tasks.reserve(capacity);
    _dict.reserve(capacity);
  }
  TASK_PTR get_next();
  void serialize_all();  // unused!!
  bool has_next() { return walker < _tasks.size(); }
  bool has_key(void* vp) { return _dict.contains(vp); }
  bool has_unread() { return reader < walker; }
  bool has_unstore() { return keeper < reader; }
  TASK_PTR get_next_unstore();
  TASK_PTR get_next_unread();
  TASK_PTR add_key(const instance& inst);
  TASK_PTR get_key(const instance& inst);
  instance get_wrapped(const instance& inst);
  variant get_wrapped(const variant& var);
  type get_wrapped(const type& type);
};

struct StoreQueue
{
  std::queue<std::string_view> store_list;
  const int n_per_run = 100;

  bool run_store();
  void execute_all();  // 合并batch和不合并batch
};

class TaskAllocator
{
 public:
  void allocate_all(rttr::instance& inst, ID_TYPE& cid);
  void serialize_all();  // TODO(): 顺序处理tasks, 这个第二遍已经有所有的key
  void store_all();

 private:
  void serialize_variant(const variant& var, PrettyWriter<StringBuffer>& writer);
  void serialize_instance(const instance& obj, PrettyWriter<StringBuffer>& writer);
  bool serialize_pointer(const variant& var, PrettyWriter<StringBuffer>& writer);
  bool serialize_atomic(const variant& var, PrettyWriter<StringBuffer>& writer);
  void serialize_associative_container(const variant_associative_view&, PrettyWriter<StringBuffer>&);
  void serialize_sequential_container(const variant_sequential_view&, PrettyWriter<StringBuffer>&);
  void allocate_associative_container(const variant_associative_view& view);
  void allocate_sequential_container(const variant_sequential_view& view);
  void allocate_instance(const instance& obj);

  void create_task(const variant& var);
  bool write_optinal_types_to_json(const variant& var, PrettyWriter<StringBuffer>& writer);
  bool is_optional(const type& t) { return t.get_name().to_string().find("optional") != std::string::npos; }
  const int thread_num = 32;

  TaskDict dict;
  StoreQueue queue;
  int level = 0;
  TaskDict task_dict;
  bool alloc_finished = false;
  bool read_finished = false;
};

class ToRedis
{
 public:
  ID_TYPE operator()(instance inst);

 private:
  TaskAllocator allocator;
  void call_alloc(rttr::instance& inst, ID_TYPE& cid);
  void call_serialize_all();
  void call_store();
};
}  // namespace c2redis