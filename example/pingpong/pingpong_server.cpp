#include "common/server.h"
#include <vector>

using namespace std;
using namespace xynet;

inline static size_t PINGPONG_BUFFER_SIZE = size_t{};

auto pingpong_server(socket_t peer_socket) -> task<>
{
  auto buf = vector<byte>(PINGPONG_BUFFER_SIZE);
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

int main(int argc, char** argv)
{
  if(argc != 3)
  {
    puts("usage: pingpong_server [port] [message length bytes]");
    return 0;
  }

  auto port = uint16_t{};
  auto len  = size_t{};

  try
  {
    port = static_cast<uint16_t>(stoi(string(argv[1])));
    len = stoi(string(argv[2]));
  }catch(const exception& ex)
  {
    puts(ex.what());
    return 0;
  }

  PINGPONG_BUFFER_SIZE = len;
  
  auto service = io_service{};
  sync_wait(start_server(pingpong_server, service, port));
}


