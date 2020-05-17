#include "common/server.h"
#include "common/websocket.h"

using namespace std;
using namespace xynet;

inline constexpr static const uint16_t ECHO_PORT = 2007;

auto disconnect(socket_t peer_socket) -> task<>
{
  try
  {
    peer_socket.shutdown();
    co_await peer_socket.close();
  }catch(...){}
}

auto echo_once(socket_t& peer_socket, 
  std::array<byte, MAX_WEBSOCKET_FRAME_SIZE>& buf) -> task<>
{
  auto data_span = co_await websocket_recv_data(peer_socket, buf);
  auto header = websocket_frame_header{websocket_flags::WS_FINAL_FRAME 
    | websocket_flags::WS_OP_TEXT, data_span.size()};
  [[maybe_unused]]
  auto sent_bytes = co_await peer_socket.send(header.span(), data_span);
  co_return;
}

auto websocket_echo(socket_t peer_socket) -> task<>
{
  try
  {
    co_await websocket_handshake(peer_socket);
    array<byte, MAX_WEBSOCKET_FRAME_SIZE> buf{};
    while(true)
    {
      // this line must be added here to avoid some gcc compiler bug.
      [[maybe_unused]]
      auto i = 0;
      co_await echo_once(peer_socket, buf);
    }
    
  }catch(const std::exception& ex)
  {
    std::cout<<ex.what();
  }

  co_await disconnect(std::move(peer_socket));
  co_return;
}

int main()
{
  auto service = io_service{};
  sync_wait(start_server(websocket_echo, service, ECHO_PORT));
}


