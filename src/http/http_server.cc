#include "http_server.h"
#include "../log.h"

namespace pudge {
namespace http {
static pudge::Logger::ptr g_logger = PUDGE_LOG_NAME("system");


HttpServer::HttpServer(bool keepalive
          ,pudge::IOManager* worker
          ,pudge::IOManager* io_worker 
          ,pudge::IOManager* accept_worker)
        :TcpServer(worker, io_worker, accept_worker)
        ,m_isKeepalive(keepalive) {
    m_dispatch.reset(new ServletDispatch);
    m_type = "http";
}



void HttpServer::setName(const std::string& v) {
    TcpServer::setName(v);
    m_dispatch->setDefault(std::make_shared<NotFoundServlet>(v));
}

void HttpServer::handleClient(Socket::ptr client) {
    PUDGE_LOG_DEBUG(g_logger) << "handleClient " << *client;
    HttpSession::ptr session(new HttpSession(client));
    do {
        auto req = session->recvRequest();
        if(!req) {
            PUDGE_LOG_DEBUG(g_logger) << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " cliet:" << *client << " keep_alive=" << m_isKeepalive;
            break;
        }
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion()
                            ,req->isClose() || !m_isKeepalive));
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);
        if(!m_isKeepalive || req->isClose()) {
            break;
        }                    
    } while (true);
    session->close();
}


}
}