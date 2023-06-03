#pragma once
#include <sw/redis++/redis++.h>
#include <sw/redis++/redis.h>
#include <uuid/uuid.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

using namespace sw::redis;
using std::string;
const string REDIS_ADDRESS = "tcp://127.0.0.1:6380";  // aborted
const int PIPE_MAX = 1000;

class RedisAux {
 public:
  static std::shared_ptr<RedisAux> GetRedisAux(bool with_pipe = true, int db = 0) {
    if (_redis_aux == nullptr) {
      _redis_aux = std::make_shared<RedisAux>(with_pipe, db);
    }
    return _redis_aux;
  };

  // TODO() : fuilure and competition
  string get(const string& key) { return _redis->get(key).value(); }
  bool hset(const string& key, const string& field, const string& value);
  string hget(const string& key, const string& field) { return _redis->hget(key, field).value(); }
  void hset_piped(const string& key, const string& field, const string& value);
  void exec_pipe() { _pipe->exec(); }
  std::vector<OptionalString> hget_piped(const string& key, const std::vector<string>& field);
  // ID_TYPE get_increased_class_key(const string& classname, const string& session);
  RedisAux() = default;

  explicit RedisAux(bool with_pipe = true, int db = 0) {
    ConnectionOptions connection_options;
    connection_options.host = "127.0.0.1";
    connection_options.port = 6380;
    connection_options.db = db;
    _redis = std::make_shared<Redis>(connection_options);
    if (with_pipe) {
      _pipe = std::make_shared<Pipeline>(_redis->pipeline());
    }
  };

  static std::shared_ptr<RedisAux> _redis_aux;
  std::shared_ptr<Pipeline> _pipe = nullptr;
  std::shared_ptr<Redis> _redis = nullptr;
  int pipe_num = 0;
};
