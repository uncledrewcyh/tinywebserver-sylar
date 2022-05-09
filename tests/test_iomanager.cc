/*
 * @description: 
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <string.h>

#include "src/iomanager.h"
#include "src/log.h"

pudge::Logger::ptr g_logger = PUDGE_LOG_ROOT();

int sock = 0;

void test_fiber() {
    PUDGE_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    //sleep(3);

    //close(sock);
    //sylar::IOManager::GetThis()->cancelAll(sock);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "112.80.248.76", &addr.sin_addr.s_addr);

    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        PUDGE_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        pudge::IOManager::GetThis()->addEvent(sock, pudge::IOManager::READ, [](){
            PUDGE_LOG_INFO(g_logger) << "read callback";
        });
        pudge::IOManager::GetThis()->addEvent(sock, pudge::IOManager::WRITE, [](){
            PUDGE_LOG_INFO(g_logger) << "write callback";
            // close(sock);
            pudge::IOManager::GetThis()->cancelEvent(sock, pudge::IOManager::READ);
            close(sock);
        });
    } else {
        PUDGE_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }

}

void test1() {
    std::cout << "EPOLLIN=" << EPOLLIN
              << " EPOLLOUT=" << EPOLLOUT << std::endl;
    pudge::IOManager iom(3, false);
    iom.schedule(&test_fiber);
}

// sylar::Timer::ptr s_timer;
// void test_timer() {
//     sylar::IOManager iom(2);
//     s_timer = iom.addTimer(1000, [](){
//         static int i = 0;
//         SYLAR_LOG_INFO(g_logger) << "hello timer i=" << i;
//         if(++i == 3) {
//             s_timer->reset(2000, true);
//             //s_timer->cancel();
//         }
//     }, true);
// }

int main(int argc, char** argv) {
    test1();
    // test_timer();
    return 0;
}
