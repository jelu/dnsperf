#include "config.h"

#include "engine.h"
#include "tsig.h"
#include "edns.h"
#include "datafile.h"
#include "util.h"
#include "dns.h"
#include "edns.h"

#define MAX_INPUT_DATA (64 * 1024)

struct config {
    int                argc;
    char**             argv;
    int                family;
    uint32_t           clients;
    uint32_t           threads;
    uint32_t           maxruns;
    uint64_t           timelimit;
    perf_sockaddr_t    server_addr;
    perf_sockaddr_t    local_addr;
    uint64_t           timeout;
    uint32_t           bufsize;
    bool               edns;
    bool               dnssec;
    perf_tsigkey_t*    tsigkey;
    perf_ednsoption_t* edns_option;
    uint32_t           max_outstanding;
    uint32_t           max_qps;
    uint64_t           stats_interval;
    bool               updates;
    bool               verbose;
    enum perf_net_mode mode;
    bool ng_engine;
};

struct ctx {
    struct config* config;
    perf_engine_t* engine;
    perf_datafile_t* input;
};

static void* worker(void* vp)
{
    struct ctx* ctx = (struct ctx*)vp;

return 0;
    perf_result_t   result;
    perf_buffer_t   lines, msg;
    perf_region_t   used;
    char            input_data[MAX_INPUT_DATA];
    unsigned char   packet_buffer[MAX_EDNS_PACKET];

    perf_buffer_init(&lines, input_data, sizeof(input_data));
    perf_buffer_init(&msg, packet_buffer, ctx->config->edns ? MAX_EDNS_PACKET : MAX_UDP_PACKET);

    uint16_t qid = 0;

    size_t inqueue = 0;
    char* q = 0;

    while(1) {
        if (!q) {
            perf_buffer_clear(&lines);
            result = perf_datafile_next(ctx->input, &lines, ctx->config->updates);
            if (result != PERF_R_SUCCESS) {
                if (result == PERF_R_INVALIDFILE)
                    perf_log_fatal("input file contains no data");
                break;
            }
            // printf("read: %.*s\n", (int)perf_buffer_usedlength(&lines), (char*)perf_buffer_base(&lines));


            // qid = q - tinfo->queries;
            perf_buffer_usedregion(&lines, &used);
            perf_buffer_clear(&msg);
            result = perf_dns_buildrequest(&used, qid++,
                ctx->config->edns, ctx->config->dnssec, ctx->config->updates,
                ctx->config->tsigkey, ctx->config->edns_option,
                &msg);
            if (result != PERF_R_SUCCESS) {
                // PERF_LOCK(&tinfo->lock);
                // query_move(tinfo, q, prepend_unused);
                // PERF_UNLOCK(&tinfo->lock);
                // now = perf_get_time();
                continue;
            }

            // base   = perf_buffer_base(&msg);
            // length = perf_buffer_usedlength(&msg);

            q = malloc(perf_buffer_usedlength(&msg));
            memcpy(q, perf_buffer_base(&msg), perf_buffer_usedlength(&msg));
        }

        if (!perf_engine_enqueue_query(ctx->engine, q, perf_buffer_usedlength(&msg))) {
        //     printf("FAILED enqueue %p %.*s\n", q, (int)perf_buffer_usedlength(&lines), (char*)perf_buffer_base(&lines));
        //     free(q);
        // } else {
            // printf("enqueue %p %.*s\n", q, (int)perf_buffer_usedlength(&lines), (char*)perf_buffer_base(&lines));
            inqueue++;
            q = 0;
        }

        void* r;
        size_t len;
        for (;;) {
            perf_engine_dequeue_response(ctx->engine, &r, &len);
            if (!r) {
                break;
            }
            // printf("dequeue %p\n", r);
            free(r);
            inqueue--;
        }
        // printf("inqueue %zu\n", inqueue);
    }

    printf("inqueue %zu\n", inqueue);

    while (inqueue) {
        void* r;
        size_t len;
        for (;;) {
            perf_engine_dequeue_response(ctx->engine, &r, &len);
            if (!r) {
                break;
            }
            // printf("dequeue %p\n", r);
            free(r);
            inqueue--;
        }
    }

    perf_engine_stop(ctx->engine);

/*

- if no line
  - read datafile line
  - build DNS
- enqueue query
  - if full send async
- dequeue responses

*/

    return 0;
}

static void* engine(void* vp)
{
    struct ctx* ctx = (struct ctx*)vp;

    size_t n;
    perf_sockaddr_t        tmp;
    tmp  = ctx->config->local_addr;
    in_port_t port = perf_sockaddr_port(&tmp);

    for (n = 0; n < ctx->config->clients; n++) {
        if (port != 0 && n > 0) {
            port += n;
            if (port >= 0xFFFF)
                perf_log_fatal("port %d out of range", port);
            perf_sockaddr_setport(&tmp, port);
        }

        char src[512], dst[512];
        perf_sockaddr_format(&ctx->config->server_addr, dst, sizeof(dst));
        perf_sockaddr_format(&tmp, src, sizeof(src));
        printf("c %zu: %s:%d -> %s:%d\n", n, src, perf_sockaddr_port(&tmp), dst, perf_sockaddr_port(&ctx->config->server_addr));

        if (perf_engine_opensocket(ctx->engine, &ctx->config->server_addr, &tmp)) {
            perf_log_fatal("perf_engine_opensocket()");
        }
    }

/*

- setup engine sockets
  - socket connect
  - socket pull query
    - if no query, add to idle list
  - when idle async sent
    - restart sockets

*/
    printf("run\n");
    perf_engine_run(ctx->engine);
    printf("run done\n");

    void* q;
    size_t len;
    for (;;) {
        perf_engine_dequeue_query(ctx->engine, &q, &len);
        if (!q) {
            break;
        }
        // printf("cleanup qry %p\n", q);
        free(q);
    }
    void* r;
    for (;;) {
        perf_engine_dequeue_response(ctx->engine, &r, &len);
        if (!r) {
            break;
        }
        // printf("cleanup resp %p\n", r);
        free(r);
    }

/*

can each socket get own queries?

- get query
  - find socket that is ready
  - send query

*/
    return 0;
}


static uv_loop_t *loop;
static uv_udp_t sock;
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

static void* engine2(void* vp)
{
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

    return 0;
}


int dnsperf_engine(struct config* config, perf_datafile_t* input)
{
    struct ctx ctx = {
        .config = config,
        .input = input,
    };
    pthread_t w, e;

    ctx.engine = perf_engine_new(config->mode);

    PERF_THREAD(&w, worker, &ctx);
    PERF_THREAD(&e, engine, &ctx);
    PERF_JOIN(w, NULL);
    PERF_JOIN(e, NULL);

    perf_engine_destroy(ctx.engine);

    return 0;
}
