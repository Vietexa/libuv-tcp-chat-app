cmake_minimum_required(VERSION 3.20)
project(chat_app)

include(FetchContent)

FetchContent_Declare(
    libuv
    GIT_REPOSITORY https://github.com/libuv/libuv.git
    GIT_TAG v1.52.1
)

FetchContent_MakeAvailable(libuv)


add_executable(chat_client
               chat_client/main.c
               )


target_link_libraries(chat_client uv)

add_executable(chat_server 
               chat_server/main.c
               chat_server/alloc.c
               chat_server/parser.c)
               
target_link_libraries(chat_server uv)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(chat_client PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_compile_options(chat_server PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)

    target_link_options(chat_client PRIVATE -fsanitize=address,undefined)
    target_link_options(chat_server PRIVATE -fsanitize=address,undefined)
endif()
