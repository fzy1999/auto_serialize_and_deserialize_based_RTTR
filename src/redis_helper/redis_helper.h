#pragma once
#include <sw/redis++/redis++.h>
#include <sw/redis++/redis.h>
#include <uuid/uuid.h>

#include <cstdint>
#include <memory>
#include <string>

using namespace sw::redis;
using std::string;
const string REDIS_ADDRESS = "tcp://127.0.0.1:6380";
using ID_TYPE = string;

class RedisAux
{
 public:
  static std::shared_ptr<RedisAux> GetRedisAux()
  {
    if (_redis_aux == nullptr) {
      // TODO(sxx): address
      _redis_aux = std::make_shared<RedisAux>(REDIS_ADDRESS);
    }
    return _redis_aux;
  };

  // TODO() : fuilure and competition
  string get(const string& key) { return _redis->get(key).value(); }
  void hset(const string& key, const string& field, const string& value) { _redis->hset(key, field, value); }
  string hget(const string& key, const string& field) { return _redis->hget(key, field).value(); }
  // ID_TYPE get_increased_class_key(const string& classname, const string& session);
  RedisAux() = default;

  explicit RedisAux(const string& addr) { _redis = std::make_shared<Redis>(addr); };
  explicit RedisAux(string&& addr) { _redis = std::make_shared<Redis>(addr); };

  static std::shared_ptr<RedisAux> _redis_aux;
  std::shared_ptr<Redis> _redis;
  const string NUM_SURFIX = "NUM";
};
