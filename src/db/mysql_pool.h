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

public:
	std::string m_url;			 //主机地址
	std::string m_Port;		 //数据库端口号
	std::string m_User;		 //登陆数据库用户名
	std::string m_PassWord;	 //登陆数据库密码
	std::string m_DatabaseName; //使用数据库名
};

typedef pudge::Singleton<Mysql_pool> MysqlMgr;

}
