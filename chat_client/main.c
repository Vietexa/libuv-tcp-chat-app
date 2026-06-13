#include "uv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv/unix.h>

typedef struct message_buffer_t{
char *buffer;
int length;
} message_buffer_t;

typedef struct username_t {
    char *name;
    int length;
} username_t;

typedef struct app_ctx{
username_t username;
message_buffer_t msg_buf;
message_buffer_t input_buf;
uv_tcp_t *tcp;
uv_tty_t *tty;
} app_ctx;


void on_write_heap(uv_write_t *req, int status) {

    if (status < 0){
         fprintf(stderr, "write error: %s\n", uv_strerror(status));
    }

    free(req->data);
    free(req);
}

void on_close(uv_handle_t *handle) {
    free(handle);
}

void stdin_read(uv_stream_t *stdin_stream, ssize_t nread, const uv_buf_t *buf) {
     app_ctx *ctx = stdin_stream->data;

    if (nread > 0) {
        for (int i = 0; i < nread; i++) {
            char c = buf->base[i];

            // Enter
            if (c == '\r' || c == '\n') {
                if (ctx->input_buf.length > 0) {
                    int len = ctx->input_buf.length;

                    char *message = malloc(len);
                    if (!message) continue;

                    memcpy(message, ctx->input_buf.buffer, len);

                    uv_write_t *req = malloc(sizeof(*req));
                    if (!req) {
                        free(message);
                        continue;
                    }

                    req->data = message;

                    uv_buf_t out = uv_buf_init(message, len);

                    int r = uv_write(
                        req,
                        (uv_stream_t *)ctx->tcp,
                        &out,
                        1,
                        on_write_heap
                    );

                    if (r < 0) {
                        fprintf(stderr, "write error: %s\n", uv_strerror(r));
                        free(message);
                        free(req);
                    }

                    ctx->input_buf.length = 0;
                }

                printf("\n> ");
                fflush(stdout);
            }

            // Backspace / Delete
            else if (c == 127 || c == 8) {
                if (ctx->input_buf.length > 0) {
                    ctx->input_buf.length--;

                    // erase one char visually
                    printf("\b \b");
                    fflush(stdout);
                }
            }

            // Ctrl+C
            else if (c == 3) {
                uv_tty_reset_mode();
                exit(0);
            }

            // Printable chars
            else if (c >= 32 && c <= 126) {
                if (ctx->input_buf.length < 499) {
                    ctx->input_buf.buffer[ctx->input_buf.length++] = c;

                    // echo typed char
                    putchar(c);
                    fflush(stdout);
                }
            }
        }
    }

    free(buf->base);
}

void echo_read(uv_stream_t *server, ssize_t nread, const uv_buf_t* buf) {
    app_ctx *ctx = server->data;
   if (nread > 0) {
        printf("\r\033[2K");
        printf("result: %.*s\n", (int)nread, buf->base);
        printf("> ");
        
        fwrite(
            ctx->input_buf.buffer,
            1,
            ctx->input_buf.length,
            stdout
        );

        fflush(stdout);

    } else if (nread < 0) {
        if (nread == UV_EOF) {
            fprintf(stderr, "server closed connection\n");
        } else {
            fprintf(stderr, "read error: %s\n", uv_strerror((int)nread));
        }

        uv_close((uv_handle_t *)server, on_close);
        free(buf->base);
        return;
    }

    free(buf->base);
    
}

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

void on_write_end(uv_write_t *req, int status) {
    if (status < 0) {
        fprintf(stderr, "write error: %s\n", uv_strerror(status));
        free(req);
        return;
    }
    free(req);
}

void on_connect(uv_connect_t *req, int status) {
    app_ctx *ctx = req->data;

    if (status < 0) {
        fprintf(stderr, "Error connecting!\n");
        uv_close((uv_handle_t *)req->handle, on_close);
        free(req);
        return;
    }

    ctx->tcp = (uv_tcp_t *)req->handle;

    snprintf(ctx->msg_buf.buffer,
             200,
             "name|%s\n",
             ctx->username.name);

    uv_buf_t buffer = uv_buf_init(ctx->msg_buf.buffer,
                                  strlen(ctx->msg_buf.buffer));

    uv_stream_t *tcp = req->handle;
    uv_write_t *write_request = malloc(sizeof(*write_request));

    uv_write(write_request, tcp, &buffer, 1, on_write_end);

    printf("> ");
    fflush(stdout);

    ctx->tty->data = ctx;
    tcp->data = ctx;

    uv_read_start(tcp, alloc_buffer, echo_read);
    uv_read_start((uv_stream_t *)ctx->tty, alloc_buffer, stdin_read);

    free(req);
}

int main(void){

app_ctx ctx;

ctx.username.name = malloc(200);
ctx.username.length = 200;

ctx.msg_buf.buffer = malloc(500);
ctx.msg_buf.length = 0;

ctx.input_buf.buffer = malloc(500);
ctx.input_buf.length = 0;

printf("Enter username:\n");

if (fgets(ctx.username.name, ctx.username.length, stdin) == NULL){
    fprintf(stderr, "Error geting username string\n");
    return 1;
}

ctx.username.name[strcspn(ctx.username.name, "\n")] = '\0';

uv_loop_t *loop = uv_default_loop();
uv_tcp_t *socket = malloc(sizeof(uv_tcp_t));
uv_tcp_init(loop, socket);

uv_tty_t *tty = malloc(sizeof(uv_tty_t));
uv_tty_init(loop, tty, 0, 1); 
uv_tty_set_mode(tty, UV_TTY_MODE_RAW);

ctx.tcp = socket;
ctx.tty = tty;

uv_connect_t *connect = malloc(sizeof(uv_connect_t));
connect->data = &ctx;

struct sockaddr_in dest;
uv_ip4_addr("127.0.0.1", 7000, &dest);

uv_tcp_connect(connect, socket, (const struct sockaddr*)&dest, on_connect);

uv_run(loop, UV_RUN_DEFAULT);


}