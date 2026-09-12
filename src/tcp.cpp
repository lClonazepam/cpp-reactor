#include "net/tcp.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdexcept>

namespace net {
namespace {
int set_nb(int fd) {
  int fl = fcntl(fd, F_GETFL, 0);
  return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}
}  // namespace

TcpServer::TcpServer(Reactor& r, std::uint16_t port) : r_(r) {
  listen_ = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listen_ < 0) throw std::runtime_error("socket");
  int yes = 1;
  setsockopt(listen_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_ANY);
  a.sin_port = htons(port);
  if (bind(listen_, reinterpret_cast<sockaddr*>(&a), sizeof(a)) < 0)
    throw std::runtime_error("bind");
  if (listen(listen_, 128) < 0) throw std::runtime_error("listen");
  set_nb(listen_);
  r_.add(listen_, Event::In, [this](int fd, Event ev) { on_listen(fd, ev); });
}

TcpServer::~TcpServer() {
  if (listen_ >= 0) {
    r_.del(listen_);
    close(listen_);
  }
  for (auto& [fd, _] : buf_) {
    r_.del(fd);
    close(fd);
  }
}

void TcpServer::on_listen(int, Event) {
  for (;;) {
    int cfd = ::accept(listen_, nullptr, nullptr);
    if (cfd < 0) break;
    set_nb(cfd);
    buf_[cfd] = {};
    r_.add(cfd, Event::In, [this](int fd, Event ev) { on_conn(fd, ev); });
    send(cfd, "welcome. type a line.\n");
  }
}

void TcpServer::on_conn(int fd, Event ev) {
  if (has(ev, Event::Err) || has(ev, Event::Hup)) {
    close_conn(fd);
    return;
  }
  if (!has(ev, Event::In)) return;
  char tmp[4096];
  for (;;) {
    ssize_t n = ::recv(fd, tmp, sizeof(tmp), 0);
    if (n > 0) {
      buf_[fd].append(tmp, static_cast<std::size_t>(n));
      continue;
    }
    if (n == 0) {
      close_conn(fd);
      return;
    }
    break;
  }
  auto& b = buf_[fd];
  std::size_t pos;
  while ((pos = b.find('\n')) != std::string::npos) {
    auto line = b.substr(0, pos);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    b.erase(0, pos + 1);
    if (on_line_) on_line_(fd, line);
  }
}

void TcpServer::send(int fd, const std::string& s) {
  ::send(fd, s.data(), s.size(), 0);
}

void TcpServer::broadcast(const std::string& s, int except_fd) {
  for (auto& [fd, _] : buf_) {
    if (fd != except_fd) send(fd, s);
  }
}

void TcpServer::close_conn(int fd) {
  r_.del(fd);
  close(fd);
  buf_.erase(fd);
}

}  // namespace net
