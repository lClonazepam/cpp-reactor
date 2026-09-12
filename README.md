# cpp-reactor

A Linux event loop in three layers:

1. `TimerHeap` — min-heap of deadlines, lazy cancel via a generation/dead flag
2. `Reactor` — `epoll` ET + timer-aware `epoll_wait` timeout
3. `TcpServer` — non-blocking accept, per-fd line buffer, broadcast

`chatd` is a tiny multi-client line chat on `:9000`:

```
/quit     close this connection
/stop     stop the reactor
```

This is the shape of libevent / muduo / Boost.Asio's reactor: one thread, readiness + timers, application protocol on top of byte streams.

## Build (Linux)

```bash
cmake -S . -B build && cmake --build build -j
./build/chatd
# other terminals:
nc 127.0.0.1 9000
```
