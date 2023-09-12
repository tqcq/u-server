#include "u-toolbox/concurrency/thread.h"
#include "u-toolbox/log/u_log.h"
#include "u-toolbox/net/dispatchers/socket_dispatcher.h"
#include "u-toolbox/net/ip_address.h"
#include "u-toolbox/net/null_socket_server.h"
#include "u-toolbox/net/physical_socket_server.h"
#include "u-toolbox/net/socket_address.h"
#include "u-toolbox/net/socket_server.h"
#include "u-toolbox/net/sockets/physical_socket.h"
#include <iostream>
#include <memory>
#include <sys/socket.h>

using namespace tqcq;

class HTTPServer : public PhysicalSocketServer, public sigslot::has_slots<> {
public:
        static constexpr int kMaxAcceptPerCall = 100;

        HTTPServer(IPAddress listen_addr, int listen_port)
            : listen_addr_(listen_addr),
              listen_port_(listen_port),
              client_count_(0)
        {}

        bool Initialize()
        {
                Socket *accept_socket = CreateSocket(AF_INET, SOCK_STREAM);
                SocketAddress bind_addr(listen_addr_, listen_port_);
                if (accept_socket->Bind(bind_addr) != 0) {
                        accept_socket->Close();
                        U_ERROR("Bind failed");
                        return false;
                }

                if (accept_socket->Listen(10) != 0) {
                        accept_socket->Close();
                        U_ERROR("Listen failed");
                        return false;
                }

                accept_socket->SignalReadEvent.connect(this,
                                                       &HTTPServer::OnAccept);
                return true;
        }

protected:
        void OnAccept(Socket *accept_socket)
        {
                U_ASSERT(accept_socket != nullptr,
                         "OnAccept: accept_socket is nullptr");
                for (int i = 0; i < kMaxAcceptPerCall; ++i) {
                        Socket *client_socket = accept_socket->Accept(nullptr);
                        if (!client_socket) {
                                if (accept_socket->GetError() == EINTR) {
                                        continue;
                                }
                                break;
                        }

                        client_count_++;
                        client_socket->SignalReadEvent.connect(
                                this, &HTTPServer::OnRead);

                        client_socket->SignalCloseEvent.connect(
                                this, &HTTPServer::OnClose);
                        if (client_count_ % 1000 == 0) {
                                U_INFO("Client connected: {}", client_count_);
                        }
                }
        }

        void OnRead(Socket *socket)
        {
                U_ASSERT(socket != nullptr, "OnRead: socket is nullptr");
                char buffer[1024];
                int ret = 0;
                int received = 0;
                do {
                        received += ret;
                        ret = socket->Recv(buffer, sizeof(buffer), nullptr);
                } while (ret > 0);

                if (ret == 0) {
                        // do nothing
                        // Close();
                }
                if (received > 0) {
                        std::string str(buffer, buffer + received);
                        // U_INFO("Received: {} ", str);
                        socket->SignalWriteEvent.connect(this,
                                                         &HTTPServer::OnWrite);
                        OnWrite(socket);
                }
        }

        void OnWrite(Socket *socket)
        {
                int res = socket->Send("HTTP/1.1 200 OK\r\nContent-Length: "
                                       "5\r\n\r\nHello",
                                       5);
                if (res > 0) {
                        socket->Close();
                }
        }

        void OnClose(Socket *socket, int err)
        {
                U_INFO("OnClose: err: {}", err);
                socket->Close();
        }

private:
        IPAddress listen_addr_;
        int listen_port_;
        int client_count_;
};

int
main(int argc, char *argv[])
{
        std::unique_ptr<HTTPServer> socket_server = std::unique_ptr<HTTPServer>(
                new HTTPServer(IPAddress("0.0.0.0"), 8080));
        if (!socket_server->Initialize()) {
                U_ERROR("Initialize failed");
                return -1;
        }

        auto thread =
                std::unique_ptr<Thread>(new Thread(std::move(socket_server)));

        thread->Start();
        thread->Join();
        // thread->PostDelayedTask([] { U_INFO("Hello, world!"); }, 1000);
        return 0;
}