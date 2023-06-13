#include <sw/redis++/errors.h>

#include <cassert>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
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

std::vector<string> RedisAux::hget_piped(const string& key, const std::vector<string>& fields)
{
  auto cur = fields.cbegin();
  auto tail = cur;
  auto cend = fields.cend();
  std::vector<string> vals;
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

void RedisAux::hget_piped(const string& key, const string& field, std::vector<string>& results)
{
  if (pipe_num == PIPE_MAX) {
    pipe_num = 0;
    auto replies = _pipe->exec();
    for (int i = 0; i < replies.size(); ++i) {
      results.emplace_back(replies.get<string>(i));
    }
  }
  pipe_num++;
  if (key.empty() || field.empty()) {
    _pipe->hget(NULL_KEY, NULL_KEY);
  } else {
    _pipe->hget(key, field);
  }
}

std::vector<string> RedisAux::hget_piped(const std::vector<string>& keys,
                                         const std::vector<string>& fields)
{
  PipeReadLocker _lock(_pipe_read_mutex);
  auto size = keys.size();
  assert(size == fields.size());
  std::vector<string> vals;
  vals.reserve(size);
  for (int i = 0; i < size; ++i) {
    hget_piped(keys[i], fields[i], vals);
  }
  auto replies = _pipe->exec();
  for (int i = 0; i < replies.size(); ++i) {
    vals.emplace_back(replies.get<string>(i));
  }
  return std::move(vals);
}
