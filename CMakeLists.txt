cmake_minimum_required(VERSION 3.20)
project(bob)

set(EXEC_NAME bob)
set(SRC_FILES
    src/main.c
    src/repository.c
)

add_executable(${EXEC_NAME} ${SRC_FILES})

target_include_directories(${EXEC_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${EXEC_NAME} PUBLIC
    ssl
    crypto
)
