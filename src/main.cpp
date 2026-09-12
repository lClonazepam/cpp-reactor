#include "net/reactor.hpp"
#include "net/tcp.hpp"

#include <iostream>
#include <sstream>

int main() {
  net::Reactor loop;
  net::TcpServer srv(loop, 9000);

  loop.after(0, [] { std::cout << "chatd :9000  (idle ping every 15s)\n"; });
  loop.after(15000, [&] {
    srv.broadcast("[server] ping\n");
    // re-arm by scheduling another one from a recursive style is awkward;
    // one-shot ping is enough to show the timer path.
  });

  srv.on_line([&](int fd, const std::string& line) {
    if (line == "/quit") {
      srv.send(fd, "bye\n");
      srv.close_conn(fd);
      return;
    }
    if (line == "/stop") {
      srv.broadcast("[server] shutting down\n");
      loop.stop();
      return;
    }
    std::ostringstream oss;
    oss << "fd" << fd << ": " << line << "\n";
    srv.broadcast(oss.str(), /*except*/ -1);
  });

  loop.run();
}
