//
// Created by tqcq on 2023/9/12.
//

#include "signaler.h"
#include "u-toolbox/concurrency/lock_guard.h"
#include "u-toolbox/log/u_log.h"
#include <sys/pipe.h>

namespace tqcq {
Signaler::Signaler(PhysicalSocketServer *ss, bool &flag_to_clear)
    : ss_(ss),
      afd_([] {
              std::array<int, 2> afd = {-1, -1};
              if (pipe(afd.data()) < 0) {
                      U_ERROR("Signaler: pipe failed");
                      return afd;
              }
              return afd;
      }()),
      f_signaled_(false),
      flag_to_clear_(flag_to_clear)
{
        ss_->Add(this);
}

Signaler::~Signaler()
{
        ss_->Remove(this);
        close(afd_[0]);
        close(afd_[1]);
}

void
Signaler::Signal()
{
        LockGuard lock(mutex_);
        if (!f_signaled_) {
                const uint8_t b[1] = {0};
                const ssize_t res = write(afd_[1], b, sizeof(b));
                U_ASSERT(1 == res, "Signaler: write failed");
                f_signaled_ = true;
        }
}

uint32_t
Signaler::GetRequestedEvents()
{
        return DE_READ;
}

void
Signaler::OnEvent(uint32_t ff, int err)
{
        LockGuard lock(mutex_);
        if (f_signaled_) {
                uint8_t b[4];
                const ssize_t res = read(afd_[0], b, sizeof(b));
                U_ASSERT(1 == res, "Signaler: read failed");
                f_signaled_ = false;
        }
        flag_to_clear_ = false;
}

int
Signaler::GetDescriptor() const
{
        return afd_[0];
}

bool
Signaler::IsDescriptorClosed() const
{
        return false;
}
}// namespace tqcq
