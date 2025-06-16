project(EdoyunNet)

include_directories(${CMAKE_SOURCE_DIR}/EdoyunNet)

aux_source_directory(. NET_SRC)

add_library(
    EdoyunNet
    ${NET_SRC}
)