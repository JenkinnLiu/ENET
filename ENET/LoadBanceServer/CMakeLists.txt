project(LoadBanceSvr)

include_directories(${CMAKE_SOURCE_DIR}/LoadBanceServer)

aux_source_directory(. loadbance_SRC)

add_executable(
    LoadBanceSvr
    ${loadbance_SRC}
)

target_link_libraries(
    LoadBanceSvr
    EdoyunNet
    LoginSvr
)

