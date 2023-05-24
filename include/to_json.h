
#include "myrttr/property.h"
#include "myrttr/registration.h"

#include "myrttr/type"
#include <string>
#include "common.h"

namespace io {
/*!
 * Serialize the given instance to a json encoded string.
 */
c2redis::ID_TYPE to_json(rttr::instance obj);
}  // namespace io

// RTTR_REGISTRATION
// {
//   rttr::registration::class_<io::IdHolder>("IdHolder").property("id", &io::IdHolder::id);
// }