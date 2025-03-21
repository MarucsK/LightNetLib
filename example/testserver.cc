#include <mymuduo/TcpServer.h>
#include <mymuduo/logger.h>
#include <string>
#include <functional>

class EchoServer{
public:
    EchoServer(EventLoop *loop,
               const InetAddress &addr,
               const std::string &name)
            : server_(loop,addr,name)
            , loop_(loop)
    {
        // 注册回调函数
        // 此处仅注册ConnectionCallback与MessageCallback
        // 那么onConnection和onMessage传给TcpServer::connectionCallback_与messageCallback_
        // 在TcpServer::newConnection中, 又把它们设置给了TcpConnection::connectionCallback_与messageCallback_
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this,
                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        // 设置合适的loop线程数量
        server_.setThreadNum(3);
    }

    void start(){
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr &conn){
        if(conn->connected()){
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str());
        }
        else{
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
        }
    }
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time){
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown(); // 关闭写端 EPOLLHUP -> closeCallback
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main(){
    EventLoop loop; // baseloop
    InetAddress addr(8000);
    EchoServer server(&loop, addr, "EchoServer-01"); // Acceptor non-blocking listenfd  create bind
    server.start(); // listen loopthread listenfd => acceptChannel => mainloop =>
    loop.loop(); // 启动mainloop的底层poller
    return 0;
}