#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <queue>
#include <vector>

namespace net {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using TimerId = std::uint64_t;

struct Timer {
  TimePoint when;
  TimerId id{};
  std::function<void()> cb;
  bool operator>(const Timer& o) const {
    if (when != o.when) return when > o.when;
    return id > o.id;
  }
};

class TimerHeap {
 public:
  TimerId add(TimePoint when, std::function<void()> cb);
  void cancel(TimerId id);
  int pop_due(TimePoint now, std::vector<std::function<void()>>& out);
  int wait_ms(TimePoint now) const;

 private:
  std::priority_queue<Timer, std::vector<Timer>, std::greater<Timer>> q_;
  std::uint64_t next_{1};
  std::vector<char> dead_;  // indexed by id, lazily grown
};

}  // namespace net
