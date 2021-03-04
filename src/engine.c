#include "config.h"

#include "engine.h"
#include "engine_udp.h"

#include <stdlib.h>
#include <assert.h>

void _stop(uv_async_t* handle)
{
    uv_stop((uv_loop_t*)handle->data);
}

perf_engine_t* perf_engine_new(enum perf_net_mode mode)
{
    perf_engine_t* engine = calloc(1, sizeof(perf_engine_t));

    if (!engine) {
        return 0;
    }

    engine->mode = mode;

    engine->queries_buf = calloc(1024, sizeof(ck_ring_buffer_t));
    if (!engine->queries_buf) {
        free(engine);
        return 0;
    }
    ck_ring_init(&engine->queries, 1024);

    engine->responses_buf = calloc(1024, sizeof(ck_ring_buffer_t));
    if (!engine->responses_buf) {
        free(engine->queries_buf);
        free(engine);
        return 0;
    }
    ck_ring_init(&engine->responses, 1024);

    // uv_loop_init(&engine->loop);
    engine->loop = uv_default_loop();

    engine->stopped = false;
    engine->stop.data = engine->loop;
    uv_async_init(engine->loop, &engine->stop, _stop);

    return engine;
}

void perf_engine_destroy(perf_engine_t* engine)
{
    assert(engine);

    uv_loop_close(engine->loop);
    free(engine->queries_buf);
    free(engine->responses_buf);

    perf_engine_ringwrapper_t* next;
    while (engine->produce) {
        next = engine->produce->next;
        free(engine->produce);
        engine->produce = next;
    }
    while (engine->consume) {
        next = engine->consume->next;
        free(engine->consume);
        engine->consume = next;
    }

    free(engine);
}

void perf_engine_run(perf_engine_t* engine)
{
    assert(engine);

    if (engine->stopped)
        return;

    uv_run(engine->loop, UV_RUN_DEFAULT);
}

void perf_engine_stop(perf_engine_t* engine)
{
    assert(engine);

    uv_async_send(&engine->stop); // TODO
}

// int perf_engine_setup(perf_engine_t* engine, enum perf_net_mode mode)
// {
//     assert(engine);
//
//     switch (mode) {
//     case sock_udp:
//         return perf_engine_udp_setup(engine);
//         break;
//     default:
//         assert(0);
//         break;
//     }
//
//     return 1;
// }

int perf_engine_opensocket(perf_engine_t* engine, const perf_sockaddr_t* server, const perf_sockaddr_t* local)
{
    assert(engine);

    switch (engine->mode) {
    case sock_udp:
        return perf_engine_udp_opensocket(engine, server, local);
        break;
    default:
        assert(0);
        break;
    }

    return 1;
}

int perf_engine_enqueue_query(perf_engine_t* engine, void* query, size_t len)
{
    assert(engine);
    assert(query);

    perf_engine_ringwrapper_t* wrap = engine->produce;
    if (wrap) {
        engine->produce = wrap->next;
        wrap->next = 0;
    } else {
        wrap = calloc(1, sizeof(perf_engine_ringwrapper_t));
    }

    wrap->data = query;
    wrap->len = len;

    if (ck_ring_enqueue_spsc(&engine->queries, engine->queries_buf, wrap)) {
        return 0;
    }

    wrap->next = engine->produce;
    engine->produce = wrap;

    return 1;
}

void perf_engine_dequeue_query(perf_engine_t* engine, void** query, size_t* len)
{
    assert(engine);

    perf_engine_ringwrapper_t* wrap = 0;

    ck_ring_dequeue_spsc(&engine->queries, engine->queries_buf, &wrap);

    if (wrap) {
        *query = wrap->data;
        *len = wrap->len;

        wrap->next = engine->consume;
        engine->consume = wrap;
    } else {
        *query = 0;
        *len = 0;
    }
}

int perf_engine_enqueue_response(perf_engine_t* engine, void* response, size_t len)
{
    assert(engine);
    assert(response);

    perf_engine_ringwrapper_t* wrap = engine->consume;
    if (wrap) {
        engine->consume = wrap->next;
        wrap->next = 0;
    } else {
        wrap = calloc(1, sizeof(perf_engine_ringwrapper_t));
    }

    wrap->data = response;
    wrap->len = len;

    if (ck_ring_enqueue_spsc(&engine->responses, engine->responses_buf, wrap)) {
        return 0;
    }

    wrap->next = engine->consume;
    engine->consume = wrap;

    return 1;
}

void perf_engine_dequeue_response(perf_engine_t* engine, void** response, size_t* len)
{
    assert(engine);

    perf_engine_ringwrapper_t* wrap = 0;

    ck_ring_dequeue_spsc(&engine->responses, engine->responses_buf, &wrap);

    if (wrap) {
        *response = wrap->data;
        *len = wrap->len;

        wrap->next = engine->produce;
        engine->produce = wrap;
    } else {
        *response = 0;
        *len = 0;
    }
}
