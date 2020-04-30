//
// Created by Xuanyi Fu on 4/15/20.
//

/* socket use exception */

#include "doctest/doctest.h"
#include "xynet/socket/impl/socket_init.h"
#include "xynet/socket/socket_operation.h"
#include <system_error>

struct exception_policy
{

};

struct error_code_policy
{
  using policy_use_error_code = void;
};

struct exception_policy_with_address : public exception_policy
{
  using policy_socket_use_address = void;
};

struct error_code_policy_with_address : public error_code_policy
{
  using policy_socket_use_address = void;
};

template<typename T>
struct policy_with
{
  template <template <typename ...> typename module>
  struct map
  {
    template<typename... Ts>
    using type = module<T, Ts ...>;
  };
};

using socket_exception_t = xynet::file_descriptor<
  xynet::detail::map<xynet::detail::module_list
  <
  xynet::tcp::socket,
  xynet::tcp::operation_shutdown
>, policy_with<exception_policy>::template map>::type
>;

using socket_exception_address_t = xynet::file_descriptor<
  xynet::detail::map<xynet::detail::module_list
    <
      xynet::tcp::socket,
      xynet::tcp::address,
      xynet::tcp::operation_bind,
      xynet::tcp::operation_listen,
      xynet::tcp::operation_shutdown
    >, policy_with<exception_policy_with_address>::template map>::type
>;

using socket_errc_t = xynet::file_descriptor<
  xynet::detail::map<xynet::detail::module_list
    <
      xynet::tcp::socket,
      xynet::tcp::operation_shutdown
    >, policy_with<error_code_policy>::template map>::type
>;

TEST_CASE("socket with exception tests")
{

  auto socket = socket_exception_t {};
  auto address_socket = socket_exception_address_t {};

  SUBCASE("init works")
  {
    static_assert(!noexcept(socket.init()), "exception socket init should not be noexcept");
    socket.init();
    CHECK(socket.valid());
  }

  SUBCASE("shutdown without init throws not connected")
  {
    socket.init();
    CHECK_THROWS(socket.shutdown());
  }

  SUBCASE("bind a socket with address module")
  {
    address_socket.init();
    address_socket.bind(xynet::socket_address{25565});
    CHECK(std::is_eq(address_socket.get_local_address() <=> xynet::socket_address{25565}));
  }
}

TEST_CASE("socket with error code tests")
{

  auto socket = socket_errc_t {};

  SUBCASE("init works")
  {
    static_assert(noexcept(socket.init()), "error code socket init should be noexcept");
    auto errc = socket.init();
    if(errc)
    {
      FAIL("socket init failed");
    }
    CHECK(socket.valid());
  }

  SUBCASE("shutdown without init return errc not connceted")
  {
    socket.init();
    auto ret = socket.shutdown();
    auto expected = std::make_error_condition(std::errc::not_connected);
    CHECK((ret == expected));
  }
}

