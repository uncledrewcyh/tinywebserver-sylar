#include "src/tcp_server.h"
#include "src/iomanager.h"
#include "src/log.h"

pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

void run() {
    auto addr = pudge::Address::LookupAny("0.0.0.0:8033");
    std::vector<pudge::Address::ptr> addrs;
    addrs.push_back(addr);

    pudge::TcpServer::ptr tcp_server(new pudge::TcpServer);
    std::vector<pudge::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
    
}
int main(int argc, char** argv) {
    pudge::IOManager iom(4);
    iom.schedule(run);
    return 0;
}
