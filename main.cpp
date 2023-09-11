#include "u-toolbox/concurrency/thread.h"
#include "u-toolbox/log/u_log.h"
#include "u-toolbox/net/null_socket_server.h"
#include "u-toolbox/net/socket_server.h"
#include <iostream>
#include <memory>

using namespace tqcq;

int
main(int argc, char *argv[])
{
        std::unique_ptr<SocketServer> socket_server =
                std::unique_ptr<NullSocketServer>(new NullSocketServer());
        auto thread =
                std::unique_ptr<Thread>(new Thread(std::move(socket_server)));
        thread->PostDelayedTask([] { U_INFO("Hello, world!"); }, 1000);
        return 0;
}