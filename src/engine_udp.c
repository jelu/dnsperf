#include "config.h"

#include "engine_udp.h"
#include "log.h"

#include <assert.h>

typedef struct perf_udp_socket {
    perf_socket_t base;
    uv_udp_t handle;
    uv_idle_t idle;
    char recvbuf[64*1024];
} perf_udp_socket_t;

// static size_t _sent = 0;
// static size_t _recv = 0;
//
// static void _idle(uv_idle_t* handle);
//
// static void _alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
// {
//     perf_udp_socket_t* sock = (perf_udp_socket_t*)handle->data;
//
//     buf->base = sock->recvbuf;
//     buf->len = sizeof(sock->recvbuf);
//     // buf->base = malloc(suggested_size);
//     // buf->len = suggested_size;
// }
//
// static void _recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags)
// {
//     perf_udp_socket_t* sock = (perf_udp_socket_t*)handle->data;
//
//     if (nread < 0) {
//         perf_log_fatal("_recv_cb !nread: %s", uv_err_name(nread));
//         // uv_udp_recv_stop(handle);
//         // uv_close((uv_handle_t*)handle, _uv_close_cb);
//         // return;
//     }
//     if (nread == 0) {
//         // uv_udp_recv_stop(handle);
//         // uv_idle_start(&sock->idle, _idle);
//
//         if (addr) { // empty datagram
//             // printf("empty datagram\n");
//             return;
//         }
//         // printf("zero\n");
//         return;
//         // perf_log_fatal("_recv_cb() conn closed");
//     }
//
//     _recv++;
//
//     // printf("recv %zd %zu\n", nread, _recv);
//
//     if (_recv == 560) {
//         perf_engine_stop(sock->base.engine);
//         printf("sent/recv done\n");
//         uv_udp_recv_stop(handle);
//     }
//
//     // void* r = malloc(nread);
//     // memcpy(r, sock->recvbuf, nread);
//     //
//     // if (perf_engine_enqueue_response(sock->base.engine, r, nread)) {
//     //     perf_log_fatal("perf_engine_enqueue_response()");
//     // }
//     // printf("enqueue r %p\n", r);
//
//     // uv_udp_recv_stop(handle);
//
//     // uv_udp_recv_stop(handle);
//     // uv_idle_start(&sock->idle, _idle);
// }
//
// static void _send_cb(uv_udp_send_t* req, int status)
// {
//     perf_udp_socket_t* sock = (perf_udp_socket_t*)req->handle->data;
//
//     // TODO status
//     if (status) {
//         perf_log_fatal("_send_cb() status %d", status);
//     }
//
//     // uv_udp_recv_start(&sock->handle, _alloc_cb, _recv_cb);
//
//     // if (uv_udp_recv_start(req->handle, _alloc_cb, _recv_cb)) {
//     //     perf_log_fatal("uv_udp_recv_start()");
//     // //     uv_close((uv_handle_t*)req->handle, _uv_close_cb);
//     // }
//
//     // _buf_add(req->data);
//     // _req_add(req);
//
//     // printf("sent %p\n", req->data);
//
//     // free(req->data);
//     free(req);
//
//     uv_idle_start(&sock->idle, _idle);
//
//     return;
//
//     void* q;
//     size_t len;
//
//     perf_engine_dequeue_query(sock->base.engine, &q, &len);
//
//     if (!q) {
//         // free(req);
//
//         uv_idle_start(&sock->idle, _idle);
//         return;
//     }
//
//     req = malloc(sizeof(uv_udp_send_t)); // TODO
//     if (!req) {
//         perf_log_fatal("malloc(uv_udp_send_t)");
//     }
//     req->data = q;
//
//     uv_buf_t sndbuf = uv_buf_init(q, len);
//     if (uv_udp_send(req, &sock->handle, &sndbuf, 1, &sock->base.server.sa.sa, _send_cb)) {
//         perf_log_fatal("uv_udp_send()");
//     //     uv_close((uv_handle_t*)handle, _uv_close_cb);
//     //     _req_add(req);
//     //     _buf_add(buf->base);
//     //     return;
//     }
//     // printf("sending %p %zu\n", q, len);
//     _sent++;
//     printf("sent %zu\n", _sent);
// }
//
// static void _send_cb2(uv_udp_send_t* req, int status)
// {
//     perf_udp_socket_t* sock = (perf_udp_socket_t*)req->handle->data;
//
//     // TODO status
//     if (status) {
//         perf_log_fatal("_send_cb() status %d", status);
//     }
//
//     free(req);
//
//     if (_sent == 560) {
//         // perf_engine_stop(sock->base.engine);
//         return;
//     }
//
//
//     req = calloc(1, sizeof(uv_udp_send_t)); // TODO
//     if (!req) {
//         perf_log_fatal("malloc(uv_udp_send_t)");
//     }
//     char* q = "skjdhfkjsdhfkjhsdjfkhsdsdkfhjskldhjflksdjflksdjlkfjsdkljlk";
//     size_t len = 28;
//     uv_buf_t sndbuf = uv_buf_init(q, len);
//     if (uv_udp_send(req, &sock->handle, &sndbuf, 1, 0/*&sock->base.server.sa.sa*/, _send_cb2)) {
//         perf_log_fatal("uv_udp_send()");
//     }
//     _sent++;
//     // printf("sent %zu\n", _sent);
// }
//
// static void _idle(uv_idle_t* handle)
// {
//     perf_udp_socket_t* sock = (perf_udp_socket_t*)handle->data;
//
//     // uv_udp_recv_start(&sock->handle, _alloc_cb, _recv_cb);
//
//     // void* q;
//     // size_t len;
//     //
//     // perf_engine_dequeue_query(sock->base.engine, &q, &len);
//     //
//     // if (!q) {
//     //     // if (_sent == 560) {
//     //     //     uv_idle_stop(handle);
//     //     //
//     //     // }
//     //     // uv_udp_recv_start(&sock->handle, _alloc_cb, _recv_cb);
//     //     return;
//     // }
//
// //     uv_buf_t sndbuf2 = uv_buf_init(q, len);
// //     for (;;) {
// //         int err = uv_udp_try_send(&sock->handle, &sndbuf2, 1, 0);//&sock->base.server.sa.sa);
// //         if (err < 0) {
// //             if (err == UV_EAGAIN) {
// //                 continue;
// //             }
// //             perf_log_fatal("uv_udp_try_send()");
// //         }
// //         break;
// //     //     uv_close((uv_handle_t*)handle, _uv_close_cb);
// //     //     _req_add(req);
// //     //     _buf_add(buf->base);
// //     //     return;
// //     }
// //     printf("sending %p %zu\n", q, len);
// //     printf("sent %zu\n", ++_sent);
// //     printf("queue %zu %zu\n", uv_udp_get_send_queue_size(&sock->handle), uv_udp_get_send_queue_count(&sock->handle));
// //
// //     // uv_udp_recv_start(&sock->handle, _alloc_cb, _recv_cb);
// //
// // return;
//
//
//
//
//     // printf("dequeue q %p\n", q);
//     // while (perf_engine_enqueue_response(sock->base.engine, q, len)) {}
//     // printf("enqueue r %p\n", q);
//
//     if (_sent == 560) {
//         perf_engine_stop(sock->base.engine);
//         return;
//     }
//
//
//     uv_idle_stop(handle);
//
//     uv_udp_send_t* req = malloc(sizeof(uv_udp_send_t)); // TODO
//     if (!req) {
//         perf_log_fatal("malloc(uv_udp_send_t)");
//     }
//     // if (_req_list) {
//     //     req = _req_list;
//     //     _req_list = *(void**)_req_list;
//     // } else {
//     //     req = malloc(sizeof(*req));
//     // }
//     // if (!req) {
//     //     uv_close((uv_handle_t*)handle, _uv_close_cb);
//     //     _buf_add(buf->base);
//     //     return;
//     // }
//
//     char* q = "skjdhfkjsdhfkjhsdjfkhsdsdkfhjskldhjflksdjflksdjlkfjsdkljlk";
//     size_t len = 28;
//
//     // req->data = q;
//
//     uv_buf_t sndbuf = uv_buf_init(q, len);
//     if (uv_udp_send(req, &sock->handle, &sndbuf, 1, 0/*&sock->base.server.sa.sa*/, _send_cb)) {
//         perf_log_fatal("uv_udp_send()");
//     //     uv_close((uv_handle_t*)handle, _uv_close_cb);
//     //     _req_add(req);
//     //     _buf_add(buf->base);
//     //     return;
//     }
//     // printf("sending %p %zu\n", q, len);
//     printf("sent %zu\n", ++_sent);
// }

