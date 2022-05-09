#include "src/http/http_server.h"
#include "src/log.h"
#include "src/mutex.h"
#include "src/bytearray.h"
#include "src/db/mysql_pool.h"

class TinyWebserver : public pudge::http::HttpServer {
public: 
    typedef pudge::RWMutex RWMutexType;
    typedef pudge::http::FunctionServlet::callback callback;
    typedef std::shared_ptr<TinyWebserver> ptr;

    TinyWebserver(bool keepalive = false
               ,pudge::IOManager* worker = pudge::IOManager::GetThis()
               ,pudge::IOManager* io_worker = pudge::IOManager::GetThis()
               ,pudge::IOManager* accept_worker = pudge::IOManager::GetThis());

    virtual void handleClient(pudge::Socket::ptr client) override;


    ///定义
    #define XX(fun) \
    int32_t fun(pudge::http::HttpRequest::ptr req \
                    ,pudge::http::HttpResponse::ptr rsp \
                    ,pudge::http::HttpSession::ptr session);
        XX(main_page)
        XX(register_page)
        XX(log_page)
        XX(check_register_page)
        XX(check_log_page)
        XX(picture_1_page)
        XX(video_1_page)
        XX(video_1_resource)
        XX(picture_1_resource)
        XX(favicon_1_resource)
    #undef XX

    /**
     * @brief: 判断是否含有cookies
     */    
    bool checkCookie(pudge::http::HttpRequest::ptr req);

    /**
     * @brief: 删除登陆中的用户 
     */    
    void erase(std::string user) { m_userpass.erase(user); }

private:
    ///存放body文件
    std::unordered_map<std::string, std::string> m_userpass;
    RWMutexType m_mutex;
    pudge::ByteArray::ptr m_ba;
    pudge::Mysql_pool *m_sql_pool;
};

