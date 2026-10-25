cmake_minimum_required(VERSION 3.22.1)
project(gazeora_client CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The protocols, without Android: the host tests (app/src/test/cpp) build these too.
add_library(gazeora_protocols STATIC
    protocols/annex_b.cpp
    protocols/discovery_reply.cpp
    protocols/frame_assembler.cpp
    protocols/gzf.cpp
    protocols/pointer_message.cpp
    protocols/rtp_depacketizer.cpp
    protocols/rtsp_message.cpp
    protocols/ts_demuxer.cpp
    protocols/utf16.cpp
    protocols/view_info.cpp)
target_include_directories(gazeora_protocols PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(gazeora_protocols PRIVATE -Wall -Wextra)

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

add_library(gazeora_client SHARED
    discovery.cpp
    jni.cpp
    net.cpp
    player.cpp
    pointer/pointer_sender_factory.cpp
    pointer/queued_pointer_sender.cpp
    pointer/udp_pointer_sender.cpp
    pointer/usb_pointer_sender.cpp
    shaders/shader_program.cpp
    video_decoder.cpp
    video_renderer.cpp
    sources/http_source.cpp
    sources/rtsp_source.cpp
    sources/udp_source.cpp
    sources/usb_link.cpp)
target_include_directories(gazeora_client PRIVATE ${GENERATED_DIR})
target_compile_options(gazeora_client PRIVATE -Wall -Wextra)
target_link_libraries(gazeora_client PRIVATE gazeora_protocols mediandk android nativewindow
    EGL GLESv2 log)
