#pragma once

#include <uv.h>

typedef struct parsing_res_t{
    char *key_positions[10];
    char *value_positions[10];
} parsing_res_t;

typedef struct peer_t{
    uv_tcp_t *client;
    char name[100];
} peer_t;

typedef struct clients_t{
peer_t **peers;
int client_count;
int client_capacity;
} clients_t;

typedef struct app_ctx_t{
    uv_loop_t *loop;
    struct sockaddr_in addr;
    clients_t clients;
    parsing_res_t res;
    
} app_ctx_t;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;