/*

use idle on return of response if queue is full


need slabs on both sides

*/

static uv_loop_t *loop;
static size_t _sent = 0, _bytes = 0, _recv = 0, _rbytes = 0;

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
        printf("sent %zu bytes %zu, recv %zu bytes %zu\n", _sent, _bytes, _recv, _rbytes);
        return;
    }
}

static void _send_cb(uv_udp_send_t* req, int status)
{
    if (status) {
    }

    uv_udp_t* handle = req->handle;
    free(req);

    if (_sent == 560) {
        return;
    }

    req = calloc(1, sizeof(uv_udp_send_t)); // TODO
    char* q = "skjdhfkjsdhfkjhsdjfkhsdsdkfhjskldhjflksdjflksdjlkfjsdkljlk";
    size_t len = 28;
    uv_buf_t sndbuf = uv_buf_init(q, len);
    uv_udp_send(req, handle, &sndbuf, 1, 0, _send_cb);
    // printf("sent %zu\n", ++_sent);
    _sent++;
    _bytes += len;
}

int perf_engine_udp_opensocket(perf_engine_t* engine, const perf_sockaddr_t* server, const perf_sockaddr_t* local)
{
    assert(engine);
    assert(server);
    assert(local);

    perf_udp_socket_t* sock = calloc(1, sizeof(perf_udp_socket_t));
    if (!sock) {
        return 0;
    }
    sock->base.server = *server;
    sock->base.local = *local;
    sock->base.engine = engine;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    int bufsize = 1024*1024;
    int ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    if (ret < 0)
        perf_log_warning("setsockbuf(SO_RCVBUF) failed");

    ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    if (ret < 0)
        perf_log_warning("setsockbuf(SO_SNDBUF) failed");

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        perf_log_fatal("fcntl(F_GETFL)");

    ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (ret < 0)
        perf_log_fatal("fcntl(F_SETFL)");


    uv_udp_init(engine->loop, &sock->handle);
    if (uv_udp_open(&sock->handle, fd))
        perf_log_fatal("uv_udp_open()");
    sock->handle.data = sock;
    if (uv_udp_bind(&sock->handle, &local->sa.sa, 0)) // TODO
        perf_log_fatal("uv_udp_bind()");
    if (uv_udp_connect(&sock->handle, &server->sa.sa)) // ??
        perf_log_fatal("uv_udp_connect()");

    // sock->base.next = engine->sockets;
    // engine->sockets = (perf_socket_t*)sock;

    // uv_idle_init(engine->loop, &sock->idle);
    // sock->idle.data = sock;
    // uv_idle_start(&sock->idle, _idle);

    // int s = 1024*1024;
    // if (uv_recv_buffer_size((uv_handle_t*)&sock->handle, &s)) {
    //     perf_log_fatal("uv_recv_buffer_size()");
    // }
    // s = 1024*1024;
    // if (uv_send_buffer_size((uv_handle_t*)&sock->handle, &s)) {
    //     perf_log_fatal("uv_send_buffer_size()");
    // }

    loop = uv_default_loop();

    uv_udp_send_t* req = calloc(1, sizeof(uv_udp_send_t)); // TODO
    char* q = "skjdhfkjsdhfkjhsdjfkhsdsdkfhjskldhjflksdjflksdjlkfjsdkljlk";
    size_t len = 28;
    uv_buf_t sndbuf = uv_buf_init(q, len);
    uv_udp_send(req, &sock->handle, &sndbuf, 1, 0, _send_cb);
    // printf("sent %zu\n", ++_sent);
    _sent++;
    _bytes += len;

    uv_udp_recv_start(&sock->handle, _alloc_cb, _recv_cb);


    // uv_udp_send_t* req = calloc(1, sizeof(uv_udp_send_t)); // TODO
    // if (!req) {
    //     perf_log_fatal("malloc(uv_udp_send_t)");
    // }
    // char* q = "skjdhfkjsdhfkjhsdjfkhsdsdkfhjskldhjflksdjflksdjlkfjsdkljlk";
    // size_t len = 28;
    // uv_buf_t sndbuf = uv_buf_init(q, len);
    // if (uv_udp_send(req, &sock->handle, &sndbuf, 1, 0/*&sock->base.server.sa.sa*/, _send_cb2)) {
    //     perf_log_fatal("uv_udp_send()");
    // }
    // _sent++;
    // // printf("sent %zu\n", _sent);
    //
    //
    // if (uv_udp_recv_start(&sock->handle, _alloc_cb, _recv_cb)) {
    //     perf_log_fatal("uv_udp_recv_start()");
    // //     uv_close((uv_handle_t*)req->handle, _uv_close_cb);
    // }

    return 0;
}



