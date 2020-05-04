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
#include "xynet/coroutine/async_scope.h"


using namespace xynet;

using socket_exception_t = xynet::socket_t;

auto echo(socket_exception_t peer_socket) -> task<>
{
  auto buffer = std::array<std::byte, 4096>{};
  try
  {
    for(;;)
    {
      auto read_bytes = co_await peer_socket.recv(buffer);
      auto send_bytes = co_await peer_socket.send(std::span{buffer.begin(), read_bytes});
    }
  }
  catch(...){}

  try
  {
    peer_socket.shutdown();
    co_await peer_socket.close();
  }
  catch(...){}

  co_return;
}

auto acceptor(io_service& service) -> task<>
{
  auto scope = async_scope{};
  auto ex = std::exception_ptr{};
  try
  {
    auto listen_socket = socket_exception_t{};
    listen_socket.init();
    listen_socket.reuse_address();
    listen_socket.bind(socket_address{2012});
    listen_socket.listen();
    
    for(;;)
    {
      auto peer_socket = socket_exception_t{};
      co_await listen_socket.accept(peer_socket);
      scope.spawn(echo(std::move(peer_socket)));
    }
  }
  catch (...)
  {
    ex = std::current_exception();
  }

  co_await scope.join();
  service.request_stop();

  if(ex)
  {
    std::rethrow_exception(ex);
  }

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