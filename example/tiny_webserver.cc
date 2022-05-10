#include "tiny_webserver.h"
#include "src/bytearray.h"
#include "src/db/mysql_pool.h"
#include "functional"

pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

pudge::ConfigVar<std::string>::ptr g_host_addr = pudge::Config::Lookup("host", std::string("0.0.0.0:9006"), "host ip config");

static const char* judge_html = "/home/pudge/workspace/pudge-server/root/judge.html";
static const char* register_html = "/home/pudge/workspace/pudge-server/root/register.html";
static const char* log_html = "/home/pudge/workspace/pudge-server/root/log.html";
static const char* welcome_html = "/home/pudge/workspace/pudge-server/root/welcome.html";
static const char* log_error_html = "/home/pudge/workspace/pudge-server/root/logError.html";
static const char* register_error_html = "/home/pudge/workspace/pudge-server/root/registerError.html";
static const char* pic1 = "/home/pudge/workspace/pudge-server/root/xxx.jpg";
static const char* vid1 = "/home/pudge/workspace/pudge-server/root/video/xxx.mp4";
static const char* favicon = "/home/pudge/workspace/pudge-server/root/favicon.ico";
static const char* vid1_html = "/home/pudge/workspace/pudge-server/root/video.html";
static const char* pic1_html = "/home/pudge/workspace/pudge-server/root/picture.html";

