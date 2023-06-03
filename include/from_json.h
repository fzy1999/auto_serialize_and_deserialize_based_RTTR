#include <string>
#include <vector>

#include "common.h"
#include "myrttr/type"

namespace io {
/*!
 * Deserialize the given json string \p json to the given instance \p obj.
 */
bool from_json(const std::string& json, rttr::instance obj);

/**
 * @brief Deserialize the given json string key to the given instance \p obj.
 *
 * @param key
 * @param obj
 * @return true
 * @return false
 */
bool from_key(const std::string& key, rttr::instance obj);

bool from_key(const std::string& key, rttr::instance obj, std::vector<std::string>&&);

}  // namespace io
