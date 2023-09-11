//
// Created by tqcq on 2023/9/6.
//

#include "null_socket_server.h"

namespace tqcq {
NullSocketServer::NullSocketServer() = default;

NullSocketServer::~NullSocketServer() = default;

bool
NullSocketServer::Wait(int64_t max_wait_duration, bool process_io)
{
        event_.Wait(max_wait_duration);
        return true;
}

void
NullSocketServer::WakeUp()
{
        event_.Set();
}

Socket *
NullSocketServer::CreateSocket(int family, int type)
{
        return nullptr;
}
}// namespace tqcq
