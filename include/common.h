#pragma once
#include <array>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#define DEBUG 0

namespace c2redis {
const size_t VP_LEN = 8;
using ID_TYPE = std::optional<std::string>;
const ID_TYPE NULL_ID = "";
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
}  // namespace c2redis
