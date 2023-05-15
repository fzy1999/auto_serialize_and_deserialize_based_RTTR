# set(RAPID_JSON_DIR ${PROJECT_SOURCE_DIR}/third_party/rapidjson-1.1.0)
# include_directories(${RAPID_JSON_DIR})
file(GLOB CPP_SOURCES "./*.cpp")
file(GLOB HPP_SOURCES "./*.hpp")
# find_package(RTTR CONFIG REQUIRED Core)
add_library(json_serializator ${HPP_SOURCES} from_json.cpp to_json.cpp )
target_link_libraries(json_serializator myrttr_core_lib redis_helper uuid)
target_include_directories(json_serializator PUBLIC 
            # ${RAPID_JSON_DIR} 
            ${PROJECT_SOURCE_DIR}/third_party
            ${PROJECT_SOURCE_DIR}/include
            )

