#pragma once
#include "../tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace pudge {
namespace http {

class HttpServer: public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;

    /**
     * @brief 构造函数
     * @param[in] keepalive 是否长连接
     * @param[in] worker 工作调度器
     * @param[in] accept_worker 接收连接调度器
     */
    HttpServer(bool keepalive = false
               ,pudge::IOManager* worker = pudge::IOManager::GetThis()
               ,pudge::IOManager* io_worker = pudge::IOManager::GetThis()
               ,pudge::IOManager* accept_worker = pudge::IOManager::GetThis());

    /**
     * @brief 获取ServletDispatch
     */
    ServletDispatch::ptr getServletDispatch() const { return m_dispatch;}
    
    virtual void setName(const std::string& v) override;

protected:
    virtual void handleClient(Socket::ptr client) override;
    
protected:
    /// 是否支持长连接
    bool m_isKeepalive;
    /// Servlet分发器
    ServletDispatch::ptr m_dispatch;
};
}
}