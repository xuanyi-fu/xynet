#include "doctest/doctest.h"

#include "xynet/file_descriptor.h"

#include "xynet/socket/impl/address.h"

#include "xynet/socket/impl/bind.h"
#include "xynet/socket/impl/listen.h"
#include "xynet/socket/impl/shutdown.h"
#include "xynet/socket/impl/socket_init.h"
#include "xynet/socket/impl/setsockopt.h"

#include "xynet/socket/impl/accept.h"
#include "xynet/socket/impl/connect.h"
#include "xynet/socket/impl/recv_all.h"
#include "xynet/socket/impl/close.h"
#include "xynet/socket/impl/send_all.h"

#include "xynet/io_service.h"

#include "xynet/coroutine/task.h"
#include "xynet/coroutine/when_all.h"
#include "xynet/coroutine/sync_wait.h"
#include "xynet/coroutine/async_scope.h"

#include <stop_token>
#include <random>

using namespace xynet;
using namespace std;

using socket_t = file_descriptor
<
  detail::module_list
  <
    address,
    socket_init,
    operation_shutdown,
    operation_bind,
    operation_listen,
    operation_set_options,
    operation_accept,
    operation_connect,
    operation_close,
    operation_recv,
    operation_send
  >
>;


auto close_socket = [](socket_t& socket) -> task<>
{
  try
  {
    socket.shutdown();
    co_await socket.close();
  }catch(...){}
};

auto port_gen = []
{
  auto rd  = random_device{};
  auto gen = mt19937{rd()};
  auto dis = uniform_int_distribution<>{50000, 60000};
  return dis(gen);
};

auto connector(auto func, uint16_t port) -> task<>
{
  auto s = socket_t{};
  try
  {
    s.init();
    s.reuse_address();
    s.bind(socket_address{0});
    co_await s.connect(socket_address{port});
    co_await func(std::move(s));
  }catch(...){}
}

auto acceptor(auto client,
              xynet::io_service& service,
              uint16_t port) 
