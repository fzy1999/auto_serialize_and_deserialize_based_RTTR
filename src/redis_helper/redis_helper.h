#pragma once
#include <sw/redis++/redis++.h>
#include <sw/redis++/redis.h>
#include <uuid/uuid.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

using namespace sw::redis;
using std::string;
const string REDIS_ADDRESS = "tcp://127.0.0.1:6380";  // aborted
const int PIPE_MAX = 1000;
const string NULL_KEY = "null";
class RedisAux
{
 public:
  static std::shared_ptr<RedisAux> GetRedisAux(bool with_pipe = true, int db = 0)
  {
    if (_redis_aux == nullptr) {
      _redis_aux = std::make_shared<RedisAux>(with_pipe, db);
      init_null_key();
    }

    return _redis_aux;
  };

  // TODO() : fuilure and competition
  static void init_null_key() { _redis_aux->hset(NULL_KEY, NULL_KEY, ""); }
  string get(const string& key) { return _redis->get(key).value(); }
  bool hset(const string& key, const string& field, const string& value);
  string hget(const string& key, const string& field) { return _redis->hget(key, field).value(); }
  void hset_piped(const string& key, const string& field, const string& value);
  void exec_pipe() { _pipe->exec(); }
  std::vector<string> hget_piped(const string& key, const std::vector<string>& field);
  void hget_piped(const string& key, const string& field, std::vector<string>& results);
  std::vector<string> hget_piped(const std::vector<string>& keys,
                                 const std::vector<string>& fields);
  // ID_TYPE get_increased_class_key(const string& classname, const string& session);
  RedisAux() = default;

  explicit RedisAux(bool with_pipe = true, int db = 0)
  {
    ConnectionOptions connection_options;
    connection_options.host = "127.0.0.1";
    connection_options.port = 6380;
    connection_options.db = db;
    _redis = std::make_shared<Redis>(connection_options);
    if (with_pipe) {
      _pipe = std::make_shared<Pipeline>(_redis->pipeline());
    }
  };
  class PipeReadLocker
  {
   public:
    explicit PipeReadLocker(std::mutex& mutex) : m_mutex(mutex) { m_mutex.lock(); }

    ~PipeReadLocker() { m_mutex.unlock(); }

   private:
    std::mutex& m_mutex;
  };
  static std::shared_ptr<RedisAux> _redis_aux;
  std::shared_ptr<Pipeline> _pipe = nullptr;
  std::shared_ptr<Redis> _redis = nullptr;
  std::mutex _pipe_read_mutex;
  // TODO(): 加锁, 避免交叉读写
  int pipe_num = 0;
};
