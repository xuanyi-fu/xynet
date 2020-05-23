#include <concepts>

#include "xynet/io_service.h"
#include "xynet/socket/socket.h"

#include "xynet/coroutine/task.h"
#include "xynet/coroutine/when_all.h"
#include "xynet/coroutine/sync_wait.h"
#include "xynet/coroutine/async_scope.h"

auto acceptor(auto client,
              xynet::io_service& service,
              uint16_t port) 
-> xynet::task<>
{
  auto scope = xynet::async_scope{};
  auto listen_socket = xynet::socket_t{};
  auto ex = std::exception_ptr{};

  try
  {
    listen_socket.init();
    listen_socket.reuse_address();
    listen_socket.bind(xynet::socket_address{port});
    listen_socket.listen();

    for(;;)
    {
      auto peer_socket = xynet::socket_t{};
      co_await listen_socket.accept(peer_socket);
      scope.spawn(client(std::move(peer_socket)));
    }
  }
  catch(...)
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

auto start_service(xynet::io_service& service) 
-> xynet::task<>
{
  service.run();
  co_return;
}

auto start_server(auto client, 
xynet::io_service& service, uint16_t port)
-> xynet::task<>
{
  co_await xynet::when_all
  (
    acceptor(client, service, port),
    start_service(service)
  );

  co_return;
}