#if 0

static void _uv_stats_cb(uv_timer_t* w) {
    stats_cb();
}

void* _req_list = 0;

inline void _req_add(void* vp) {
    *(void**)vp = _req_list;
    _req_list = vp;
}

void* _buf_list = 0;

inline void _buf_add(void* vp) {
    *(void**)vp = _buf_list;
    _buf_list = vp;
}

static void _uv_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = recvbuf;
    buf->len = sizeof(recvbuf);
}

static void _uv_alloc_reflect_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    if (_buf_list) {
        buf->base = _buf_list;
        _buf_list = *(void**)_buf_list;
    } else {
        buf->base = malloc(4096);
    }
    buf->len = 4096;
}

static void _uv_close_cb(uv_handle_t* handle) {
    free(handle);
}

static void _uv_udp_recv_reflect_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags);

static void _uv_udp_send_cb(uv_udp_send_t* req, int status) {
    if (uv_udp_recv_start(req->handle, _uv_alloc_reflect_cb, _uv_udp_recv_reflect_cb)) {
        uv_close((uv_handle_t*)req->handle, _uv_close_cb);
    }
    _buf_add(req->data);
    _req_add(req);
}

static void _uv_udp_recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
    if (nread < 0) {
        uv_udp_recv_stop(handle);
        uv_close((uv_handle_t*)handle, _uv_close_cb);
        return;
    }

    _stats.pkts++;
    _stats.bytes += nread;
}

