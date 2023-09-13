#include "u-toolbox/base/defer.h"
#include "u-toolbox/concurrency/thread.h"
#include "u-toolbox/log/u_log.h"
#include "u-toolbox/net/dispatchers/socket_dispatcher.h"
#include "u-toolbox/net/ip_address.h"
#include "u-toolbox/net/null_socket_server.h"
#include "u-toolbox/net/physical_socket_server.h"
#include "u-toolbox/net/socket_address.h"
#include "u-toolbox/net/sockets/physical_socket.h"
#include <iostream>
#include <memory>
#include <spdlog/spdlog.h>
#include <sys/socket.h>

using namespace tqcq;

char index_html_buffer[4096] = {0};
size_t index_html_length = 0;
char response_buffer[4096] = {0};
size_t response_length;

class HTTPServer : public PhysicalSocketServer, public sigslot::has_slots<> {
public:
        static constexpr int kMaxAcceptPerCall = 511;

        HTTPServer(IPAddress listen_addr, int listen_port)
            : listen_addr_(listen_addr),
              listen_port_(listen_port),
              client_count_(0)
        {}

        bool Initialize()
        {
                Socket *accept_socket = CreateSocket(AF_INET, SOCK_STREAM);
                accept_socket->SetOption(Socket::OPT_REUSEADDR, 1);
                SocketAddress bind_addr(listen_addr_, listen_port_);
                if (accept_socket->Bind(bind_addr) != 0) {
                        accept_socket->Close();
                        U_ERROR("Bind failed: {}, {}",
                                accept_socket->GetError(),
                                strerror(accept_socket->GetError()));
                        return false;
                }

                if (accept_socket->Listen(kMaxAcceptPerCall) != 0) {
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

                        client_socket->SignalReadEvent.connect(
                                this, &HTTPServer::OnRead);

                        client_socket->SignalCloseEvent.connect(
                                this, &HTTPServer::OnClose);

                        client_count_++;
                }
                U_INFO("Client connected: {}", client_count_);
        }

        void OnRead(Socket *socket)
        {
                U_ASSERT(socket != nullptr, "OnRead: socket is nullptr");
                static char buffer[4096];
                int ret = 0;
                int received = 0;
                do {
                        received += ret;
                        ret = socket->Recv(buffer, sizeof(buffer), nullptr);
                } while (ret > 0);

                if (received > 0) {
                        std::string str(buffer, buffer + received);
                        // U_INFO("Received: {} ", str);
                        socket->SignalWriteEvent.connect(this,
                                                         &HTTPServer::OnWrite);
                        /*
                        shutdown(static_cast<SocketDispatcher *>(socket)
                                         ->GetDescriptor(),
                                 SHUT_RD);
                         */
                        // U_INFO("OnRead: SignalWriteEvent.connect: {}", str);
                        OnWrite(socket);
                }

                if (ret == 0) {
                        U_INFO("Client Count: {}", client_count_--);
                        // socket->Close();
                        return;
                }
        }

        void OnWrite(Socket *socket)
        {
                int res = 0;
                int read_idx = 0;
                do {
                        res = socket->Send(response_buffer + read_idx,
                                           response_length - read_idx);
                        if (res > 0) {
                                read_idx += res;
                        }
                } while (res > 0);

                if (read_idx > 0) {
                        socket->SignalWriteEvent.disconnect(this);
                }
        }

        void OnClose(Socket *socket, int err)
        {
                // U_INFO("OnClose: err: {}", err);
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
        // write log to current dir log.txt
        auto logger = spdlog::rotating_logger_mt("file_logger", "log.txt",
                                                 1024 * 1024 * 5, 3);
        spdlog::set_default_logger(logger);
        spdlog::flush_every(std::chrono::seconds(3));
        defer(logger->flush());

        FILE *f = fopen("index.html", "r");
        if (f == nullptr) {
                U_ERROR("fopen failed");
                return -1;
        }
        int ret = 0;
        do {
                ret = fread(index_html_buffer, 1, sizeof(index_html_buffer), f);
                if (ret > 0) {
                        index_html_length += ret;
                }
        } while (ret > 0);
        fclose(f);
        U_INFO("index.html length: {}", index_html_length);

        snprintf(response_buffer, sizeof(response_buffer),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "Content-Length: %ld\r\n"
                 "\r\n",
                 index_html_length);

        size_t len = strlen(response_buffer);
        memcpy(response_buffer + len, index_html_buffer, index_html_length);
        response_length = len + index_html_length;
        U_INFO("header length: {}, response length: {}", len, response_length);

        std::unique_ptr<HTTPServer> socket_server = std::unique_ptr<HTTPServer>(
                new HTTPServer(IPAddress("0.0.0.0"), 8080));
        if (!socket_server->Initialize()) {
                U_ERROR("Initialize failed");
                return -1;
        }

        auto thread =
                std::unique_ptr<Thread>(new Thread(std::move(socket_server)));

        thread->Start();
        U_INFO("Server started");
        thread->Join();
        // thread->PostDelayedTask([] { U_INFO("Hello, world!"); }, 1000);

        return 0;
}