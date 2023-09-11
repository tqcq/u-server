//
// Created by tqcq on 2023/9/5.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_PHYSICAL_SOCKET_SERVER_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_PHYSICAL_SOCKET_SERVER_H_

#include "dispatcher.h"
#include "socket_server.h"
#include "u-toolbox/net/sockets/physical_socket.h"
#include "u-toolbox/third_party/sigslot/sigslot.h"
#include <array>
#include <pcap/socket.h>

#ifdef __linux__
#define U_USE_EPOLL
#else
#define U_USE_POLL
#endif

#if defined(U_USE_EPOLL)
#include <sys/epoll.h>
#endif

#if defined(U_USE_POLL)
#include <poll.h>
#endif

namespace tqcq {

class PhysicalSocketServer : public SocketServer {
public:
        static constexpr size_t kNumEpollEvents = 128;
        PhysicalSocketServer();
        ~PhysicalSocketServer() override;

        Socket *CreateSocket(int family, int type) override;

        virtual Socket *WrapSocket(SOCKET);

        bool Wait(int64_t max_wait_duration, bool process_io) override;
        void WakeUp() override;

        void Add(Dispatcher *dispatcher);
        void Remove(Dispatcher *dispatcher);
        void Update(Dispatcher *dispatcher);

private:
#if defined(USE_EPOLL)
        bool WaitEpoll(int cms_wait, bool process_io);
        std::array<epoll_event, kNumEpollEvents> epoll_events;
#endif

#if defined(USE_POLL)
        bool WaitPoll(int cms_wait, bool process_io);
        std::array<pollfd, kNumEpollEvents> poll_events;
#endif

        static constexpr int kForeverMs = -1;

        uint64_t next_dispatcher_key_;
        bool flag_wait_;
        bool waitint_ = false;
};

class SocketDispatcher : public Dispatcher, public PhysicalSocket {
public:
        explicit SocketDispatcher(PhysicalSocketServer *ss);
        SocketDispatcher(SOCKET s, PhysicalSocketServer *ss);
        ~SocketDispatcher() override;

        bool Initialize();

        virtual bool Create(int type);
        bool Create(int family, int type) override;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_PHYSICAL_SOCKET_SERVER_H_
