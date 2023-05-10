aux_source_directory(. SRC)
add_library(redis_helper ${SRC})


#<------------ add hiredis dependency here --------------->
find_path(HIREDIS_HEADER hiredis)
target_include_directories(redis_helper PUBLIC ${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
target_link_libraries(redis_helper ${HIREDIS_LIB})

# <------------ add redis-plus-plus dependency here -------------->
find_path(REDIS_PLUS_PLUS_HEADER sw)      # NOTE: this should be *sw* NOT *redis++*
target_include_directories(redis_helper PUBLIC ${REDIS_PLUS_PLUS_HEADER})

find_library(REDIS_PLUS_PLUS_LIB redis++)
target_link_libraries(redis_helper ${REDIS_PLUS_PLUS_LIB} pthread)