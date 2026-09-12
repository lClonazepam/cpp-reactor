#pragma once

#include "net/reactor.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

namespace net {

class TcpServer {
 public:
  using LineHandler = std::function<void(int fd, const std::string& line)>;

  TcpServer(Reactor& r, std::uint16_t port);
  ~TcpServer();

  void on_line(LineHandler h) { on_line_ = std::move(h); }
  void send(int fd, const std::string& s);
  void broadcast(const std::string& s, int except_fd = -1);
  void close_conn(int fd);

 private:
  void on_listen(int fd, Event ev);
  void on_conn(int fd, Event ev);

  Reactor& r_;
  int listen_{-1};
  LineHandler on_line_;
  std::unordered_map<int, std::string> buf_;
};

}  // namespace net
