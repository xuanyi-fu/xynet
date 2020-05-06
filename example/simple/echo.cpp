#include "common/server.h"
#include <array>

using namespace std;
using namespace xynet;

inline constexpr static const uint16_t ECHO_PORT = 2007;
inline constexpr static const uint16_t ECHO_BUFFER_SIZE = 4096;

auto echo(socket_t peer_socket) -> task<>
{
  array<byte, ECHO_BUFFER_SIZE> buf{};
  try
  {
    for(;;)
    {
      auto recv_bytes = co_await peer_socket.recv_some(buf);
      [[maybe_unused]]
      auto sent_bytes = co_await peer_socket.send(span{buf.begin(), recv_bytes});
    }
  }catch(...){}

  try
  {
    peer_socket.shutdown();
    co_await peer_socket.close();
  }catch(...){}

}

int main()
{
  auto service = io_service{};
  sync_wait(start_server(echo, service, ECHO_PORT));
}


