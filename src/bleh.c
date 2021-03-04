#include "config.h"

#include "engine.h"
#include "tsig.h"
#include "edns.h"
#include "datafile.h"
#include "util.h"
#include "dns.h"
#include "edns.h"

const char* progname = "bleh";

int main(void) {
    perf_engine_t* e = perf_engine_new(sock_udp);

    perf_sockaddr_t server, local;

    perf_net_parseserver(AF_UNSPEC, "127.0.0.1", 5353, &server);
    perf_net_parselocal(server.sa.sa.sa_family, 0, 0, &local);

    perf_engine_opensocket(e, &server, &local);

    perf_engine_run(e);
}