static void _uv_udp_recv_reflect_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
    if (nread < 0) {
        uv_udp_recv_stop(handle);
        uv_close((uv_handle_t*)handle, _uv_close_cb);
        _buf_add(buf->base);
        return;
    }

    _stats.pkts++;
    _stats.bytes += nread;

    uv_udp_recv_stop(handle);

    uv_udp_send_t* req;
    if (_req_list) {
        req = _req_list;
        _req_list = *(void**)_req_list;
    } else {
        req = malloc(sizeof(*req));
    }
    if (!req) {
        uv_close((uv_handle_t*)handle, _uv_close_cb);
        _buf_add(buf->base);
        return;
    }
    req->data = buf->base;
    uv_buf_t sndbuf;
    sndbuf = uv_buf_init(buf->base, nread);
    if (uv_udp_send(req, handle, &sndbuf, 1, addr, _uv_udp_send_cb)) {
        uv_close((uv_handle_t*)handle, _uv_close_cb);
        _req_add(req);
        _buf_add(buf->base);
        return;
    }
}

static void _uv_tcp_recv_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
    if (nread < 0) {
        uv_read_stop(handle);
        uv_close((uv_handle_t*)handle, _uv_close_cb);
        return;
    }

    _stats.pkts++;
    _stats.bytes += nread;
}

static void _uv_on_connect_cb(uv_stream_t* server, int status) {
    uv_tcp_t* tcp;
    int err;

    if (status) {
        _stats.accdrop++;
        return;
    }

    tcp = calloc(1, sizeof(uv_tcp_t));
    if ((err = uv_tcp_init(uv_default_loop(), tcp))) {
        fprintf(stderr, "uv_tcp_init() %s\n", uv_strerror(err));
        free(tcp);
        _stats.accdrop++;
        return;
    }
    if ((err = uv_accept(server, (uv_stream_t*)tcp))) {
        fprintf(stderr, "uv_accept() %s\n", uv_strerror(err));
        uv_close((uv_handle_t*)tcp, _uv_close_cb);
        _stats.accdrop++;
        return;
    }
    _stats.accept++;
    if ((err = uv_read_start((uv_stream_t*)tcp, _uv_alloc_cb, _uv_tcp_recv_cb))) {
        fprintf(stderr, "uv_read_start() %s\n", uv_strerror(err));
        uv_close((uv_handle_t*)tcp, _uv_close_cb);
        return;
    }
    _stats.conns++;
}


int engine_udp_run(void) {
    uv_run(uv_default_loop(), 0);
    return 0;
}

    int opt, use_udp = 0, use_tcp = 0, reuse_addr = 0, reuse_port = 0, linger = 0, reflect = 0;
    struct addrinfo* addrinfo = 0;
    struct addrinfo hints;
    const char* node = 0;
    const char* service = 0;
    int use_ev = 0, use_uv = 0;

#if defined(HAVE_LIBUV)
    use_uv = 1;
#elif defined(HAVE_LIBEV)
    use_ev = 1;
