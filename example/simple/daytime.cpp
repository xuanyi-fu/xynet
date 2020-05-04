#include "common/server.h"
#include "xynet/buffer.h"
#include "xynet/stream_buffer.h"
#include <chrono>
#include <iomanip>
#include <ostream>

#include <iostream>

using namespace std;
using namespace xynet;

inline constexpr static const uint16_t DAYTIME_PORT = 2013;

auto daytime(socket_t peer_socket) -> task<>
{
  auto time_t_now = chrono::system_clock::to_time_t(chrono::system_clock::now());
  auto* tm_now = localtime(&time_t_now);
  
  stream_buffer buf{};
  auto os = ostream(&buf);
  os << put_time(tm_now, "%c\n");
  auto send_span = buf.data();
  
  try
  {
    [[maybe_unused]]
    auto sent_bytes = co_await peer_socket.send(send_span);
    peer_socket.shutdown();
    co_await peer_socket.close();
  }catch(...){}
}

int main()
{
  auto service = io_service{};
  sync_wait(start_server(daytime, service, DAYTIME_PORT));
}


