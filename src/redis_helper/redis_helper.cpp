#include <sw/redis++/errors.h>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include "redis_helper.h"
std::shared_ptr<RedisAux> RedisAux::_redis_aux = nullptr;

bool RedisAux::hset(const string& key, const string& field, const string& value)
{
  bool ok = false;
  try {
    ok = _redis->hset(key, field, value);

  } catch (const Error& e) {
    std::cerr << "Redis exception: " << e.what() << std::endl;
  }
  return ok;
}

void RedisAux::hset_piped(const string& key, const string& field, const string& value)
{
  if (pipe_num == PIPE_MAX) {
    pipe_num = 0;
    _pipe->exec();
  }
  pipe_num++;
  _pipe->hset(key, field, value);
}

std::vector<OptionalString> RedisAux::hget_piped(const string& key, const std::vector<string>& fields)
{
  auto cur = fields.cbegin();
  auto tail = cur;
  auto cend = fields.cend();
  std::vector<OptionalString> vals;
  while (cur != cend) {
    if (cend - cur < PIPE_MAX) {
      tail = cend;
    } else {
      tail = cur + PIPE_MAX;
    }
    _redis->hmget(key, cur, tail, std::back_inserter(vals));
  }
  return std::move(vals);
}
