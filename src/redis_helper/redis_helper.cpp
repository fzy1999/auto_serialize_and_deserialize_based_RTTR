#include <sw/redis++/errors.h>

#include <iostream>
#include <string>
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
