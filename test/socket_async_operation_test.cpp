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
    operation_recv
  >
>;

inline static auto service = io_service{};
auto service_spin = [&service](stop_token token)->task<>
{
  service.run(token);
  co_return;
};

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


TEST_CASE("recv")
{

}

TEST_CASE("accept / connect")
{
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

