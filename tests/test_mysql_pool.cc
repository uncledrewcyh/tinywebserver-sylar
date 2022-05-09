#include "src/db/mysql_pool.h"
#include <sstream>

pudge::Mysql_pool *mysql_pool = PUDGE_MYSQL_ROOT();

int main() {
    std::vector<std::vector<std::string>> res;
    std::stringstream ss;
    ss << "select passwd from user where username = '" << "cyh" << "'";
    mysql_pool->Query(ss.str().c_str(), res);
    //INSERT INTO user(username, passwd) VALUES('name', 'passwd')
    // ss << "insert into user(username, passwd) values('" << "cyh" <<"', '" << "123" << "')";
    // std::cout << ss.str() << std::endl;
    // if(!mysql_pool->Insert(ss.str().c_str())) {
    //     std::cout << "error: insert into users" << std::endl;
    // }
    for (auto& i: res) {
        for (auto& j: i) {
            std::cout << j << std::endl;
        }
    }
    return 0;
}