#include "common/server.h"
#include "xynet/buffer.h"
#include "xynet/stream_buffer.h"
#include <ctime>


using namespace std;
using namespace xynet;

inline constexpr static const uint16_t TIME_PORT = 2037;

auto time_server(socket_t peer_socket) -> task<>
{
  auto now = time(nullptr);
  auto buf = span{&now, sizeof(now)};
  try
  {
    [[maybe_unused]]
    auto sent_bytes = co_await peer_socket.send(buf);
    peer_socket.shutdown();
    co_await peer_socket.close();
  }catch(...){}
}

auto time_client(const socket_address& address, io_service& service) -> task<>
{
  auto s = socket_t{};
  try
  {
    s.init();
    co_await s.connect_timeout(address, std::chrono::seconds{1});
    time_t t{};
    auto buf = span{&t, sizeof(t)};
    auto recv_bytes = co_await s.recv(buf);

    auto tm_now = ::tm{};
    localtime_r(&t, &tm_now);

    array<char, 128> time_str;
    strftime(time_str.data(), time_str.size(), "%c\n", &tm_now);
    printf("%s", time_str.data());
  }catch(const std::exception& ex)
  {
    printf("Failed: %s\n", ex.what());
  }

  try
  {
    s.shutdown();
    co_await s.close();
  }catch(...){}

  service.request_stop();

}

int main(int argc, char** argv)
{
  bool b = string{argv[1]} == "-l"s;
  // server
  if((argc == 2) && string{argv[1]} == "-l"s)
  {
    auto service = io_service{};
    sync_wait(start_server(time_server, service, TIME_PORT));
    return 0;
  }

  //client
  if(argc == 2)
  {
    auto service = io_service{};
    auto ip = std::string{argv[1]};
    auto address = socket_address{ip, TIME_PORT};
    sync_wait(when_all(time_client(address, service), [&]()->task<>{service.run(); co_return;}()));
    return 0;
  }

  printf("Usage: simple_time [-l] [destination]");
}


