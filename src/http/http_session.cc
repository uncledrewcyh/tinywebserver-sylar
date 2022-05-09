#include "http_session.h"
#include "http_parser.h"
#
namespace pudge {

namespace http{
HttpSession::HttpSession(Socket::ptr sock, bool owner)
    :SocketStream(sock, owner) {
}

HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    ///智能指针buffer数组
    std::shared_ptr<char> buffer(new char[buff_size], [](char* ptr) {
        delete[] ptr;
    });
    ///获取raw指针
    char* data = buffer.get();
    ///解析后剩余的长度
    int offset = 0;
    do {
        ///拼接到剩余长度后
        int len = read(data + offset, buff_size - offset);
        if (len <= 0) {
            close();
            return nullptr;
        }
        ///新读取的长度 + 剩余长度 = 需要解析的长度
        len += offset;
        ///每解析一部分都覆盖掉之前的部分，节约空间
        size_t nparse = parser->execute(data, len);
        if (parser->hasError()) {
            close();
            return nullptr;
        }
        ///得到剩余长度
        offset = len - nparse;
        if (offset == (int)(buff_size)) {
            close();
            return nullptr;
        }
        if (parser->isFinished()) {
            break;
        }
    } while(true);
    ///读取body长度
    int64_t length = parser->getContentLength();
    if (length > 0) {
        std::string body;
        body.resize(length);

        int len = 0;
        if (length >= offset) {
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
        }
        length -= offset;
        ///如果还没读满，则继续读
        if (length > 0) {
            if (readFixSize(&body[len], length) <= 0) {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }
    ///设置是否为长短连接
    parser->getData()->init();
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    int rt1 = writeFixSize(data.c_str(), data.size());
    int rt2 = 0;
    if (rsp->getFileResponse()) {
        rt2 =  writeFixSize(rsp->getFileBa(), rsp->getFileBa()->getReadSize());
    }
    return rt1 + rt2;
}

}
}