#include "servlet.h"
#include <fnmatch.h>
#include "../log.h"

namespace pudge {
namespace http {
static Logger::ptr g_logger = PUDGE_LOG_NAME("system");

FunctionServlet::FunctionServlet(callback cb)
    :Servlet("FunctionServlet")
    ,m_cb(cb) {
}

int32_t FunctionServlet::handle(pudge::http::HttpRequest::ptr request
               , pudge::http::HttpResponse::ptr response
               , pudge::http::HttpSession::ptr session) {
    return m_cb(request, response, session);
}

ServletDispatch::ServletDispatch()
    :Servlet("ServletDispatch") {
    m_default.reset(new NotFoundServlet("pudge/1.0"));
}

int32_t ServletDispatch::handle(pudge::http::HttpRequest::ptr request
               , pudge::http::HttpResponse::ptr response
               , pudge::http::HttpSession::ptr session) {
    auto slt = getMatchedServlet(request->getPath());
    PUDGE_LOG_INFO(g_logger) << request->getPath();
    if(slt) {
        slt->handle(request, response, session);
    }
    return 0;
}

Servlet::ptr ServletDispatch::getMatchedServlet(const std::string& uri) {
    RWMutexType::ReadLock lock(m_mutex);
    auto mit = m_datas.find(uri);
    if (mit != m_datas.end()) {
        return mit->second;
    }
    for (auto &it : m_globs) {
        if (!fnmatch(it.first.c_str(), uri.c_str(), 0)) {
            return it.second;
        }
    }
    return m_default;
}

void ServletDispatch::addServlet(const std::string& uri, Servlet::ptr slt) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas[uri] = slt;
}

void ServletDispatch::addServlet(const std::string& uri
                        ,FunctionServlet::callback cb) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas[uri] = std::make_shared<FunctionServlet>(cb);
}

void ServletDispatch::addGlobServlet(const std::string& uri
                                    ,Servlet::ptr slt) {
    RWMutexType::WriteLock lock(m_mutex);
    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri, slt));
}

void ServletDispatch::addGlobServlet(const std::string& uri
                                    ,FunctionServlet::callback cb) {

    addGlobServlet(uri, std::make_shared<FunctionServlet>(cb));
}

void ServletDispatch::delServlet(const std::string& uri) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas.erase(uri);
}

void ServletDispatch::delGlobServlet(const std::string& uri) {
    RWMutexType::WriteLock lock(m_mutex);
    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
}

Servlet::ptr ServletDispatch::getServlet(const std::string& uri) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second;
}

Servlet::ptr ServletDispatch::getGlobServlet(const std::string& uri) {
    RWMutexType::ReadLock lock(m_mutex);
    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if(it->first == uri) {
            return it->second;
        }
    }
    return nullptr;
}

void ServletDispatch::listAllServlet(std::map<std::string, Servlet::ptr>& infos) {
    RWMutexType::ReadLock lock(m_mutex);
    for(auto& i : m_datas) {
        infos[i.first] = i.second;
    }
}

void ServletDispatch::listAllGlobServlet(std::map<std::string, Servlet::ptr>& infos) {
    RWMutexType::ReadLock lock(m_mutex);
    for(auto& i : m_globs) {
        infos[i.first] = i.second;
    }
}

NotFoundServlet::NotFoundServlet(const std::string& name)
    :Servlet("NotFoundServlet")
    ,m_name(name) {
    m_content = "<html><head><title>404 Not Found"
        "</title></head><body><center><h1>404 Not Found</h1></center>"
        "<hr><center>" + name + "</center></body></html>";

}

int32_t NotFoundServlet::handle(pudge::http::HttpRequest::ptr request
                   , pudge::http::HttpResponse::ptr response
                   , pudge::http::HttpSession::ptr session) {
    response->setStatus(pudge::http::HttpStatus::NOT_FOUND);
    response->setHeader("Server", "pudge/1.0.0");
    response->setHeader("Content-Type", "text/html");
    response->setBody(m_content);
    return 0;
}

}
}