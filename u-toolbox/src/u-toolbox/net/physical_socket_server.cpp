//
// Created by tqcq on 2023/9/5.
//

#include "physical_socket_server.h"
#include "dispatcher.h"
#include "u-toolbox/concurrency/lock_guard.h"
#include "u-toolbox/concurrency/unlock_guard.h"
#include "u-toolbox/log/u_log.h"
#include "u-toolbox/net/dispatchers/signaler.h"
#include "u-toolbox/net/dispatchers/socket_dispatcher.h"
#include "u-toolbox/system/time_utils.h"

#if defined(WEBRTC_USE_EPOLL)
// POLLRDHUP / EPOLLRDHUP are only defined starting with Linux 2.6.17.
#if !defined(POLLRDHUP)
#define POLLRDHUP 0x2000
#endif
#if !defined(EPOLLRDHUP)
#define EPOLLRDHUP 0x2000
#endif
#endif

namespace tqcq {

class ScopedSetTrue {
public:
        ScopedSetTrue(bool *value) : value_(value)
        {
                U_ASSERT(value_ != nullptr, "ScopedSetTrue: value_ is nullptr");
                *value_ = true;
        }

        ~ScopedSetTrue() { *value_ = false; }

private:
        bool *value_;
};

PhysicalSocketServer::PhysicalSocketServer() : flag_wait_(false)
{
        signal_wakeup_ = new Signaler(this, flag_wait_);
}

PhysicalSocketServer::~PhysicalSocketServer() { delete signal_wakeup_; }

Socket *
PhysicalSocketServer::CreateSocket(int family, int type)
{
        SocketDispatcher *dispatcher = new SocketDispatcher(this);
        if (dispatcher->Create(family, type)) {
                return dispatcher;
        } else {
                delete dispatcher;
                return nullptr;
        }
}

Socket *
PhysicalSocketServer::WrapSocket(SOCKET s)
{
        SocketDispatcher *dispatcher = new SocketDispatcher(s, this);
        if (dispatcher->Initialize()) {
                return dispatcher;
        } else {
                delete dispatcher;
                return nullptr;
        }
}

bool
PhysicalSocketServer::Wait(int64_t max_wait_duration, bool process_io)
{
        ScopedSetTrue s(&waiting_);
        const int64_t cms_wait =
                max_wait_duration == kForever ? kForeverMs : max_wait_duration;
        return WaitSelect(cms_wait, process_io);
}

void
PhysicalSocketServer::WakeUp()
{
        signal_wakeup_->Signal();
}

void
PhysicalSocketServer::Add(Dispatcher *dispatcher)
{
        LockGuard<RecursiveMutex> lock(recursive_mutex_);
        if (key_by_dispatcher_.count(dispatcher)) {
                U_WARN("dispatcher already added");
                return;
        }

        uint64_t key = next_dispatcher_key_++;
        dispatcher_by_key_.emplace(key, dispatcher);
        key_by_dispatcher_.emplace(dispatcher, key);
}

void
PhysicalSocketServer::Remove(Dispatcher *dispatcher)
{
        LockGuard<RecursiveMutex> lock(recursive_mutex_);
        if (!key_by_dispatcher_.count(dispatcher)) {
                U_WARN("dispatcher not added");
                return;
        }

        uint64_t key = key_by_dispatcher_.at(dispatcher);
        key_by_dispatcher_.erase(dispatcher);
        dispatcher_by_key_.erase(key);
}

void
PhysicalSocketServer::Update(Dispatcher *dispatcher)
{}

static void
ProcessEvents(Dispatcher *dispatcher,
              bool readable,
              bool writeable,
              bool error_event,
              bool check_error)
{
        int errcode = 0;
        if (check_error) {
                socklen_t len = sizeof(errcode);
                int res = ::getsockopt(dispatcher->GetDescriptor(), SOL_SOCKET,
                                       SO_ERROR, &errcode, &len);

                if (res < 0) {
                        if (error_event || errno != ENOTSOCK) {
                                errcode = EBADF;
                        }
                }
        }

        const uint32_t requested_events = dispatcher->GetRequestedEvents();
        uint32_t ff = 0;
        if (readable) {
                if (errcode || dispatcher->IsDescriptorClosed()) {
                        ff |= DE_CLOSE;
                } else if (requested_events & DE_ACCEPT) {
                        ff |= DE_ACCEPT;
                } else {
                        ff |= DE_READ;
                }
        }

        if (writeable) {
                if (requested_events & DE_CONNECT) {
                        if (!errcode) {
                                ff |= DE_CONNECT;
                        }
                } else {
                        ff |= DE_WRITE;
                }
        }

        if (errcode) {
                ff |= DE_CLOSE;
        }

        if (ff != 0) {
                dispatcher->OnEvent(ff, errcode);
        }
}

#if defined(U_USE_EPOLL) || defined(U_USE_POLL)
static void
ProcessPollEvents(Dispatcher *dispatcher, const pollfd &pfd)
{
        bool readable = (pfd.revents & (POLLIN | POLLPRI));
        bool writeable = (pfd.revents & POLLOUT);
        bool error = (pfd.revents & (POLLRDHUP | POLLERR | POLLHUP));

        ProcessEvents(dispatcher, readable, writeable, error, error);
}

#endif

bool
PhysicalSocketServer::WaitSelect(int cms_wait, bool process_io)
{
        struct timeval *ptv_wait = nullptr;
        struct timeval tv_wait;
        int64_t stop_us;
        if (cms_wait != kForeverMs) {
                tv_wait.tv_sec = cms_wait / 1000;
                tv_wait.tv_usec = (cms_wait % 1000) * 1000;
                ptv_wait = &tv_wait;

                stop_us = TimeMicros() + cms_wait * 1000;
        }

        fd_set fds_read;
        fd_set fds_write;

        flag_wait_ = true;
        while (flag_wait_) {
                FD_ZERO(&fds_read);
                FD_ZERO(&fds_write);
                int fdmax = -1;
                {
                        LockGuard<RecursiveMutex> lock(recursive_mutex_);
                        for (auto const &kv : dispatcher_by_key_) {
                                uint64_t key = kv.first;
                                Dispatcher *pdispatcher = kv.second;
                                if (!process_io
                                    && (pdispatcher != signal_wakeup_)) {
                                        continue;
                                }

                                current_dispatcher_keys_.push_back(key);
                                int fd = pdispatcher->GetDescriptor();

                                U_ASSERT(fd <= FD_SETSIZE, "fd is too big");
                                fdmax = std::max(fd, fdmax);

                                uint32_t ff = pdispatcher->GetRequestedEvents();
                                if (ff & (DE_READ | DE_ACCEPT)) {
                                        FD_SET(fd, &fds_read);
                                }
                                if (ff & (DE_WRITE | DE_CONNECT)) {
                                        FD_SET(fd, &fds_write);
                                }
                        }
                }

                int n = select(fdmax + 1, &fds_read, &fds_write, nullptr,
                               ptv_wait);
                if (n < 0) {
                        if (errno != EINTR) {
                                U_ERROR("select failed: %d, %s", errno,
                                        strerror(errno));
                                return false;
                        }
                } else if (n == 0) {
                        // time out
                        return true;
                } else {
                        LockGuard<RecursiveMutex> lock(recursive_mutex_);
                        for (uint64_t key : current_dispatcher_keys_) {
                                if (!dispatcher_by_key_.count(key)) {
                                        continue;
                                }

                                Dispatcher *pdispatcher =
                                        dispatcher_by_key_.at(key);
                                int fd = pdispatcher->GetDescriptor();
                                bool readable = FD_ISSET(fd, &fds_read);
                                if (readable) {
                                        FD_CLR(fd, &fds_read);
                                }

                                bool writeable = FD_ISSET(fd, &fds_write);
                                if (writeable) {
                                        FD_CLR(fd, &fds_write);
                                }

                                ProcessEvents(pdispatcher, readable, writeable,
                                              false, readable || writeable);
                        }
                }
        }
        return true;
}

}// namespace tqcq
