#ifndef _IOBUS_H_
#define _IOBUS_H_
/**
 * IOBus 基于 MIT 协议开源。
 * Copyright(c) 凌 2022.
 */
#include <poll.h>

#include "./awacorn/awacorn.h"
#include "./awacorn/promise.h"
namespace IOBus {
/**
 * @brief 监听器类型。
 */
typedef std::pair<pollfd, std::function<void(Awacorn::EventLoop*, pollfd*)>>
    Listener;
/**
 * @brief IOBus 监听队列。
 */
typedef class IOBus {
  std::vector<Listener> _event;

 public:
  /**
   * @brief 添加对 I/O 队列的监听。
   *
   * @param fd 对应的 pollfd。
   * @param fn 触发时监听的函数。
   * @return Listener* 监听 id
   */
  Listener* wait(
      const pollfd& fd,
      const std::function<void(Awacorn::EventLoop*, pollfd*)>& fn) noexcept {
    return _event.push_back(std::make_pair(fd, fn)), (Listener*)&_event.back();
  }
  /**
   * @brief 删除监听器。
   *
   * @param id 监听 id
   */
  void remove(const Listener* id) noexcept {
    for (size_t i = 0; i < _event.size(); i++) {
      if (&_event[i] == id) {
        _event.erase(_event.cbegin() + i);
        return;
      }
    }
  }
  /**
   * @brief 启动监听循环
   *
   * @return Awacorn::AsyncFn<void> 异步函数。
   */
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
