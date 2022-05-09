#include "src/tcp_server.h"
#include "src/log.h"
#include "src/iomanager.h"
#include "src/bytearray.h"
#include "src/address.h"

static pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

class EchoServer : public pudge::TcpServer {
public:
    EchoServer(int type);
    void handleClient(pudge::Socket::ptr client);

private:
    int m_type = 0;
};

EchoServer::EchoServer(int type)
    :m_type(type) {
}

void EchoServer::handleClient(pudge::Socket::ptr client) {
    PUDGE_LOG_INFO(g_logger) << "handleClient " << *client;   
    pudge::ByteArray::ptr ba(new pudge::ByteArray);
    while(true) {
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);

        int rt = client->recv(&iovs[0], iovs.size());
        if(rt == 0) {
            PUDGE_LOG_INFO(g_logger) << "client close: " << *client;
            break;
        } else if(rt < 0) {
            PUDGE_LOG_INFO(g_logger) << "client error rt=" << rt
                << " errno=" << errno << " errstr=" << strerror(errno);
            break;
        }
        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0); 
        if(m_type == 1) {//text 
            std::cout << ba->toString();// << std::endl;
        } else {
            std::cout << ba->toHexString();// << std::endl;
        }
        std::vector<iovec> iovs1;
        ba->getReadBuffers(iovs1, 1024);
        client->send(&iovs1[0], iovs1.size());
        std::cout.flush();
    }
}

int type = 1;

void run() {
    PUDGE_LOG_INFO(g_logger) << "server type=" << type;
    EchoServer::ptr es(new EchoServer(type));
    auto addr = pudge::Address::LookupAny("0.0.0.0:8020");
    while(!es->bind(addr)) {
        sleep(2);
    }
    es->start();
}

int main(int argc, char** argv) {
    if(argc < 2) {
        PUDGE_LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 0;
    }

    if(!strcmp(argv[1], "-b")) {
        type = 2;
    }

    pudge::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
