#include "net.h"

#ifndef PERF_ENGINE_H
#define PERF_ENGINE_H 1

#include <uv.h>
#include <ck_ring.h>
#include <stdbool.h>

typedef struct perf_socket perf_socket_t;
typedef struct perf_engine perf_engine_t;
typedef struct perf_engine_ringwrapper perf_engine_ringwrapper_t;

struct perf_engine_ringwrapper {
    perf_engine_ringwrapper_t* next;
    void* data;
    size_t len;
};

struct perf_socket {
    perf_socket_t* next;
    perf_sockaddr_t server;
    perf_sockaddr_t local;
    perf_engine_t* engine;
};

struct perf_engine {
    uv_loop_t* loop;
    uv_async_t stop;
    bool stopped;
    ck_ring_buffer_t *queries_buf, *responses_buf;
    ck_ring_t queries, responses;

    perf_engine_ringwrapper_t* produce, *consume;

    enum perf_net_mode mode;

    size_t num_sockets;
    perf_socket_t* sockets;
};

perf_engine_t* perf_engine_new(enum perf_net_mode);
void perf_engine_destroy(perf_engine_t*);

void perf_engine_run(perf_engine_t*);
void perf_engine_stop(perf_engine_t*);

// int perf_engine_setup(perf_engine_t*, enum perf_net_mode);

int perf_engine_opensocket(perf_engine_t*, const perf_sockaddr_t*, const perf_sockaddr_t*);

int perf_engine_enqueue_query(perf_engine_t*, void*, size_t);
void perf_engine_dequeue_query(perf_engine_t*, void**, size_t*);
int perf_engine_enqueue_response(perf_engine_t*, void*, size_t);
void perf_engine_dequeue_response(perf_engine_t*, void**, size_t*);

#endif
