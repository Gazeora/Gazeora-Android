# Host tests for the native client's protocol code (app/src/main/cpp/protocols) and the room
# view's geometry (app/src/main/cpp/room), which have no Android in them. Build with any
# desktop C++17 compiler:
#   cmake -S app/src/test/cpp -B build/host-tests && cmake --build build/host-tests
#   build/host-tests/protocols_test && build/host-tests/room_test
# GoogleTest comes from GitHub, or from a local copy with -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=.
cmake_minimum_required(VERSION 3.22)
project(gazeora_client_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(CLIENT ${CMAKE_CURRENT_SOURCE_DIR}/../../main/cpp)
add_library(gazeora_protocols STATIC
    ${CLIENT}/protocols/annex_b.cpp
    ${CLIENT}/protocols/discovery_reply.cpp
    ${CLIENT}/protocols/frame_assembler.cpp
    ${CLIENT}/protocols/gzf.cpp
    ${CLIENT}/protocols/pointer_message.cpp
    ${CLIENT}/protocols/rtp_depacketizer.cpp
    ${CLIENT}/protocols/rtsp_message.cpp
    ${CLIENT}/protocols/ts_demuxer.cpp
    ${CLIENT}/protocols/utf16.cpp
    ${CLIENT}/protocols/view_info.cpp)
target_include_directories(gazeora_protocols PUBLIC ${CLIENT})

add_library(gazeora_room_math STATIC
    ${CLIENT}/room/head_pose.cpp
    ${CLIENT}/room/room_geometry.cpp)
target_include_directories(gazeora_room_math PUBLIC ${CLIENT})

add_executable(protocols_test protocols_test.cpp)
target_link_libraries(protocols_test PRIVATE gazeora_protocols GTest::gtest_main)
add_executable(room_test room_test.cpp)
target_link_libraries(room_test PRIVATE gazeora_room_math GTest::gtest_main)
if(MINGW)
    target_link_options(protocols_test PRIVATE -static)
    target_link_options(room_test PRIVATE -static)
endif()
