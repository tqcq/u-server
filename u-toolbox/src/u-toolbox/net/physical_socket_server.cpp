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
#include <sys/socket.h>

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

PhysicalSocketServer::PhysicalSocketServer()
    :
#if defined(U_USE_EPOLL)
      epoll_fd_(epoll_create(FD_SETSIZE)),
#endif
      flag_wait_(false)
{
#if defined(U_USE_EPOLL)
        if (epoll_fd_ == INVALID_SOCKET) {
                U_ERROR("epoll_create failed: {}, {}", errno, strerror(errno));
        }
#endif
        signal_wakeup_ = new Signaler(this, flag_wait_);
}

PhysicalSocketServer::~PhysicalSocketServer()
{
        delete signal_wakeup_;
#if defined(U_USE_EPOLL)
        if (epoll_fd_ != INVALID_SOCKET) {
                close(epoll_fd_);
        }
#endif
}

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

#if defined(U_USE_EPOLL)
        if (!process_io) {
                return WaitPollOneDispatcher(cms_wait, signal_wakeup_);
        } else if (epoll_fd_ != INVALID_SOCKET) {
                return WaitEpoll(cms_wait);
        }

        return false;
#endif
        return WaitSelect(cms_wait, process_io);
}

void
PhysicalSocketServer::WakeUp()
{
        signal_wakeup_->Signal();
}

void
PhysicalSocketServer::Add(Dispatcher *pdispatcher)
{
        LockGuard<RecursiveMutex> lock(recursive_mutex_);
        if (key_by_dispatcher_.count(pdispatcher)) {
                U_WARN("dispatcher already added");
                return;
        }

        uint64_t key = next_dispatcher_key_++;
        dispatcher_by_key_.emplace(key, pdispatcher);
        key_by_dispatcher_.emplace(pdispatcher, key);

#if defined(U_USE_EPOLL)
        if (epoll_fd_ != INVALID_SOCKET) {
                AddEpoll(pdispatcher, key);
        }
#endif
}

void
PhysicalSocketServer::Remove(Dispatcher *pdispatcher)
{
        LockGuard<RecursiveMutex> lock(recursive_mutex_);
        if (!key_by_dispatcher_.count(pdispatcher)) {
                U_WARN("dispatcher not added");
                return;
        }

        uint64_t key = key_by_dispatcher_.at(pdispatcher);
        key_by_dispatcher_.erase(pdispatcher);
        dispatcher_by_key_.erase(key);
#if defined(U_USE_EPOLL)
        if (epoll_fd_ != INVALID_SOCKET) {
                RemoveEpoll(pdispatcher);
        }
#endif
}

void
PhysicalSocketServer::Update(Dispatcher *dispatcher)
{
#if defined(U_USE_EPOLL)
        if (epoll_fd_ == INVALID_SOCKET) {
                return;
        }

        LockGuard<RecursiveMutex> lock(recursive_mutex_);
        if (!key_by_dispatcher_.count(dispatcher)) {
                return;
        }

        UpdateEpoll(dispatcher, key_by_dispatcher_.at(dispatcher));
#endif
}

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

#if defined(U_USE_EPOLL)
inline static int
GetEpollEvents(uint32_t ff)
{
        int events = 0;
        if (ff & (DE_READ | DE_ACCEPT)) {
                events |= EPOLLIN;
        }

        if (ff & (DE_WRITE | DE_CONNECT)) {
                events |= EPOLLOUT;
        }
        return events;
}

void
PhysicalSocketServer::AddEpoll(Dispatcher *dispatcher, uint64_t key)
{
        U_ASSERT(epoll_fd_ != INVALID_SOCKET, "epoll_fd_ is invalid");
        int fd = dispatcher->GetDescriptor();
        U_ASSERT(fd != INVALID_SOCKET, "fd is invalid");
        if (fd == INVALID_SOCKET) {
                return;
        }

        struct epoll_event event = {0};
        event.events = GetEpollEvents(dispatcher->GetRequestedEvents());
        if (event.events == 0u) {
                return;
        }

        event.data.u64 = key;
        int err = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event);
        U_ASSERT(err == 0, "epoll_ctl add failed: {}, {}", errno,
                 strerror(errno));
}

void
PhysicalSocketServer::RemoveEpoll(Dispatcher *dispatcher)
{
        U_ASSERT(epoll_fd_ != INVALID_SOCKET, "epoll_fd_ is invalid");
        int fd = dispatcher->GetDescriptor();
        U_ASSERT(fd != INVALID_SOCKET, "fd is invalid");
        if (fd == INVALID_SOCKET) {
                return;
        }

        struct epoll_event event = {0};
        int err = epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &event);
        U_ASSERT(err == 0 || errno == ENOENT, "assert epoll_ctl del: {}, {}",
                 errno, strerror(errno));
        if (err == -1 && errno != ENOENT) {
                U_ERROR("epoll_ctl del failed: {}, {}", errno, strerror(errno));
        }
}

