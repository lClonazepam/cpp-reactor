#pragma once

#include "net/timer.hpp"

#include <cstdint>
#include <functional>
#include <unordered_map>

namespace net {

enum class Event : std::uint32_t { In = 1, Out = 2, Err = 4, Hup = 8 };
inline Event operator|(Event a, Event b) {
  return static_cast<Event>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}
inline bool has(Event set, Event bit) {
  return (static_cast<std::uint32_t>(set) & static_cast<std::uint32_t>(bit)) != 0;
}

class Reactor {
 public:
  Reactor();
  ~Reactor();

  void add(int fd, Event ev, std::function<void(int, Event)> cb);
  void mod(int fd, Event ev);
  void del(int fd);

  TimerId after(int ms, std::function<void()> cb);
  void cancel(TimerId id);

  void run();
  void stop();

 private:
  int ep_{-1};
  bool running_{false};
  TimerHeap timers_;
  std::unordered_map<int, std::function<void(int, Event)>> cbs_;
};

}  // namespace net
