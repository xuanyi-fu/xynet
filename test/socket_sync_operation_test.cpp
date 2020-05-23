#include "doctest/doctest.h"

#include "xynet/file_descriptor.h"

#include "xynet/socket/impl/address.h"

#include "xynet/socket/impl/bind.h"
#include "xynet/socket/impl/listen.h"
#include "xynet/socket/impl/shutdown.h"
#include "xynet/socket/impl/socket_init.h"
#include "xynet/socket/impl/setsockopt.h"

using namespace xynet;
using namespace std;

using socket_sync_t = file_descriptor
<
  detail::module_list
  <
    address,
    socket_init,
    operation_shutdown,
    operation_bind,
    operation_listen,
    operation_set_options
  >
>;

inline constexpr static const uint16_t SYNC_TEST_PORT = 25564;
inline static const string             SYNC_TEST_ADDRESS_STR = "0.0.0.0:25564"s;

TEST_CASE("socket sync operations test")
{
  auto s = socket_sync_t{};
  s.init();
  CHECK(s.get() >= 0);

  SUBCASE("init twice will close old fd")
  {
    auto fd = s.get();
    s.init();
    CHECK_EQ(s.get(), fd);
  }

  SUBCASE("bind")
  {
    SUBCASE("check socket_address set after bind")
    {
      s.bind(socket_address{SYNC_TEST_PORT});
      CHECK_EQ(s.get_local_address().to_str(), SYNC_TEST_ADDRESS_STR);
    }
  
    SUBCASE("bind to random port")
    {
      s.bind(socket_address{0});
      CHECK(s.get_local_address().port() != 0);
    }

    SUBCASE("bind to a port twice throws exception")
    {
      s.bind(socket_address{SYNC_TEST_PORT});
      REQUIRE_THROWS_WITH(s.bind(socket_address{SYNC_TEST_PORT}), "Invalid argument");
    }
  }

  SUBCASE("listen")
  {
    s.bind(socket_address{SYNC_TEST_PORT});
    s.listen();
    auto ret = int{};
    auto len = socklen_t{sizeof(ret)};
    ::getsockopt(s.get(), SOL_SOCKET, SO_ACCEPTCONN, &ret, &len);
    CHECK(ret != 0);
  }

  SUBCASE("reuse_address")
  {
    s.reuse_address();
    auto ret = int{};
    auto len = socklen_t{sizeof(ret)};
    ::getsockopt(s.get(), SOL_SOCKET, SO_REUSEADDR, &ret, &len);
    CHECK(ret != 0);
  }

  SUBCASE("shutdown")
  {
    REQUIRE_THROWS(s.shutdown());
  }
}
