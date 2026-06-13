#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#define DEFAULT_PORT 7000
#define DEFAULT_BACKLOG 128

typedef struct clients_t{
uv_tcp_t **clients_ptr;
int client_count;
int client_capacity;
} clients_t;

typedef struct app_ctx_t{
    uv_loop_t *loop;
    struct sockaddr_in addr;
    clients_t clients;
    
} app_ctx_t;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

int realloc_clients(app_ctx_t *ctx){
size_t old_capacity = ctx->clients.client_capacity;
size_t new_capacity = old_capacity * 2;

uv_tcp_t **tmp = realloc(ctx->clients.clients_ptr, sizeof(*ctx->clients.clients_ptr) * new_capacity);

if (tmp){
    ctx->clients.clients_ptr = tmp;
    ctx->clients.client_capacity = new_capacity;

    for (int i = old_capacity; i < new_capacity; i++){
        ctx->clients.clients_ptr[i] = NULL;
    }
    return 0;
}
else{
    fprintf(stderr, "realloc_clients: There was a problem reallocating space\n");
    return 1;
}
}

void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t*) req;
    free(wr->buf.base);
    free(wr);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = (char*) malloc(suggested_size);
    buf->len = suggested_size;
}

void on_close(uv_handle_t* handle) {
   app_ctx_t *app_ctx = handle->data;
    
    for (int i = 0; i < app_ctx->clients.client_capacity; i++) {

        if (app_ctx->clients.clients_ptr[i] == (uv_tcp_t*)handle) {

            app_ctx->clients.clients_ptr[i] = NULL;
            app_ctx->clients.client_count--;

            break;
        }
    }


    free(handle);
}

void echo_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
    free_write_req(req);
}

void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    app_ctx_t *app_context = client->data;

    if (nread > 0) {

        printf("%.*s\n", (int)nread, buf->base);
        fflush(stdout);

        for (int i = 0; i < app_context->clients.client_capacity; i++) {

            uv_tcp_t *peer = app_context->clients.clients_ptr[i];

            if (!peer)
                continue;

            if (peer == (uv_tcp_t*)client)
                continue;

            write_req_t *req = malloc(sizeof(write_req_t));

            req->buf.base = malloc(nread);

            memcpy(req->buf.base, buf->base, nread);

            req->buf.len = nread;

            uv_write(
                (uv_write_t*)req,
                (uv_stream_t*)peer,
                &req->buf,
                1,
                echo_write
            );
        }

        free(buf->base);
        return;
    }

    if (nread < 0) {

        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));

        uv_close((uv_handle_t*)client, on_close);
    }

    free(buf->base);
}

void on_new_connection(uv_stream_t *server, int status) {
    app_ctx_t *app = server->data;

    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        // error!
        return;
    }

    uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
    client->data = server->data;
    uv_tcp_init(app->loop, client);
    if (uv_accept(server, (uv_stream_t*) client) == 0) {

        printf("Client count: %d\n", app->clients.client_count);
        printf("Client capacity: %d\n", app->clients.client_capacity);

        if (app->clients.client_count == app->clients.client_capacity){
            if(realloc_clients(app) != 0){
                uv_close((uv_handle_t *)client, on_close);
                return;
            }
        }

        for (int i = 0; i < app->clients.client_capacity; i++){
            
            if (!app->clients.clients_ptr[i]){
                app->clients.clients_ptr[i] = client;
                app->clients.client_count++;
                break;
            }
        }

        uv_read_start((uv_stream_t*) client, alloc_buffer, echo_read);
    }
    else {
        uv_close((uv_handle_t*) client, on_close);
    }
}

int main(void) {
    app_ctx_t app_ctx = {0};
    app_ctx.clients.clients_ptr = malloc(sizeof(*app_ctx.clients.clients_ptr) * 1);
    app_ctx.clients.client_capacity = 1;

    app_ctx.loop = uv_default_loop();

    uv_tcp_t server;
    server.data = &app_ctx;

    app_ctx.clients.client_count = 0;

    uv_tcp_init(app_ctx.loop, &server);

    uv_ip4_addr("0.0.0.0", DEFAULT_PORT, &app_ctx.addr);

    uv_tcp_bind(&server, (const struct sockaddr*)&app_ctx.addr, 0);
    int r = uv_listen((uv_stream_t*) &server, DEFAULT_BACKLOG, on_new_connection);
    
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return 1;
    }

    int run_ret_value = uv_run(app_ctx.loop, UV_RUN_DEFAULT);

    uv_loop_close(app_ctx.loop);

    return run_ret_value;
}