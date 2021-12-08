cmake_minimum_required(VERSION 3.5)

set(Project "betterstore")
project(${Project} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# <------------ add hiredis dependency --------------->
find_path(HIREDIS_HEADER hiredis)
#target_include_directories(target PUBLIC ${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
#target_link_libraries(target ${HIREDIS_LIB})

# <------------ add redis-plus-plus dependency -------------->
# NOTE: this should be *sw* NOT *redis++*
find_path(REDIS_PLUS_PLUS_HEADER sw)
#target_include_directories(target PUBLIC ${REDIS_PLUS_PLUS_HEADER})

find_library(REDIS_PLUS_PLUS_LIB redis++)
#target_link_libraries(target ${REDIS_PLUS_PLUS_LIB})

set(Project_IncludeDirs
    /home/ubuntu/work/libhttpserver/src/
    /home/ubuntu/work/libhttpserver/src/httpserver/
#    /home/ubuntu/work/redis-plus-plus/src/sw/redis++/
        )

    add_executable(${Project} main.cpp)


target_include_directories(${Project}
    PRIVATE ${Project_IncludeDirs}
    PUBLIC ${HIREDIS_HEADER}
    PUBLIC ${REDIS_PLUS_PLUS_HEADER})

target_link_libraries(${Project}
       /home/ubuntu/work/libhttpserver-install/libhttpserver.so.0.19.0
       ${HIREDIS_LIB}
       ${REDIS_PLUS_PLUS_LIB}
        )
