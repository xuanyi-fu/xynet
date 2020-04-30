////
//// Created by xuanyi on 4/22/20.
////

#include <type_traits>
#include <string>
#include <iostream>

#include "xynet/socket/socket.h"
#include "xynet/io_service.h"

#include "xynet/coroutine/task.h"
#include "xynet/coroutine/sync_wait.h"
#include "xynet/coroutine/when_all.h"


using namespace xynet;

using socket_exception_t = xynet::socket_t;

auto acceptor(io_service& service) -> task<>
{
  auto listen_socket = socket_exception_t{};
  auto peer_socket = socket_exception_t{};
  try
  {
    listen_socket.init();
    listen_socket.reuse_address();
    listen_socket.bind(socket_address{2012});
    listen_socket.listen();

    co_await listen_socket.accept(peer_socket);

    auto buf = std::string(1000, 'X');
    auto start_time = std::chrono::system_clock::now();
    for(int i = 0; i < 1; ++i)
    {
      co_await peer_socket.send(buf);
    }
    auto transfer_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time);
    std::cout<<transfer_time.count()<<"\n";
  }
  catch (std::exception& ex)
  {
    std::cout<<ex.what();
  }

  try
  {
    peer_socket.shutdown();
    co_await peer_socket.async_close();
  }
  catch (std::exception& ex)
  {
    std::cout<<ex.what();
  }

  service.request_stop();
  co_return;
}

auto run_service(io_service& service) -> task<>
{
  service.run();
  co_return;
}

int main()
{
  auto service = io_service{};
  auto t = when_all(acceptor(service), run_service(service));
  sync_wait(t);
}