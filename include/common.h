#pragma once
#include <iostream>
#include <string>
#define DEBUG 2

namespace io {
using ID_TYPE = std::string;
struct IdHolder
{
  ID_TYPE id;
};
}  // namespace io

constexpr void debug_log(int level, const std::string& log)
{
  if (level <= DEBUG) {
    std::cout << "-- " << log << '\n';
  }
}