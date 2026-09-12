#include "net/reactor.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <stdexcept>
#include <vector>

namespace net {

namespace {
std::uint32_t to_epoll(Event ev) {
  std::uint32_t e = 0;
  if (has(ev, Event::In)) e |= EPOLLIN;
  if (has(ev, Event::Out)) e |= EPOLLOUT;
  e |= EPOLLET | EPOLLRDHUP;
  return e;
}
Event from_epoll(std::uint32_t e) {
  std::uint32_t o = 0;
  if (e & EPOLLIN) o |= static_cast<std::uint32_t>(Event::In);
  if (e & EPOLLOUT) o |= static_cast<std::uint32_t>(Event::Out);
  if (e & EPOLLERR) o |= static_cast<std::uint32_t>(Event::Err);
  if (e & (EPOLLHUP | EPOLLRDHUP)) o |= static_cast<std::uint32_t>(Event::Hup);
  return static_cast<Event>(o);
}
}  // namespace

Reactor::Reactor() {
  ep_ = epoll_create1(0);
  if (ep_ < 0) throw std::runtime_error("epoll_create1");
}
Reactor::~Reactor() {
  if (ep_ >= 0) close(ep_);
}

void Reactor::add(int fd, Event ev, std::function<void(int, Event)> cb) {
  epoll_event e{};
  e.events = to_epoll(ev);
  e.data.fd = fd;
  if (epoll_ctl(ep_, EPOLL_CTL_ADD, fd, &e) < 0) throw std::runtime_error("epoll add");
  cbs_[fd] = std::move(cb);
}
void Reactor::mod(int fd, Event ev) {
  epoll_event e{};
  e.events = to_epoll(ev);
  e.data.fd = fd;
  epoll_ctl(ep_, EPOLL_CTL_MOD, fd, &e);
}
void Reactor::del(int fd) {
  epoll_ctl(ep_, EPOLL_CTL_DEL, fd, nullptr);
  cbs_.erase(fd);
}

TimerId Reactor::after(int ms, std::function<void()> cb) {
  return timers_.add(Clock::now() + std::chrono::milliseconds(ms), std::move(cb));
}
void Reactor::cancel(TimerId id) { timers_.cancel(id); }

void Reactor::stop() { running_ = false; }

void Reactor::run() {
  running_ = true;
  epoll_event evs[64];
  while (running_) {
    std::vector<std::function<void()>> due;
    timers_.pop_due(Clock::now(), due);
    for (auto& f : due) f();
    int to = timers_.wait_ms(Clock::now());
    int n = epoll_wait(ep_, evs, 64, to);
    for (int i = 0; i < n; ++i) {
      int fd = evs[i].data.fd;
      auto it = cbs_.find(fd);
      if (it == cbs_.end()) continue;
      auto cb = it->second;
      cb(fd, from_epoll(evs[i].events));
    }
  }
}

}  // namespace net
