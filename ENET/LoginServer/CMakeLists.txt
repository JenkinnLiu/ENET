project(LoginSvr)

include_directories(${CMAKE_SOURCE_DIR}/LoginServer)

aux_source_directory(. DataBase_SRC)

add_library(
    LoginSvr
    ${DataBase_SRC}
)


target_link_libraries(
    LoginSvr
    mysqlclient
    EdoyunNet
)
