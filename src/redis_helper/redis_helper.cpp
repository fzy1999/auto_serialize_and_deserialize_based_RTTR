#include "redis_helper.h"
#include <sw/redis++/errors.h>

#include <iostream>
#include <string>
std::shared_ptr<RedisAux> RedisAux::_redis_aux = nullptr;
// ID_TYPE RedisAux::get_increased_class_key(const string& classname, const string& session)
// {
//   // lock
//   auto id_str = _redis->hget(classname + NUM_SURFIX, session);
//   ID_TYPE id = 2;
//   if (id_str) {
//     id = std::stol(id_str.value()) + 1;
//     // for avoiding status -> 0:error 1:success
//   } else {
//     std::cout << "nil in get_increased_class_key \n";
//   }
//   // TODO(): failure
//   _redis->hset(classname + NUM_SURFIX, session, std::to_string(id));
//   return id;
// }

void RedisAux::hset(const string& key, const string& field, const string& value)
{
  try {
    _redis->hset(key, field, value);

  } catch (const Error& e) {
    std::cerr << "Redis exception: " << e.what() << std::endl;
  }
}
