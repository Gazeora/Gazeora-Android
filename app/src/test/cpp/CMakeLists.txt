# Host tests for the native client's protocol code (app/src/main/cpp/protocols) and the hands'
# geometry and camera pictures (app/src/main/cpp/hands), which have no Android in them. Build with any
# desktop C++17 compiler:
#   cmake -S app/src/test/cpp -B build/host-tests && cmake --build build/host-tests
#   build/host-tests/protocols_test && build/host-tests/hands_test
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

add_library(gazeora_hands_math STATIC
    ${CLIENT}/hands/camera_frame.cpp
    ${CLIENT}/hands/hand_detection.cpp
    ${CLIENT}/hands/hand_follower.cpp
    ${CLIENT}/hands/hand_geometry.cpp
    ${CLIENT}/hands/hand_pointer.cpp
    ${CLIENT}/hands/hand_shapes.cpp
    ${CLIENT}/hands/hand_skeleton.cpp
    ${CLIENT}/hands/hand_tracking.cpp
    ${CLIENT}/hands/light_estimate.cpp
    ${CLIENT}/hands/rigged_hand.cpp
    ${CLIENT}/hands/view_geometry.cpp)
target_include_directories(gazeora_hands_math PUBLIC ${CLIENT})

add_executable(protocols_test protocols_test.cpp)
target_link_libraries(protocols_test PRIVATE gazeora_protocols GTest::gtest_main)
add_executable(hands_test hands_test.cpp)
target_link_libraries(hands_test PRIVATE gazeora_hands_math GTest::gtest_main)
if(MINGW)
    target_link_options(protocols_test PRIVATE -static)
    target_link_options(hands_test PRIVATE -static)
endif()
