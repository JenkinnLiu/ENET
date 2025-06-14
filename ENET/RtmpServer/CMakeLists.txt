project(RtmpSvr)

include_directories(${CMAKE_SOURCE_DIR}/RtmpServer)

aux_source_directory(. RTMP_SRC)

add_executable(
    RtmpSvr
    ${RTMP_SRC}
)

target_link_libraries(
    RtmpSvr
    EdoyunNet
)