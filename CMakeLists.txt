cmake_minimum_required(VERSION 3.20)
project(bob)

add_subdirectory(${CMAKE_SOURCE_DIR}/libs)

set(EXEC_NAME bob)
set(SRC_FILES
    src/main.c
    src/repository.c
    src/object.c
    src/commands.c
    src/index.c
    src/utils.c
    src/tree.c
    src/config.c
    src/commit.c
    src/worktree.c
    src/status.c
    src/checkout.c
    src/diff.c
)

add_executable(${EXEC_NAME} ${SRC_FILES})

target_include_directories(${EXEC_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${EXEC_NAME} PUBLIC
    ssl
    crypto
    str
)
