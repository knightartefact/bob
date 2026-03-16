cmake_minimum_required(VERSION 3.20)
project(bob)

set(EXEC_NAME bob)
set(SRC_FILES src/main.c)

add_executable(${EXEC_NAME} ${SRC_FILES})

target_link_libraries(${EXEC_NAME} PUBLIC
    ssl
    crypto
)
