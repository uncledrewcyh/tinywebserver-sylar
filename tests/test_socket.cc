#include "src/socket.h"
#include "src/iomanager.h"
#include "src/log.h"

static pudge::Logger::ptr g_looger = PUDGE_LOG_ROOT();

void test_socket() {
    pudge::IPAddress::ptr addr = pudge::Address::LookupAnyIPAddress("www.taobao.com");
    if(addr) {
        PUDGE_LOG_INFO(g_looger) << "get address: " << addr->toString();
    } else {
        PUDGE_LOG_ERROR(g_looger) << "get address fail";
        return;
    }

    pudge::Socket::ptr sock = pudge::Socket::CreateTCP(addr);
    addr->setPort(80);
    PUDGE_LOG_INFO(g_looger) << "addr=" << addr->toString();
    if(!sock->connect(addr)) {
        PUDGE_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        PUDGE_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        PUDGE_LOG_INFO(g_looger) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if(rt <= 0) {
        PUDGE_LOG_INFO(g_looger) << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(rt);
    PUDGE_LOG_INFO(g_looger) << buffs;
}

void test2() {
    pudge::IPAddress::ptr addr = pudge::Address::LookupAnyIPAddress("www.baidu.com:80");
    if(addr) {
        PUDGE_LOG_INFO(g_looger) << "get address: " << addr->toString();
    } else {
        PUDGE_LOG_ERROR(g_looger) << "get address fail";
        return;
    }

    pudge::Socket::ptr sock = pudge::Socket::CreateTCP(addr);
    if(!sock->connect(addr)) {
        PUDGE_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        PUDGE_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
    }

    uint64_t ts = pudge::GetCurrentUS();
    for(size_t i = 0; i < 10000000000ul; ++i) {
        if(int err = sock->getError()) {
            PUDGE_LOG_INFO(g_looger) << "err=" << err << " errstr=" << strerror(err);
            break;
        }

        //struct tcp_info tcp_info;
        //if(!sock->getOption(IPPROTO_TCP, TCP_INFO, tcp_info)) {
        //    PUDGE_LOG_INFO(g_looger) << "err";
        //    break;
        //}
        //if(tcp_info.tcpi_state != TCP_ESTABLISHED) {
        //    PUDGE_LOG_INFO(g_looger)
        //            << " state=" << (int)tcp_info.tcpi_state;
        //    break;
        //}
        static int batch = 10000000;
        if(i && (i % batch) == 0) {
            uint64_t ts2 = pudge::GetCurrentUS();
            PUDGE_LOG_INFO(g_looger) << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
            ts = ts2;
        }
    }
}

int main(int argc, char** argv) {
    pudge::IOManager iom;
    iom.schedule(&test_socket);
    // iom.schedule(&test_socket);
    return 0;
}
