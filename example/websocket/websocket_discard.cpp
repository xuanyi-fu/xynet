#include "common/server.h"
#include "xynet/http/websocket_request_handler.h"

using namespace std;
using namespace xynet;

inline constexpr static const uint16_t ECHO_PORT = 2007;
inline constexpr static const uint16_t MAX_HTTP_REQUEST_SIZE = 4096;

auto echo(socket_t peer_socket) -> task<>
{
  array<byte, MAX_HTTP_REQUEST_SIZE> buf{};
  
  try
  {
    auto parser = websocket_request_handler{};
    auto recv_bytes = std::size_t{};
    while(true)
    {
      recv_bytes += co_await peer_socket.recv_some(buf);
      if(auto ret = parser.parse(std::span{buf.data(), recv_bytes});
         ret == -1)
      {
        continue;
      }
      else
      {
        auto response = parser.generate_response();
        co_await peer_socket.send(response);
        break;
      }
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


