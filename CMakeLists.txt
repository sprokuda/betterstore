cmake_minimum_required(VERSION 3.5)

set(Project "betterstore")
project(${Project} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Project_IncludeDirs
    /home/ubuntu/work/libhttpserver/src/
    /home/ubuntu/work/libhttpserver/src/httpserver/
        )

    add_executable(${Project} main.cpp)


target_include_directories(${Project} PRIVATE ${Project_IncludeDirs})
target_link_libraries(${Project}
       /home/ubuntu/work/libhttpserver-install/libhttpserver.so.0.19.0
        )
