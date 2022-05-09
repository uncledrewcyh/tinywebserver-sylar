#pragma once

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <string>
#include "../mutex.h"
#include "../singleton.h"
#include "../log.h"
#include "../config.h"

#define PUDGE_MYSQL_ROOT() pudge::MysqlMgr::GetInstance()

namespace pudge{

class Mysql_pool
{
public:
	static Mysql_pool* GetInstance();
	MYSQL *GetConnection();				 //获取数据库连接
	bool ReleaseConnection(MYSQL *conn); //释放连接
	int GetFreeConn();					 //获取连接
	void DestroyPool();					 //销毁所有连接

	void init(std::string url, std::string User, std::string PassWord
            , std::string DataBaseName, int Port, int MaxConn); 

    bool Query(const char* ch, std::vector<std::vector<std::string>>& out);
    bool Insert(const char* ch);
    bool Delete(const char* ch);
    bool Update(const char* ch);

	Mysql_pool();
	~Mysql_pool();
private:
	int m_MaxConn;  //最大连接数
	int m_CurConn;  //当前已使用的连接数
	int m_FreeConn; //当前空闲的连接数
	Mutex lock;
	std::list<MYSQL *> connList; //连接池
	sem_t reserve;

	std::string m_url;			 //主机地址
	std::string m_Port;		 //数据库端口号
	std::string m_User;		 //登陆数据库用户名
	std::string m_PassWord;	 //登陆数据库密码
	std::string m_DatabaseName; //使用数据库名
};

typedef pudge::Singleton<Mysql_pool> MysqlMgr;
/**
 * @brief: 数据库定义格式
 */
struct MysqlDefine {
    std::string url = "localhost";
    std::string user = "root";
    std::string password = "root";
    std::string databasename = "yourdb";
    int port = 3306;
    int maxconn = 10;

    bool operator==(const MysqlDefine& oth) const {
        return url == oth.url
            && user == oth.user
            && password == oth.password
            && databasename == oth.databasename
            && port == oth.port
            && maxconn == oth.maxconn;
    }

    bool operator<(const MysqlDefine& oth) const {
        return user < oth.user;
    }

    bool isValid() const {
        return !url.empty();
    }
};

/**
 * @brief: 字符串转换为日志定义类的
 * @details: 函数模板特例化，模板参数为空 
 */
template<>
class LexicalCast<std::string, MysqlDefine> {
public:
    MysqlDefine operator()(const std::string& v) {
        YAML::Node n = YAML::Load(v);
        MysqlDefine md;
        
    #define XX(var, type) \
        using std::string;\
        if (!n[#var].IsDefined()) { \
            std::cout << "log config error: name is null, " << n \
                      << std::endl; \
            std::logic_error("mysql config " #var " is null"); \
        } \
        md.var = n[#var].as<type>(); 
        XX(url, string);
        XX(user, string);
        XX(password, string);
        XX(databasename, string);
        XX(port, int);
        XX(maxconn, int);
    #undef XX  
        return md;
    }
};

/**
 * @brief: 日志定义类转换为字符串
 * @details: 函数模板特例化，模板参数为空 
 */
template<>
class LexicalCast<MysqlDefine, std::string> {
public:
    std::string operator()(const MysqlDefine& i) {
        YAML::Node n;
        // n["url"] = i.url;
    #define XX(var) \
        n[#var] = i.var; 

        XX(url);
        XX(user);
        XX(password);
        XX(databasename);
        XX(port);
        XX(maxconn);
    #undef XX          
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};

}
