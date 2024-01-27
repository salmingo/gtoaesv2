cmake_minimum_required(VERSION 3.10.0)
project(gtoaesv2 VERSION 2.0)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
    # set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_DEBUG_POSTFIX _d)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/modules)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

message("BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
if (“${CMAKE_BUILD_TYPE}” MATCHES "Debug")
    add_definitions(-DNDEBUG)
endif()

aux_source_directory(src SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

##=============== Library : boost
find_package(boost REQUIRED)
if (BOOST_INC AND BOOST_SYSTEM AND BOOST_THREAD AND BOOST_FILESYSTEM AND BOOST_CHRONO AND BOOST_DATETIME)
    message("include : ${BOOST_INC}")
    message("link : ${BOOST_SYSTEM} ${BOOST_THREAD} ${BOOST_FILESYSTEM} ${BOOST_CHRONO} ${BOOST_DATETIME}")
   include_directories(${BOOST_INC})
    target_link_libraries(${PROJECT_NAME}
        ${BOOST_SYSTEM}
        ${BOOST_THREAD}
        ${BOOST_FILESYSTEM}
        ${BOOST_CHRONO}
        ${BOOST_DATETIME})
else()
    message(FATAL_ERROR " : not found [boost] SDKs")
endif ()

##=============== Library : stdc++, for Linux
message(STATUS "operation system is ${CMAKE_SYSTEM}")
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(${PROJECT_NAME}
        rt
        pthread)
endif ()
target_link_libraries(${PROJECT_NAME} m)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION "/usr/local/bin/")
