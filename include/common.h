#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>

#include "myrttr/instance.h"
#define DEBUG 0

namespace c2redis {

using rttr::instance;
using rttr::type;
const size_t VP_LEN = 8;
using ID_TYPE = std::optional<std::string>;
const ID_TYPE NULL_ID = "";

inline long duration(std::chrono::time_point<std::chrono::high_resolution_clock> begin,
                     std::chrono::time_point<std::chrono::high_resolution_clock> end)
{
  return std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
}

struct NullHolder
{
};
struct IdHolder
{
  std::string id;
  std::string derive_type;
};
constexpr void debug_log(int verbose, const std::string& log)
{
  if (verbose <= DEBUG) {
    std::cout << "- " << log << '\n';
  }
}

inline instance get_wrapped(const instance& inst)
{
  return inst.get_type().get_raw_type().is_wrapper() ? inst.get_wrapped_instance() : inst;
}
inline rttr::variant get_wrapped(const rttr::variant& var)
{
  auto value_type = var.get_type();
  auto wrapped_type = value_type.is_wrapper() ? value_type.get_wrapped_type() : value_type;
  bool is_wrapper = wrapped_type != value_type;
  return is_wrapper ? var.extract_wrapped_value() : var;
}

inline type get_wrapped(const type& value_type)
{
  // auto wrapped_type = value_type.is_wrapper() ? value_type.get_wrapped_type() : value_type;
  auto wrapped_type = value_type;
  while (wrapped_type.is_wrapper()) {
    wrapped_type = wrapped_type.get_wrapped_type();
  }
  return wrapped_type;
}
}  // namespace c2redis
