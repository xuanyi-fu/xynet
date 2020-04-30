//
// Created by xuanyi on 4/2/20.
//

#include "doctest/doctest.h"
#include "xynet/socket/impl/socket_address.h"
#include <arpa/inet.h>
#include <strings.h>

using namespace xynet;

TEST_CASE("socket_address default construct")
{
  CHECK_EQ(socket_address{}.to_str(), "0.0.0.0:0");
}

TEST_CASE("socket_address construct with correct given port")
{
  CHECK_EQ(socket_address{25565}.to_str(), "0.0.0.0:25565");
}

TEST_CASE("socket_address construct with struct ::sockaddr_in")
{
  auto sockaddr = ::sockaddr_in{};
  uint16_t port = 25565;
  std::string ip{"127.0.0.1"};

  ::bzero(&sockaddr, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  ::inet_pton(AF_INET, ip.c_str(), &sockaddr.sin_addr);
  sockaddr.sin_port = ::htons(port);
  auto addr  = socket_address{sockaddr};

  CHECK_EQ(socket_address{addr}.to_str(), "127.0.0.1:25565");
}

TEST_CASE("socket_address constrcut with ip and port")
{
  CHECK_EQ(socket_address{"127.0.0.1", 25565}.to_str(), "127.0.0.1:25565");
}

TEST_CASE("socket_address copy assignment")
{
  auto addr = socket_address{"123.123.123.123", 80};
  auto copy = addr;

  CHECK_EQ(addr.to_str(), copy.to_str());
}

TEST_CASE("socket_address copy construct")
{
  auto addr = socket_address{"123.123.123.123", 80};
  auto copy{addr};

  CHECK_EQ(addr.to_str(), copy.to_str());
}

TEST_CASE("socket_address move construct")
{
  auto addr = socket_address{"123.123.123.123", 80};
  auto move_addr{std::move(addr)};

  CHECK_EQ(addr.to_str(), "0.0.0.0:0");
  CHECK_EQ(move_addr.to_str(), "123.123.123.123:80");
}

TEST_CASE("socket_address move assignment")
{
  auto addr = socket_address{"123.123.123.123", 80};
  auto move_addr = std::move(addr);

  CHECK_EQ(addr.to_str(), "0.0.0.0:0");
  CHECK_EQ(move_addr.to_str(), "123.123.123.123:80");
}