#endif


    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(node, service, &hints, &addrinfo)) {
        perror("getaddrinfo()");
        return 1;
    }
    if (!addrinfo) {
        return 1;
    }

    {
        struct addrinfo* ai = addrinfo;
        int fd, optval, flags;

        for (; ai; ai = ai->ai_next) {
            switch (ai->ai_socktype) {
                case SOCK_DGRAM:
                case SOCK_STREAM:
                    break;
                default:
                    continue;
            }

            switch (ai->ai_protocol) {
                case IPPROTO_UDP:
                    if (!use_udp)
                        continue;
                    break;
                case IPPROTO_TCP:
                    if (!use_tcp)
                        continue;
                    break;
                default:
                    continue;
            }

            fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if (fd < 0) {
                perror("socket()");
                return 1;
            }

#ifdef SO_REUSEADDR
            if (reuse_addr) {
                optval = 1;
                if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
                    perror("setsockopt(SO_REUSEADDR)");
                    return 1;
                }
            }
#endif
#ifdef SO_REUSEPORT
            if (reuse_port) {
                optval = 1;
                if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))) {
                    perror("setsockopt(SO_REUSEPORT)");
                    return 1;
                }
            }
#endif
            {
                struct linger l = { 0, 0 };
                if (linger > 0) {
                    l.l_onoff = 1;
                    l.l_linger = linger;
                }
                if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l))) {
                    perror("setsockopt(SO_LINGER)");
                    return 1;
                }
            }

            if ((flags = fcntl(fd, F_GETFL)) == -1) {
                perror("fcntl(F_GETFL)");
                return 1;
            }
            if (fcntl(fd, F_SETFL, flags | O_NONBLOCK)) {
                perror("fcntl(F_SETFL)");
                return 1;
            }

            if (use_uv) {
                int err;
                if (ai->ai_socktype == SOCK_DGRAM) {
                    uv_udp_t* udp = calloc(1, sizeof(uv_udp_t));

                    if ((err = uv_udp_init(uv_default_loop(), udp))) {
                        fprintf(stderr, "uv_udp_init() %s\n", uv_strerror(err));
                        return 1;
                    }
                    if ((err = uv_udp_open(udp, fd))) {
                        fprintf(stderr, "uv_udp_open() %s\n", uv_strerror(err));
                        return 1;
                    }
                    if ((err = uv_udp_bind(udp, ai->ai_addr, UV_UDP_REUSEADDR))) {
                        fprintf(stderr, "uv_udp_bind() %s\n", uv_strerror(err));
                        return 1;
                    }
                    if (reflect) {
                        printf("reflecting UDP packets\n");
                        if ((err = uv_udp_recv_start(udp, _uv_alloc_reflect_cb, _uv_udp_recv_reflect_cb))) {
                            fprintf(stderr, "uv_udp_recv_start() %s\n", uv_strerror(err));
                            return 1;
                        }
                    } else {
                        if ((err = uv_udp_recv_start(udp, _uv_alloc_cb, _uv_udp_recv_cb))) {
                            fprintf(stderr, "uv_udp_recv_start() %s\n", uv_strerror(err));
                            return 1;
                        }
                    }
                }
                else if(ai->ai_socktype == SOCK_STREAM) {
                    uv_tcp_t* tcp = calloc(1, sizeof(uv_tcp_t));

                    if ((err = uv_tcp_init(uv_default_loop(), tcp))) {
                        fprintf(stderr, "uv_tcp_init() %s\n", uv_strerror(err));
                        return 1;
                    }
                    if ((err = uv_tcp_open(tcp, fd))) {
                        fprintf(stderr, "uv_tcp_open() %s\n", uv_strerror(err));
                        return 1;
                    }
                    if ((err = uv_tcp_bind(tcp, ai->ai_addr, UV_UDP_REUSEADDR))) {
                        fprintf(stderr, "uv_tcp_bind() %s\n", uv_strerror(err));
                        return 1;
                    }
                    if ((err = uv_listen((uv_stream_t*)tcp, 10, _uv_on_connect_cb))) {
                        fprintf(stderr, "uv_listen() %s\n", uv_strerror(err));
                        return 1;
                    }
                }
                else {
                    continue;
                }
            }
            else
            {
                return 3;
            }

            {
                char h[NI_MAXHOST], s[NI_MAXSERV];
                if (getnameinfo(ai->ai_addr, ai->ai_addrlen, h, NI_MAXHOST, s, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV)) {
                    perror("getnameinfo()");
                    h[0] = 0;
                    s[0] = 0;
                }

                printf("listen: %d fam: %d type: %d proto: %d host: %s service: %s\n", fd, ai->ai_family, ai->ai_socktype, ai->ai_protocol, h, s);
            }
        }
    }

    freeaddrinfo(addrinfo);

#endif
