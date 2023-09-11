//
// Created by tqcq on 2023/9/5.
//

#include "physical_socket_server.h"

namespace tqcq {
PhysicalSocketServer::PhysicalSocketServer() {}

PhysicalSocketServer::~PhysicalSocketServer() {}

Socket *
PhysicalSocketServer::CreateSocket(int family, int type)
{
        return nullptr;
}

Socket *
PhysicalSocketServer::WrapSocket(int)
{
        return nullptr;
}

bool
PhysicalSocketServer::Wait(int64_t max_wait_duration, bool process_io)
{
        return false;
}

void
PhysicalSocketServer::WakeUp()
{}

void
PhysicalSocketServer::Add(Dispatcher *dispatcher)
{}

void
PhysicalSocketServer::Remove(Dispatcher *dispatcher)
{}

void
PhysicalSocketServer::Update(Dispatcher *dispatcher)
{}
}// namespace tqcq
