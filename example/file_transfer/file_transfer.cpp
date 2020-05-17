#include "common/server.h"
#include "xynet/file/file.h"
#include "xynet/buffer.h"
#include "xynet/stream_buffer.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <time.h>

using namespace std;
using namespace xynet;

inline constexpr static const uint16_t FILE_TRANSFER_PORT = 25566;

auto file_transfer(socket_t peer_socket, std::string_view path) -> task<>
{
  auto file = file_t{};
  try
  {
    co_await file.openat(path, O_RDONLY, 0u);
    co_await file.update_statx();
    auto buffer = vector<byte>(file.file_size());
    co_await file.read_some(buffer);
    [[maybe_unused]]
    auto sent_bytes = co_await peer_socket.send(buffer);
  }catch(const exception& ex)
  {
    puts(ex.what());
  }
  try
  {
    peer_socket.shutdown();
    co_await peer_socket.close();
  }catch(...){}
}

int main(int argc, char** argv)
{
  if(argc != 2)
  {
    puts("usage: file_transfer [path-to-file]");
    return 0;
  }
  auto service = io_service{};
  sync_wait(start_server([argv](socket_t peer_socket) -> task<>
  {
    co_await file_transfer(std::move(peer_socket), string_view{argv[1]});
    co_return;
  }, service, FILE_TRANSFER_PORT));
}


