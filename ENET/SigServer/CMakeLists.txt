project(SigSvr)

include_directories(${CMAKE_SOURCE_DIR}/SigServer)

aux_source_directory(. Sig_SRC)

add_executable(
    SigSvr
    ${Sig_SRC}
)

target_link_libraries(
    SigSvr
    EdoyunNet
)

