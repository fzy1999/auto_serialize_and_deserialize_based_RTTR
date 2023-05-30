#include "batch_from.h"
#include <memory>
#include "common.h"
namespace c2redis {
void TaskResumer::alloc_all(const instance& inst, const ID_TYPE& cid)
{
  auto root = std::make_shared<FromTask>(get_wrapped(inst), "root");
  tasks.push(root);
  while (!tasks.empty()) {
    auto task = tasks.pop();
  }
}

}  // namespace c2redis