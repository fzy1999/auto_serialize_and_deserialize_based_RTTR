#pragma once

#include <cstddef>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include "myrttr/instance.h"
#include "myrttr/type"
#include "common.h"

namespace c2redis {
using rttr::instance;
using std::shared_ptr;
using std::string;
using std::vector;

struct FromTask;
using FTASK_PTR = shared_ptr<FromTask>;
struct Architecture
{
  int level = 0;
  int ceil = 0;
  vector<string> construction;
};

struct FromTask
{
  FromTask(const instance& _inst, string _pname) : inst(_inst), pname(std::move(_pname)) {}
  FTASK_PTR parent = nullptr;
  size_t need = 0;
  instance inst;
  string pname;
  vector<string> cids;
};

struct FTaskQue
{
  std::queue<FTASK_PTR> que;
  void push(const FTASK_PTR& ptr) { que.push(ptr); }
  FTASK_PTR pop()
  {
    auto ptr = que.front();
    que.pop();
    return ptr;
  }
  [[nodiscard]] bool empty() const { return que.empty(); }
};

class TaskResumer
{
 public:
  void alloc_all(const instance& inst, const ID_TYPE& cid);

 private:
  Architecture archi;
  FTaskQue tasks;
};
}  // namespace c2redis
