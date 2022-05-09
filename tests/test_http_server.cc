#include "src/http/http_server.h"
#include "src/log.h"

pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

void run() {
    auto addr = pudge::Address::LookupAny("0.0.0.0:8033");
    pudge::http::HttpServer::ptr http_server(new pudge::http::HttpServer);
    http_server->bind(addr);
    http_server->start();
}
int main(int argc, char** argv) {
    pudge::IOManager iom(4);
    iom.schedule(run);
    return 0;
}
