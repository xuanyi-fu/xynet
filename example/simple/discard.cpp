#include "common/server.h"
#include "xynet/buffer.h"

#include <span>

using namespace std;
using namespace xynet;

inline constexpr static const uint16_t DISCARD_PORT = 2009; 
inline constexpr static const size_t   NULL_BUFFER_SIZE = 4096;

auto get_null_buffer_span()
{
  static array<byte, NULL_BUFFER_SIZE> null_buffer{};
  return as_writable_bytes(span{null_buffer});
}

auto discard(socket_t peer_socket) -> task<>
{
  try
  {
    for(;;)
    {
      auto read_bytes = 
      co_await peer_socket.recv_some(get_null_buffer_span());
    }
  }
  catch(...){}
}

int main()
{
  auto service = io_service{};
  sync_wait(start_server(discard, service, DISCARD_PORT));
}