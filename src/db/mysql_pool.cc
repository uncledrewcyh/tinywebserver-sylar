#include "mysql_pool.h"


static pudge::Logger::ptr g_logger = PUDGE_LOG_NAME("system");

namespace pudge{



static pudge::ConfigVar<MysqlDefine>::ptr g_mysql_define
         = pudge::Config::Lookup("db", MysqlDefine(), "mysql config");

struct MysqlInit {
    MysqlInit() {
        g_mysql_define->addListener([](const MysqlDefine& old_value, const MysqlDefine& new_value) {
            MysqlMgr::GetInstance()-> init(g_mysql_define->getValue().url, g_mysql_define->getValue().user, g_mysql_define->getValue().password, 
                                            g_mysql_define->getValue().databasename, g_mysql_define->getValue().port,  g_mysql_define->getValue().maxconn);
            PUDGE_LOG_INFO(g_logger) << "Mysql config have been changed...";
        });
        // PUDGE_LOG_INFO(g_logger) << "Mysqlinit";
        // PUDGE_LOG_INFO(g_logger) << g_mysql_define->toString();
    }
};

static MysqlInit _mysqlinit;

Mysql_pool::Mysql_pool()
            : m_CurConn(0)
            , m_FreeConn(0) {           
    init(g_mysql_define->getValue().url, g_mysql_define->getValue().user, g_mysql_define->getValue().password, 
    g_mysql_define->getValue().databasename, g_mysql_define->getValue().port,  g_mysql_define->getValue().maxconn);
}

Mysql_pool::~Mysql_pool() {
    DestroyPool();
}

MYSQL *Mysql_pool::GetConnection() {
    MYSQL* con = NULL;
    if(connList.size() == 0)
        return NULL;

    sem_wait(&reserve);
    lock.lock();

    con = connList.front();
    connList.pop_front();

    --m_FreeConn;
    ++m_CurConn;

    lock.unlock();
    return con;
}	

bool Mysql_pool::ReleaseConnection(MYSQL *conn) {
    if(conn == NULL)
        return false;
    lock.lock();

    connList.push_back(conn);

    ++m_FreeConn;
    --m_CurConn;

    lock.unlock();

    sem_post(&reserve);
    return true;
}

int Mysql_pool::GetFreeConn() {
    return m_FreeConn;
}	

void Mysql_pool::DestroyPool() {
    lock.lock();
    std::list<MYSQL*>::iterator it;
    for(it = connList.begin(); it != connList.end(); it++) {
        MYSQL *con = *it;
        mysql_close(con);
    }

    m_CurConn = 0;
    m_FreeConn = 0;
    connList.clear();

    lock.unlock();
}			

void Mysql_pool::init(std::string url, std::string User, std::string PassWord, std::string DataBaseName
                        , int Port, int MaxConn) {
    m_url = url;
	m_Port = Port;
	m_User = User;
	m_PassWord = PassWord;
	m_DatabaseName = DataBaseName;

    ///先销毁
    DestroyPool();
	for (int i = 0; i < MaxConn; i++)
	{
		MYSQL *con = NULL;
		con = mysql_init(con);

		if (con == NULL)
		{
			PUDGE_LOG_ERROR(g_logger) << "MySQL Error: mysql_init";
			exit(1);
		}
		con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DataBaseName.c_str(), Port, NULL, 0);

		if (con == NULL)
		{
			PUDGE_LOG_ERROR(g_logger) << "MySQL Error: mysql_real_connect";
			exit(1);
		}
		connList.push_back(con);
		++m_FreeConn;
	}

    sem_t cur;
    sem_init(&cur, 0, m_FreeConn);
	reserve = cur;

	m_MaxConn = m_FreeConn;
}

bool Mysql_pool::Query(const char* ch, std::vector<std::vector<std::string>>& out) {
    MYSQL* con = NULL;
    con = GetConnection();
    if(!con){
        PUDGE_LOG_ERROR(g_logger) << "MySQL Error: no connection";
        return false;
    }
    
    if(mysql_query(con, ch)) {
        PUDGE_LOG_ERROR(g_logger) << "MySQL Error: mysql_query" << "----error: " << mysql_error(con);
        return false;
    }

    MYSQL_RES* result = mysql_store_result(con);

    int num_fields = mysql_num_fields(result);
    // PUDGE_LOG_INFO(g_logger) << " mysql_query call...";
    while(MYSQL_ROW row = mysql_fetch_row(result)) {
        std::vector<std::string> cur;
        for(int i = 0; i < num_fields; i++) {
            cur.push_back(std::string(row[i]));
        }
        out.push_back(cur);
    }
    ReleaseConnection(con);
    return true;
}

bool Mysql_pool::Insert(const char* ch) {
    MYSQL* con = NULL;
    con = GetConnection();
    if(!con){
        PUDGE_LOG_ERROR(g_logger) << "MySQL Error: no connection";
        return false;
    }

    if(mysql_query(con, ch)) {
        PUDGE_LOG_ERROR(g_logger) << "MySQL Error: mysql_query" << "----error: " << mysql_error(con);
        return false;
    }
    ReleaseConnection(con);
    return true;
}

bool Mysql_pool::Delete(const char* ch) {
    MYSQL* con = NULL;
    con = GetConnection();
    if(!con){
        PUDGE_LOG_ERROR(g_logger) << "MySQL Error: no connection";
        return false;
    }

    if(mysql_query(con, ch)) {
        PUDGE_LOG_ERROR(g_logger) << "MySQL Error: mysql_query" << "----error: " << mysql_error(con);
        return false;
    }
    ReleaseConnection(con);
    return true;
}

bool Mysql_pool::Update(const char* ch) {
    MYSQL* con = NULL;
    con = GetConnection();
    if(!con){
        PUDGE_LOG_ERROR(g_logger) << "MySQL Error: no connection";
        return false;
    }

    if(mysql_query(con, ch)) {
        PUDGE_LOG_ERROR(g_logger) << "MySQL Error: mysql_query" << "----error: " << mysql_error(con);
        return false;
    }
    ReleaseConnection(con);
    return true;
}

}