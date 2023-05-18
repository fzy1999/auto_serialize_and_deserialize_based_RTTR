#pragma once
#include <iostream>
#include <string>
#define DEBUG 0

namespace io {
using ID_TYPE = std::string;
struct NullHolder
{
};
struct IdHolder
{
  ID_TYPE id;
  std::string derive_type;
};
}  // namespace io

constexpr void debug_log(int verbose, const std::string& log)
{
  if (verbose <= DEBUG) {
    std::cout << "- " << log << '\n';
  }
}