void
PhysicalSocketServer::UpdateEpoll(Dispatcher *dispatcher, uint64_t key)
{
        U_ASSERT(epoll_fd_ != INVALID_SOCKET, "epoll_fd_ is invalid");
        int fd = dispatcher->GetDescriptor();
        U_ASSERT(fd != INVALID_SOCKET, "fd is invalid");
        if (fd == INVALID_SOCKET) {
                return;
        }

        struct epoll_event event = {0};
        event.events = GetEpollEvents(dispatcher->GetRequestedEvents());
        event.data.u64 = key;

        if (event.events == 0u) {
                epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
        } else {
                int err = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
                U_ASSERT(err == 0, "epoll_ctl mod failed: {}, {}", errno,
                         strerror(errno));
                if (err == -1) {
                        if (errno == ENOENT) {
                                epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event);
                                if (err == -1) {
                                        U_ERROR("epoll_ctl add failed: {}, {}",
                                                errno, strerror(errno));
                                }
                        } else {
                                U_ERROR("epoll_ctl mod failed: {}, {}", errno,
                                        strerror(errno));
                        }
                }
        }
}

bool
PhysicalSocketServer::WaitEpoll(int cms_wait)
{
        int64_t ms_wait = -1;
        int64_t ms_stop = -1;

        if (cms_wait != kForeverMs) {
                ms_wait = cms_wait;
                ms_stop = TimeAfter(cms_wait);
        }

        flag_wait_ = true;
        while (flag_wait_) {
                int n = epoll_wait(epoll_fd_, epoll_events_.data(),
                                   epoll_events_.size(),
                                   static_cast<int>(ms_wait));

                if (n < 0) {
                        if (errno != EINTR) {
                                U_ERROR("epoll_wait failed: {}, {}", errno,
                                        strerror(errno));
                                return false;
                        }
                } else if (n == 0) {
                        // timeout, do nothing
                        return true;
                } else {
                        LockGuard<RecursiveMutex> lock(recursive_mutex_);
                        for (int i = 0; i < n; ++i) {
                                const epoll_event &event = epoll_events_[i];
                                uint64_t key = event.data.u64;
                                if (!dispatcher_by_key_.count(key)) {
                                        continue;
                                }

                                Dispatcher *pdispatcher =
                                        dispatcher_by_key_.at(key);

                                bool readable =
                                        (event.events & (EPOLLIN | EPOLLPRI));
                                bool writable = (event.events & EPOLLOUT);
                                bool error =
                                        (event.events
                                         & (EPOLLRDHUP | EPOLLERR | EPOLLHUP));

                                ProcessEvents(pdispatcher, readable, writable,
                                              error, error);
                        }
                }

                if (cms_wait != kForeverMs) {
                        ms_wait = TimeDiff(ms_stop, TimeMicros());
                        if (ms_wait <= 0) {
                                return true;
                        }
                }
        }

        return true;
}

bool
PhysicalSocketServer::WaitPollOneDispatcher(int cms_wait,
                                            Dispatcher *dispatcher)
{
        int64_t ms_wait = -1;
        int64_t ms_stop = -1;
        if (cms_wait != kForeverMs) {
                ms_wait = cms_wait;
                ms_stop = TimeAfter(cms_wait);
        }

        flag_wait_ = true;
        const int fd = dispatcher->GetDescriptor();

        while (flag_wait_) {
                pollfd fds{
                        .fd = dispatcher->GetDescriptor(),
                        .events = 0,
                        .revents = 0,
                };
                uint32_t ff = dispatcher->GetRequestedEvents();
                if (ff & (DE_READ | DE_ACCEPT)) {
                        fds.events |= POLLIN;
                }

                if (ff & (DE_WRITE | DE_CONNECT)) {
                        fds.events |= POLLOUT;
                }

                int n = poll(&fds, 1, static_cast<int>(ms_wait));
                if (n < 0) {
                        if (errno != EINTR) {
                                U_ERROR("poll failed: {}, {}", errno,
                                        strerror(errno));
                                return false;
                        }
                } else if (n == 0) {
                        // timeout, do nothing
                        return true;
                } else {
                        U_ASSERT(n == 1, "poll return value is not 1");
                        U_ASSERT(fds.fd == fd, "poll return fd is not equal");
                        ProcessPollEvents(dispatcher, fds);
                }

                if (cms_wait != kForeverMs) {
                        ms_wait = TimeDiff(ms_stop, TimeMicros());
                        if (ms_wait < 0) {
                                return true;
                        }
                }
        }

        return true;
}

bool
PhysicalSocketServer::WaitEpoll(int cms_wait, bool process_io)
{
        return false;
}
#endif

}// namespace tqcq
