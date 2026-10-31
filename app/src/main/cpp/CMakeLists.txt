# Copyright (C) 2026 Nithin Aashik
# SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

cmake_minimum_required(VERSION 3.22.1)
project(gazeora_client CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The protocols, without Android: the host tests (app/src/test/cpp) build these too.
add_library(gazeora_protocols STATIC
    protocols/annex_b.cpp
    protocols/camera_status_message.cpp
    protocols/discovery_reply.cpp
    protocols/frame_assembler.cpp
    protocols/gzf.cpp
    protocols/head_pose_message.cpp
    protocols/pointer_message.cpp
    protocols/rtp_depacketizer.cpp
    protocols/rtsp_message.cpp
    protocols/ts_demuxer.cpp
    protocols/utf16.cpp
    protocols/view_info.cpp)
target_include_directories(gazeora_protocols PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(gazeora_protocols PRIVATE -Wall -Wextra)

# The hands' geometry and camera pictures, also without Android, for the host tests.
add_library(gazeora_hands_math STATIC
    hands/camera_frame.cpp
    hands/camera_light_gate.cpp
    hands/camera_status_reporter.cpp
    hands/hand_detection.cpp
    hands/hand_follower.cpp
    hands/hand_geometry.cpp
    hands/hand_pointer.cpp
    hands/hand_shapes.cpp
    hands/hand_skeleton.cpp
    hands/hand_tracking.cpp
    hands/light_estimate.cpp
    hands/rigged_hand.cpp
    hands/view_geometry.cpp)
target_include_directories(gazeora_hands_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(gazeora_hands_math PRIVATE -Wall -Wextra)

# The head's orientation math, also without Android, for the host tests.
add_library(gazeora_head_math STATIC
    head/head_orientation.cpp
    head/quaternion.cpp)
target_include_directories(gazeora_head_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(gazeora_head_math PRIVATE -Wall -Wextra)

# The shaders (shaders/*.vert, *.frag), as C++ strings in generated headers: shaders/<file>.h.
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
function(embed_shader SHADER_FILE SHADER_NAME)
    set(path ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER_FILE})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${path})  # edits rebuild
    file(READ ${path} SHADER_SOURCE)
    configure_file(shaders/shader_source.h.in ${GENERATED_DIR}/shaders/${SHADER_FILE}.h @ONLY)
endfunction()
embed_shader(video.vert kVideoVertexShader)
embed_shader(video.frag kVideoFragmentShader)
embed_shader(hand.vert kHandVertexShader)
embed_shader(hand.frag kHandFragmentShader)

# LiteRT, for the hand tracker's models: its C API's headers and libraries, which the Gradle
# build takes out of its AARs (app/build.gradle.kts: extractLiteRt) and packages too.
set(LITERT_DIR "" CACHE PATH "LiteRT's headers/ and jni/, from its AARs")
add_library(litert SHARED IMPORTED)
set_target_properties(litert PROPERTIES
    IMPORTED_LOCATION ${LITERT_DIR}/jni/${ANDROID_ABI}/libtensorflowlite_jni.so
    INTERFACE_INCLUDE_DIRECTORIES ${LITERT_DIR}/headers)
add_library(litert_gpu SHARED IMPORTED)
set_target_properties(litert_gpu PROPERTIES
    IMPORTED_LOCATION ${LITERT_DIR}/jni/${ANDROID_ABI}/libtensorflowlite_gpu_jni.so)

add_library(gazeora_client SHARED
    discovery.cpp
    gl/egl_procs.cpp
    gl/hardware_buffer_textures.cpp
    gl/image_releaser.cpp
    hands/camera_feed.cpp
    hands/camera_setup.cpp
    hands/camera_status_sender.cpp
    hands/hand_overlay.cpp
    hands/hand_models.cpp
    hands/hand_renderer.cpp
    hands/hand_tracker.cpp
    head/head_pose_sender.cpp
    head/head_tracker.cpp
    head/rotation_sensor.cpp
    jni.cpp
    links/message_link_factory.cpp
    links/udp_message_link.cpp
    links/usb_message_link.cpp
    net.cpp
    player.cpp
    pointer/link_pointer_sender.cpp
    pointer/queued_pointer_sender.cpp
    shaders/shader_program.cpp
    video_decoder.cpp
    video_renderer.cpp
    sources/http_source.cpp
    sources/rtsp_source.cpp
    sources/udp_source.cpp
    sources/usb_link.cpp)
target_include_directories(gazeora_client PRIVATE ${GENERATED_DIR})
target_compile_options(gazeora_client PRIVATE -Wall -Wextra)
target_link_libraries(gazeora_client PRIVATE gazeora_protocols gazeora_hands_math gazeora_head_math litert
    litert_gpu mediandk camera2ndk android nativewindow EGL GLESv2 log)
