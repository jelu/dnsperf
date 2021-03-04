#include <uv.h>
#include <stdlib.h>
#include <stdio.h>

uv_loop_t *loop;
uv_udp_t sock;
size_t _sent = 0, _bytes = 0, _recv = 0, _rbytes = 0;

char recvbuf[64*1024];

static void _alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    buf->base = recvbuf;
    buf->len = sizeof(recvbuf);
}

static void _recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags)
{
    if (nread < 1)
        return;

    _recv++;
    _rbytes += nread;

    if (_recv == 560) {
        uv_stop(loop);
        return;
    }
}

static void _send_cb(uv_udp_send_t* req, int status)
{
    if (status) {
    }

    free(req);

    if (_sent == 560) {
        return;
    }

    req = calloc(1, sizeof(uv_udp_send_t)); // TODO
    char* q = "skjdhfkjsdhfkjhsdjfkhsdsdkfhjskldhjflksdjflksdjlkfjsdkljlk";
    size_t len = 28;
    uv_buf_t sndbuf = uv_buf_init(q, len);
    uv_udp_send(req, &sock, &sndbuf, 1, 0, _send_cb);
    // printf("sent %zu\n", ++_sent);
    _sent++;
    _bytes += len;
}

int main() {
    loop = uv_default_loop();

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    int bufsize = 1024*1024;
    int ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    int flags = fcntl(fd, F_GETFL, 0);
    ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    uv_udp_init(loop, &sock);
    uv_udp_open(&sock, fd);
    struct sockaddr_in local_addr;
    uv_ip4_addr("0.0.0.0", 0, &local_addr);
    uv_udp_bind(&sock, (const struct sockaddr *)&local_addr, UV_UDP_REUSEADDR);
    struct sockaddr_in server_addr;
    uv_ip4_addr("127.0.0.1", 5353, &server_addr);
    uv_udp_connect(&sock, (const struct sockaddr *)&server_addr);


    uv_udp_send_t* req = calloc(1, sizeof(uv_udp_send_t)); // TODO
    char* q = "skjdhfkjsdhfkjhsdjfkhsdsdkfhjskldhjflksdjflksdjlkfjsdkljlk";
    size_t len = 28;
    uv_buf_t sndbuf = uv_buf_init(q, len);
    uv_udp_send(req, &sock, &sndbuf, 1, 0, _send_cb);
    // printf("sent %zu\n", ++_sent);
    _sent++;
    _bytes += len;

    uv_udp_recv_start(&sock, _alloc_cb, _recv_cb);

    uv_run(loop, UV_RUN_DEFAULT);

    printf("sent %zu bytes %zu, recv %zu bytes %zu\n", _sent, _bytes, _recv, _rbytes);
}
