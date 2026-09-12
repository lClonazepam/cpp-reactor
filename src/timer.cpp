#include "net/timer.hpp"

namespace net {

TimerId TimerHeap::add(TimePoint when, std::function<void()> cb) {
  auto id = next_++;
  if (dead_.size() <= id) dead_.resize(id + 1, 0);
  dead_[id] = 0;
  q_.push(Timer{when, id, std::move(cb)});
  return id;
}

void TimerHeap::cancel(TimerId id) {
  if (id < dead_.size()) dead_[id] = 1;
}

int TimerHeap::pop_due(TimePoint now, std::vector<std::function<void()>>& out) {
  int n = 0;
  while (!q_.empty() && q_.top().when <= now) {
    auto t = q_.top();
    q_.pop();
    if (t.id < dead_.size() && dead_[t.id]) continue;
    out.push_back(std::move(t.cb));
    ++n;
  }
  return n;
}

int TimerHeap::wait_ms(TimePoint now) const {
  if (q_.empty()) return 200;
  auto d = std::chrono::duration_cast<std::chrono::milliseconds>(q_.top().when - now).count();
  if (d < 0) return 0;
  if (d > 1000) return 1000;
  return static_cast<int>(d);
}

}  // namespace net
