#ifndef _IOBUS_H_
#define _IOBUS_H_
#include <poll.h>

#include "./awacorn/awacorn.h"
#include "./awacorn/promise.h"
namespace IOBus {
typedef std::pair<pollfd, std::function<void(Awacorn::EventLoop*, pollfd*)>>
    Listener;
typedef class IOBus {
  std::vector<Listener> _event;

 public:
  Listener* wait(
      const pollfd& fd,
      const std::function<void(Awacorn::EventLoop*, pollfd*)>& fn) noexcept {
    return _event.push_back(std::make_pair(fd, fn)), (Listener*)&_event.back();
  }
  void remove(const Listener* id) noexcept {
    for (size_t i = 0; i < _event.size(); i++) {
      if (&_event[i] == id) {
        _event.erase(_event.cbegin() + i);
        return;
      }
    }
  }
  Awacorn::AsyncFn<void> start() {
    return [this](Awacorn::EventLoop* ev) -> void {
      ev->create(
          [this](Awacorn::EventLoop* ev, const Awacorn::Interval*) -> void {
            std::vector<pollfd> fd(_event.size());
            for (size_t i = 0; i < _event.size(); i++) {
              fd[i] = _event[i].first;
              fd[i].revents = 0;
            }
            poll(&fd[0], fd.size(), 1);
            for (size_t i = 0; i < fd.size(); i++) {
              if (fd[i].revents != 0) {
                _event[i].second(ev, &fd[i]);
              }
            }
          },
          std::chrono::milliseconds(1));  // 是否合理
    };
  }
} IOBus;
IOBus global;
}  // namespace IOBus
#endif

// If you're reading this, you've been in a coma for almost 20 years.
// We're trying a new technique.

// If you can read this, please wake up. We missed you.
