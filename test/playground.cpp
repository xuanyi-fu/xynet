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

auto echo(socket_exception_t peer_socket) -> task<>
{
  auto buffer = std::vector<char>(1024);
  try
  {
    for(;;)
    {
      auto read_bytes = co_await peer_socket.recv_some(buffer);
      [[maybe_unused]]
      auto send_bytes = co_await peer_socket.send(std::span{buffer.begin(), read_bytes});
    }
  }
  catch(const std::system_error& error)
  {
    std::cout<<error.what()<<"\n";
  }

  peer_socket.shutdown();
  co_await peer_socket.close();
  co_return;
}

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
    co_await echo(std::move(peer_socket));
  }
  catch (std::exception& ex)
  {
    std::cout<<ex.what();
  }
  
  listen_socket.shutdown();
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