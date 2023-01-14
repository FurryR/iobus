#ifndef _IOBUS_H_
#define _IOBUS_H_
/**
 * IOBus 基于 MIT 协议开源。
 * Copyright(c) 凌 2022.
 */
#include <poll.h>

#include <list>
#include <set>
#include <vector>

#include "./awacorn/awacorn.h"
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
  std::list<Listener> _event;
  std::set<const Listener*> _off_schedule;
  Awacorn::EventLoop* _ev;
  const Awacorn::Interval* _intv;
  /**
   * @brief 启动监听循环
   *
   * @return Awacorn::AsyncFn<void> 异步函数。
   */
  void _start() {
    _intv = _ev->create(
        [this](Awacorn::EventLoop* ev, const Awacorn::Interval*) -> void {
          if (!_off_schedule.empty()) {
            for (std::set<const Listener*>::const_iterator it =
                     _off_schedule.cbegin();
                 it != _off_schedule.cend(); it++) {
              for (std::list<Listener>::const_iterator it2 = _event.cbegin();
                   it2 != _event.cend(); it2++) {
                if (&(*it2) == *it) {
                  _event.erase(it2);
                  if (_event.empty()) {
                    _ev->clear(_intv);
                    _intv = nullptr;
                    break;
                  }
                }
              }
            }
            _off_schedule.clear();
          }
          std::vector<pollfd> fd(_event.size());
          size_t i = 0;
          for (std::list<Listener>::const_iterator it = _event.cbegin();
               it != _event.cend(); it++, i++) {
            fd[i] = it->first;
            fd[i].revents = 0;
          }
          i = 0;
          if (poll(&fd[0], fd.size(), 1) != 0) {
            for (std::list<Listener>::const_iterator it = _event.cbegin();
                 it != _event.cend(); it++, i++) {
              if (fd[i].revents != 0) {
                it->second(ev, &fd[i]);
              }
            }
          }
        },
        std::chrono::milliseconds(1));  // 是否合理
  }
  IOBus(Awacorn::EventLoop* _ev) : _ev(_ev), _intv(nullptr) {}

 public:
  /**
   * @brief 添加对 I/O 队列的监听。
   *
   * @param fd 对应的 pollfd。
   * @param fn 触发时监听的函数。
   * @return const Listener* 监听 id
   */
  const Listener* on(
      const pollfd& fd,
      const std::function<void(Awacorn::EventLoop*, pollfd*)>& fn) noexcept {
    _event.push_back(std::make_pair(fd, fn));
    if (!_intv) {
      _start();
    }
    return (const Listener*)&_event.back();
  }
  /**
   * @brief 删除监听器。
   *
   * @param id 监听 id
   */
  void off(const Listener* id) noexcept { _off_schedule.insert(id); }
  friend Awacorn::AsyncFn<IOBus> create();
} IOBus;
Awacorn::AsyncFn<IOBus> create() {
  return [](Awacorn::EventLoop* ev) -> IOBus { return IOBus(ev); };
}
}  // namespace IOBus
#endif