TinyWebserver::TinyWebserver(bool keepalive
               ,pudge::IOManager* worker 
               ,pudge::IOManager* io_worker
               ,pudge::IOManager* accept_worker)
    :HttpServer(keepalive, worker, io_worker, accept_worker)
    ,m_sql_pool(PUDGE_MYSQL_ROOT()){

    auto sd = getServletDispatch();
    sd->addGlobServlet("/*", std::bind(&TinyWebserver::main_page, 
                        this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    #define XX(uri, fun) \
        sd->addServlet(#uri, std::bind(&TinyWebserver::fun, \
                            this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    XX(/, main_page);
    XX(/register, register_page);
    XX(/log, log_page);
    XX(/check_register,check_register_page);
    XX(/check_log, check_log_page);
    XX(/picture/1, picture_1_page);
    XX(/video/1, video_1_page);
    XX(/video/xxx.mp4, video_1_resource);
    XX(/picture/xxx.jpg, picture_1_resource);
    XX(/favicon.ico, favicon_1_resource);
    #undef XX
}

int32_t TinyWebserver::favicon_1_resource(pudge::http::HttpRequest::ptr req
                ,pudge::http::HttpResponse::ptr rsp
                ,pudge::http::HttpSession::ptr session) {
    rsp->setVersion(17);
    rsp->setStatus(pudge::http::HttpStatus::OK);
    pudge::ByteArray::ptr m_ba(new pudge::ByteArray);
    m_ba->readFromFile(favicon);
    m_ba->setPosition(0);
    rsp->setFileBa(m_ba);
    rsp->setFileResponse(true);
    return 0; 
}

int32_t TinyWebserver::picture_1_page(pudge::http::HttpRequest::ptr req
                ,pudge::http::HttpResponse::ptr rsp
                ,pudge::http::HttpSession::ptr session) {
    rsp->setVersion(17);
    rsp->setStatus(pudge::http::HttpStatus::OK);
    pudge::ByteArray::ptr m_ba(new pudge::ByteArray);
    // if (checkCookie(req)) {
            m_ba->readFromFile(pic1_html);
    // } else {
    //     m_ba->readFromFile(log_html);
    // }
    m_ba->setPosition(0);
    rsp->setHeader("Content-Type", "text/html");
    rsp->setFileBa(m_ba);
    rsp->setFileResponse(true);
    return 0; 
}

int32_t TinyWebserver::video_1_page(pudge::http::HttpRequest::ptr req
                ,pudge::http::HttpResponse::ptr rsp
                ,pudge::http::HttpSession::ptr session) {
    rsp->setVersion(17);
    rsp->setStatus(pudge::http::HttpStatus::OK);
    pudge::ByteArray::ptr m_ba(new pudge::ByteArray);
    // if (checkCookie(req)) {
        m_ba->readFromFile(vid1_html);
    // } else {
    //     m_ba->readFromFile(log_html);
    // }
    m_ba->setPosition(0);
    rsp->setHeader("Content-Type", "text/html");
    rsp->setFileBa(m_ba);
    rsp->setFileResponse(true);
    return 0; 
}

int32_t TinyWebserver::picture_1_resource(pudge::http::HttpRequest::ptr req
                ,pudge::http::HttpResponse::ptr rsp
                ,pudge::http::HttpSession::ptr session) {
    rsp->setVersion(17);
    rsp->setStatus(pudge::http::HttpStatus::OK);
    pudge::ByteArray::ptr m_ba(new pudge::ByteArray);
    m_ba->readFromFile(pic1);
    rsp->setHeader("Content-Type", "image/jpg");
    m_ba->setPosition(0);
    rsp->setFileBa(m_ba);
    rsp->setFileResponse(true);
    return 0; 
}

int32_t TinyWebserver::video_1_resource(pudge::http::HttpRequest::ptr req
                ,pudge::http::HttpResponse::ptr rsp
                ,pudge::http::HttpSession::ptr session) {
    rsp->setVersion(17);
    rsp->setStatus(pudge::http::HttpStatus::OK);
    pudge::ByteArray::ptr m_ba(new pudge::ByteArray);
    m_ba->readFromFile(vid1);
    rsp->setHeader("Content-Type", "video/mp4");
    m_ba->setPosition(0);
    rsp->setFileBa(m_ba);
    rsp->setFileResponse(true);
    return 0; 
}

int32_t TinyWebserver::main_page(pudge::http::HttpRequest::ptr req
                ,pudge::http::HttpResponse::ptr rsp
                ,pudge::http::HttpSession::ptr session) {
    rsp->setVersion(17);
    rsp->setStatus((pudge::http::HttpStatus)200);
    rsp->setHeader("Content-Type", "text/html");
    pudge::ByteArray::ptr m_ba(new pudge::ByteArray);
    m_ba->readFromFile(judge_html);
    m_ba->setPosition(0);
    rsp->setFileBa(m_ba);
    rsp->setFileResponse(true);
    return 0; 
}

int32_t TinyWebserver::log_page(pudge::http::HttpRequest::ptr req
                ,pudge::http::HttpResponse::ptr rsp
                ,pudge::http::HttpSession::ptr session) {
    rsp->setVersion(17);
    rsp->setStatus((pudge::http::HttpStatus)200);
    rsp->setHeader("Content-Type", "text/html");
    pudge::ByteArray::ptr m_ba(new pudge::ByteArray);
    m_ba->readFromFile(log_html);
    m_ba->setPosition(0);
    rsp->setFileBa(m_ba);
    rsp->setFileResponse(true);
    return 0; 
}

int32_t TinyWebserver::check_register_page(pudge::http::HttpRequest::ptr req
                ,pudge::http::HttpResponse::ptr rsp
                ,pudge::http::HttpSession::ptr session) {
    rsp->setVersion(17);
    rsp->setStatus((pudge::http::HttpStatus)200);
    rsp->setHeader("Content-Type", "text/html");
    std::string user = req->getParam("user");
    std::string password = req->getParam("password");

    pudge::ByteArray::ptr m_ba(new pudge::ByteArray);
    std::vector<std::vector<std::string>> res;
    std::stringstream ss;
    ss << "select passwd from user where username = '" << user << "'";
    if (m_sql_pool->Query(ss.str().c_str(), res)) {
        if (!res.empty()) {
            m_ba->readFromFile(register_error_html);
            PUDGE_LOG_INFO(g_logger) << "user: " << user << " password: " << password << " has exists failed!";
        } else {
            ss.clear();
            ss.str("");
            ss << "insert into user(username, passwd) values('" << user <<"', '" << password << "')";
            if(m_sql_pool->Insert(ss.str().c_str())) {
                m_ba->readFromFile(log_html);
                PUDGE_LOG_INFO(g_logger) << "user: " << user << " password: " << password << " register scuccessful!";
            } else {
                m_ba->readFromFile(register_error_html);
            }
        }
    } else {
        m_ba->readFromFile(register_error_html);
        PUDGE_LOG_INFO(g_logger) << "user: " << user << " password: " << password << " register failed!";
    }
    m_ba->setPosition(0);
    rsp->setFileBa(m_ba);
    rsp->setFileResponse(true);
    return 0; 
}

int32_t TinyWebserver::register_page(pudge::http::HttpRequest::ptr req
                ,pudge::http::HttpResponse::ptr rsp
                ,pudge::http::HttpSession::ptr session) {
    rsp->setVersion(17);
    rsp->setStatus((pudge::http::HttpStatus)200);
    rsp->setHeader("Content-Type", "text/html");
    pudge::ByteArray::ptr m_ba(new pudge::ByteArray);
    m_ba->readFromFile(register_html);
    m_ba->setPosition(0);
    rsp->setFileBa(m_ba);
    rsp->setFileResponse(true);
    return 0; 
}

/**
 * @brief: 检查是否有cookies 
 */
bool TinyWebserver::checkCookie(pudge::http::HttpRequest::ptr req) {
    std::string val = req->getCookie("user");
    PUDGE_LOG_INFO(g_logger) << "checkCookie val " << val;
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_userpass.find(val);
    m_mutex.unlock();
    return it == m_userpass.end() ? false : true;
}

int32_t TinyWebserver::check_log_page(pudge::http::HttpRequest::ptr req
                ,pudge::http::HttpResponse::ptr rsp
                ,pudge::http::HttpSession::ptr session) {
    rsp->setVersion(17);
    rsp->setStatus((pudge::http::HttpStatus)200);
    rsp->setHeader("Content-Type", "text/html");
    pudge::ByteArray::ptr m_ba(new pudge::ByteArray);

    std::string user = req->getParam("user");
    std::string password = req->getParam("password");
    
    if(user.size() && password.size()){
        if (m_userpass.find(user) == m_userpass.end()) {
            std::vector<std::vector<std::string>> res;
            std::stringstream ss;
            ss << "select passwd from user where username = '" << user << "'";
            if (!m_sql_pool->Query(ss.str().c_str(), res)) {
                m_ba->readFromFile(log_error_html);
            } else {
                if (!res.empty() && res[0][0] == password) {
                    auto server_ptr = shared_from_this();
                    /// 超时则取消登录状态
                    pudge::Timer::ptr timer = m_worker->addTimer(5000, [server_ptr, user] {
                        std::dynamic_pointer_cast<TinyWebserver>(server_ptr)->erase(user);
                    });
                    m_userpass[user] = password;
                    m_ba->readFromFile(welcome_html);  
                } else {
                    m_ba->readFromFile(log_error_html);  
                }
            }
        } else {
            if (m_userpass[user] == password) {
                m_ba->readFromFile(welcome_html);
            } else {
                m_ba->readFromFile(log_error_html);
            }
        }
    } else {
        m_ba->readFromFile(log_error_html);
    }
    m_ba->setPosition(0);
    rsp->setFileBa(m_ba);
    rsp->setFileResponse(true);
    return 0; 
}

/**
 * @brief: 处理http连接
 */
void TinyWebserver::handleClient(pudge::Socket::ptr client) {
    PUDGE_LOG_DEBUG(g_logger) << "handleClient " << *client;
    pudge::http::HttpSession::ptr session(new pudge::http::HttpSession(client));
    do {
        auto req = session->recvRequest();
        if(!req) {
            PUDGE_LOG_DEBUG(g_logger) << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " cliet:" << *client << " keep_alive=" << m_isKeepalive;
            break;
        }
        pudge::http::HttpResponse::ptr rsp(new pudge::http::HttpResponse(req->getVersion()
                            ,req->isClose() || !m_isKeepalive));
        
        rsp->setHeader("Server", getName());
        /// 处理 servlet 回调函数
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);
        /// 客户端一般都keep-alive
        if(!m_isKeepalive || req->isClose()) {
            break;
        }                    
    } while (true);
    session->close();
}

void run() {
    TinyWebserver::ptr server(new TinyWebserver(false));
    server->setName("pudge");

    PUDGE_LOG_INFO(g_logger) << "loading Config...";

    pudge::Config::LoadFromConfDir("/home/pudge/workspace/pudge-server/conf");

    pudge::Address::ptr addr = pudge::Address::LookupAnyIPAddress(g_host_addr->getValue());
    
    // pudge::ConfigVar<pudge::MysqlDefine>::ptr foo
    //      = pudge::Config::Lookup<pudge::MysqlDefine>("db");

    // PUDGE_LOG_INFO(g_logger) << foo->toString();

    while(!server->bind(addr)) {
        sleep(2);
    }

    server->start();
}

int main(int argc, char** argv){
    pudge::IOManager iom(1);
    iom.schedule(run);
    return 0;
}