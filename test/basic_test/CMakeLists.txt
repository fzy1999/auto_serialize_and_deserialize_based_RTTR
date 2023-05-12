add_executable(basic_test main.cpp)
target_include_directories(basic_test PUBLIC
                ${PROJECT_SOURCE_DIR}/third_party
                ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(basic_test 
                myrttr_core_lib
                json_serializator)
