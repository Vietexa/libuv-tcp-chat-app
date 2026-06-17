cmake_minimum_required(VERSION 4.3.3)
project(chat_app)

add_executable(chat_client
               chat_client/main.c
               )

target_link_libraries(chat_client uv)

add_executable(chat_server 
               chat_server/main.c
               chat_server/alloc.c
               chat_server/parser.c)
               
target_link_libraries(chat_server uv)
