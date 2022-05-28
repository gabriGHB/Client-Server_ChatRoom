cmake_minimum_required(VERSION 3.13)

project(DS-Lab-Assignment
        VERSION 0.9
        DESCRIPTION "Main Lab Assignment"
        LANGUAGES C CXX
        )

# set target names

# server app
set(TARGET_SERVER server)

# libraries
set(TARGET_NET_UTIL netUtil)
set(TARGET_DBMS dbms)
set(TARGET_SERVICES services)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 11)
endif()

# library code
add_subdirectory(src)

# executable code
add_subdirectory(app)
