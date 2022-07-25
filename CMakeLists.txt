#   Copyright 2020-2021 github.com/nvevg
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

cmake_minimum_required(VERSION 3.16)

project(schematose)
set(CXX_STANDARD_REQUIRED ON)

add_library(schematose INTERFACE)

set(CMAKE_CXX_STANDARD 20)
target_include_directories(schematose INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/)

set(GTEST_REPO "https://github.com/google/googletest" CACHE STRING "url of GoogleTest repository")

option(DOWNLOAD_GTEST "Download the git repo of GoogleTest, build and use the master branch version" ON)
option(BUILD_TESTS "Build unit testings" ON)
option(TIXML2_BACKEND "Enable TinyXML2 backend")

if(BUILD_TESTS)
    if(DOWNLOAD_GTEST)
        include(FetchContent)
        FetchContent_Declare(googletest
            URL ${GTEST_REPO}/archive/refs/tags/release-1.11.0.zip
        )

        set(BUILD_GMOCK OFF CACHE BOOL "")
        set(INSTALL_GTEST OFF CACHE BOOL "")

        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        FetchContent_MakeAvailable(googletest)

        set(GTEST_LIBS gtest gtest_main)
    else()
        set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
        set(THREADS_PREFER_PTHREAD_FLAG TRUE)

        find_package(Threads REQUIRED)
        find_package(GTest CONFIG REQUIRED)
        message(NOTICE "-- Found GTest: " ${GTest_DIR})

        set(GTEST_LIBS GTest::gtest GTest::gtest_main)
    endif()
    
    enable_testing()

    file(GLOB TESTS test/*.cpp)
    add_executable(schematose_test ${TESTS})

    if (TIXML2_BACKEND)
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
        find_package(TinyXML2)
        message(NOTICE "-- FOUND TinyXML2")
        add_definitions(-DSCHEMATOSE_TIXML2_BACKEND)
    endif()

    target_include_directories(schematose_test PRIVATE ${GTEST_INCLUDE_DIRS} ${TinyXML2_INCLUDE_DIRS})
    target_link_libraries(schematose_test schematose ${CMAKE_THREAD_LIBS_INIT} ${GTEST_LIBS} ${TinyXML2_LIBRARIES})

    include(GoogleTest)
    gtest_discover_tests(schematose_test)
endif()

