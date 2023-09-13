//
// Created by tqcq on 2023/9/5.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_PHYSICAL_SOCKET_SERVER_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_PHYSICAL_SOCKET_SERVER_H_

#include "dispatcher.h"
#include "socket_server.h"
#include "u-toolbox/base/config.h"
#include "u-toolbox/concurrency/recursive_mutex.h"
#include "u-toolbox/net/sockets/physical_socket.h"
#include "u-toolbox/third_party/sigslot/sigslot.h"
#include <array>
#include <pcap/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#if defined(U_USE_EPOLL)
#include <sys/epoll.h>
#endif

#include <poll.h>

namespace tqcq {

// u-toolbox/net/dispatchers/signaler.h
class Signaler;

class PhysicalSocketServer : public SocketServer {
public:
        static constexpr size_t kNumEpollEvents = 128;
        PhysicalSocketServer();
        ~PhysicalSocketServer() override;

        Socket *CreateSocket(int family, int type) override;

        virtual Socket *WrapSocket(SOCKET);

        bool Wait(int64_t max_wait_duration, bool process_io) override;
        void WakeUp() override;

        void Add(Dispatcher *pdispatcher);
        void Remove(Dispatcher *pdispatcher);
        void Update(Dispatcher *pdispatcher);

private:
        bool WaitSelect(int cms_wait, bool process_io);

#if defined(U_USE_EPOLL)
        void AddEpoll(Dispatcher *dispatcher, uint64_t key);
        void RemoveEpoll(Dispatcher *dispatcher);
        void UpdateEpoll(Dispatcher *dispatcher, uint64_t key);
        bool WaitEpoll(int cms_wait);
        bool WaitPollOneDispatcher(int cms_wait, Dispatcher *dispatcher);
        bool WaitEpoll(int cms_wait, bool process_io);

        std::array<epoll_event, kNumEpollEvents> epoll_events_;
        const int epoll_fd_ = INVALID_SOCKET;
#endif

        static constexpr int kForeverMs = -1;

        uint64_t next_dispatcher_key_ U_GUARDED_BY(recursive_mutex_) = 0;
        std::unordered_map<uint64_t, Dispatcher *>
                dispatcher_by_key_ U_GUARDED_BY(recursive_mutex_);
        std::unordered_map<Dispatcher *, uint64_t>
                key_by_dispatcher_ U_GUARDED_BY(recursive_mutex_);
        std::vector<uint64_t> current_dispatcher_keys_;
        Signaler *signal_wakeup_;
        RecursiveMutex recursive_mutex_;
        // Mutex mutex_;
        bool flag_wait_;
        bool waiting_ = false;
};

}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_PHYSICAL_SOCKET_SERVER_H_