-> xynet::task<>
{
  auto scope = xynet::async_scope{};
  auto listen_socket = socket_t{};
  auto ex = std::exception_ptr{};

  try
  {
    listen_socket.init();
    listen_socket.reuse_address();
    listen_socket.bind(xynet::socket_address{port});
    listen_socket.listen();
    auto peer_socket = socket_t{};
    co_await listen_socket.accept(peer_socket);
    scope.spawn(client(std::move(peer_socket)));
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

auto service_start(io_service& service, stop_token token) -> task<>
{
  service.run(token);
  co_return;
}


TEST_CASE("send / recv" * doctest::timeout(10.0))
{
  auto service = io_service{};
  
  SUBCASE("recv/send 0 byte")
  {
    auto PORT = port_gen();
    auto source = stop_source{};
    auto client = [](socket_t s) -> task<>
    {
      int i = 0;
      REQUIRE_NOTHROW(co_await s.send(std::span{&i, 0}));
      s.shutdown();
    };

    auto server = [](socket_t s) -> task<>
    {
      int i = 0;
      REQUIRE_THROWS_WITH(co_await s.recv(std::span{&i, 0}), "connection read eof.");
    };

    auto test_recv_send_0 = [&]() -> task<>
    {
      co_await when_all(
        connector(client, PORT),
        acceptor(server, service, PORT)
      );

      source.request_stop();
    };

    sync_wait(when_all(
      test_recv_send_0(),
      service_start(service, source.get_token())
    ));
  }

  SUBCASE("send/recv some bytes")
  {
    auto PORT = port_gen();
    auto source = stop_source{};
    array<char, 5> msg{'x', 'y', 'n', 'e', 't'};


    auto client = [&](socket_t s) -> task<>
    {
      REQUIRE_NOTHROW(co_await s.send(msg));
      co_await close_socket(s);
    };

    auto server = [&](socket_t s) -> task<>
    {
      array<char, 5> buf{};
      REQUIRE_NOTHROW(co_await s.recv(buf));
      CHECK(string_view{msg.data(), msg.size()} == string_view{buf.data(), buf.size()});
      co_await close_socket(s);
    };

    auto test_recv_send_0 = [&]() -> task<>
    {
      co_await when_all(
        connector(client, PORT),
        acceptor(server, service, PORT)
      );

      source.request_stop();
    };

    sync_wait(when_all(
      test_recv_send_0(),
      service_start(service, source.get_token())
    ));
  }

  SUBCASE("send/recv 64K bytes")
  {
    auto PORT = port_gen();
    auto source = stop_source{};
    array<char, 65536> msg{'x'};


    auto client = [&](socket_t s) -> task<>
    {
      REQUIRE_NOTHROW(co_await s.send(msg));
      co_await close_socket(s);
    };

    auto server = [&](socket_t s) -> task<>
    {
      array<char, 65536> buf{};
      REQUIRE_NOTHROW(co_await s.recv(buf));
      CHECK(string_view{msg.data(), msg.size()} == string_view{buf.data(), buf.size()});
      co_await close_socket(s);
    };

    auto test_recv_send_0 = [&]() -> task<>
    {
      co_await when_all(
        connector(client, PORT),
        acceptor(server, service, PORT)
      );

      source.request_stop();
    };

    sync_wait(when_all(
      test_recv_send_0(),
      service_start(service, source.get_token())
    ));
  }

  SUBCASE("send/recv 64K bytes / send part")
  {
    auto PORT = port_gen();
    auto source = stop_source{};
    array<char, 65536> msg{'x'};


    auto client = [&](socket_t s) -> task<>
    {
      for(auto i = msg.begin(); i != msg.end(); i += 1024)
      {
        REQUIRE_NOTHROW(co_await s.send(std::span{i, i + 1024}));
      }
      
      co_await close_socket(s);
    };

    auto server = [&](socket_t s) -> task<>
    {
      array<char, 65536> buf{};
      REQUIRE_NOTHROW(co_await s.recv(buf));
      CHECK(string_view{msg.data(), msg.size()} == string_view{buf.data(), buf.size()});
      co_await close_socket(s);
    };

    auto test_recv_send_0 = [&]() -> task<>
    {
      co_await when_all(
        connector(client, PORT),
        acceptor(server, service, PORT)
      );

      source.request_stop();
    };

    sync_wait(when_all(
      test_recv_send_0(),
      service_start(service, source.get_token())
    ));
  }

  SUBCASE("send/recv 64K bytes / send+recv part")
  {
    auto PORT = port_gen();
    auto source = stop_source{};
    array<char, 65536> msg;
    fill(msg.begin(), msg.end(), 'x');


    auto client = [&](socket_t s) -> task<>
    {
      for(auto i = msg.begin(); i != msg.end(); i += 1024)
      {
        REQUIRE_NOTHROW(co_await s.send(std::span{i, i + 1024}));
      }
      
      co_await close_socket(s);
    };

    auto server = [&](socket_t s) -> task<>
    {
      array<char, 65536> buf{};
      for(auto i = buf.begin(); i != buf.end(); ++i)
      {
        auto recv_bytes = 0;
        recv_bytes = co_await s.recv_some(std::span{i, 1});
      }
      
      CHECK(string_view{msg.data(), msg.size()} == string_view{buf.data(), buf.size()});
      co_await close_socket(s);
    };

    auto test_recv_send_0 = [&]() -> task<>
    {
      co_await when_all(
        connector(client, PORT),
        acceptor(server, service, PORT)
      );

      source.request_stop();
    };

    sync_wait(when_all(
      test_recv_send_0(),
      service_start(service, source.get_token())
    ));
  }

  SUBCASE("recv timeout")
  {
    auto PORT = port_gen();
    auto source = stop_source{};
    array<char, 5> msg{'x', 'y', 'n', 'e', 't'};


    auto client = [&](socket_t s) -> task<>
    {
      co_await service.schedule(chrono::milliseconds{100});
      REQUIRE_NOTHROW(co_await s.send(msg));
      co_await close_socket(s);
    };

    auto server = [&](socket_t s) -> task<>
    {
      array<char, 5> buf{};
      REQUIRE_THROWS_WITH(co_await s.recv(chrono::milliseconds{50}, buf), "Operation canceled");
      co_await close_socket(s);
    };

    auto test_recv_send_0 = [&]() -> task<>
    {
      co_await when_all(
        connector(client, PORT),
        acceptor(server, service, PORT)
      );

      source.request_stop();
    };

    sync_wait(when_all(
      test_recv_send_0(),
      service_start(service, source.get_token())
    ));
  }
}

TEST_CASE("accept / connect")
{
  auto service = io_service{};
  auto connect_socket = socket_t{};
  connect_socket.init();
  connect_socket.reuse_address();
  REQUIRE_NOTHROW(connect_socket.bind(socket_address{0}));
  CHECK(connect_socket.get() >= 0);

  auto listen_socket = socket_t{};
  listen_socket.init();
  listen_socket.reuse_address();

  auto PORT = uint16_t{};
  auto error = std::error_code{};

  auto service_spin = [&service](stop_token token)->task<>
  {
    service.run(token);
    co_return;
  };
  
  do
  {
    PORT = port_gen();
    listen_socket.bind(socket_address{PORT}, error);
  }while(error == make_error_condition(errc::address_in_use));
  
  REQUIRE_NOTHROW(listen_socket.listen());
  CHECK(connect_socket.get() >= 0);

  auto peer_socket = socket_t{};
  auto source = stop_source{};

  SUBCASE("exception")
  {
    auto accept_once = [&]() -> task<>
    {
      REQUIRE_NOTHROW(co_await listen_socket.accept(peer_socket));
      co_return;
    };

    auto connect_once = [&]() -> task<>
    {
      REQUIRE_NOTHROW(co_await connect_socket.connect(socket_address{"127.0.0.1", PORT}));
      co_return;
    };

    auto accpet_connect = [&]() -> task<>
    {
      co_await when_all(accept_once(), connect_once());
      source.request_stop();
    };
    
    sync_wait(when_all(
      accpet_connect(),
      service_spin(source.get_token())
    ));
  }

  SUBCASE("error_code")
  {
    auto accept_once = [&]() -> task<>
    {
      auto error = std::error_code{};
      co_await listen_socket.accept(peer_socket, error);
      CHECK(!error);
      co_return;
    };

    auto connect_once = [&]() -> task<>
    {
      auto error = std::error_code{};
      co_await connect_socket.connect(socket_address{"127.0.0.1", PORT}, error);
      CHECK(!error);
      co_return;
    };

    auto accpet_connect = [&]() -> task<>
    {
      co_await when_all(accept_once(), connect_once());
      source.request_stop();
    };

    sync_wait(when_all(
      accpet_connect(),
      service_spin(source.get_token())
    ));
  }

  SUBCASE("timeout")
  {
    SUBCASE("exception")
    {
      auto accept_once_timeout = [&]() -> task<>
      {
        REQUIRE_THROWS_WITH(co_await listen_socket.accept(peer_socket, chrono::milliseconds{100}), 
          "Operation canceled");
        source.request_stop();
        co_return;
      };

      sync_wait(when_all(
      accept_once_timeout(),
      service_spin(source.get_token())
      ));
    }

    SUBCASE("error_code")
    {
      auto accept_once_timeout = [&]() -> task<>
      {
        auto error = std::error_code{};
        co_await listen_socket.accept(peer_socket, chrono::milliseconds{100}, error);
        CHECK((error == make_error_condition(errc::operation_canceled)));
        source.request_stop();
        co_return;
      };

      sync_wait(when_all(
      accept_once_timeout(),
      service_spin(source.get_token())
      ));
    }

  }

  try
  {
    connect_socket.shutdown();
    listen_socket.shutdown();
    peer_socket.shutdown();
  }catch(...){}
  
}

