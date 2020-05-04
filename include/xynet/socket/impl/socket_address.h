//
// Created by xuanyi on 3/26/20.
//

#ifndef XYNET_SOCKET_SOCKET_ADDRESS_H
#define XYNET_SOCKET_SOCKET_ADDRESS_H

#include <cstdint>
#include <netinet/in.h>
#include <string>
#include <compare>
#include <arpa/inet.h>
#include <cassert>
#include <strings.h>
#include <cstring>
#include <g3log/g3log.hpp>

namespace xynet
{
class socket_address
{
public:
  socket_address() noexcept;

  socket_address (const socket_address&) noexcept = default;
  socket_address& operator=(const socket_address&) noexcept = default;

  socket_address (socket_address&& other) noexcept;
  socket_address&operator=(socket_address&& other) noexcept;

  explicit socket_address(uint16_t port) noexcept;
  explicit socket_address(const ::sockaddr_in&) noexcept ;
  socket_address(const std::string& ip, uint16_t port) noexcept;

  [[nodiscard]]
  std::string to_str() const noexcept;

  [[nodiscard]]
  const ::sockaddr_in* as_sockaddr_in() const noexcept
  {
    return reinterpret_cast<const ::sockaddr_in*>(this);
  } 

  friend std::ostream& operator<<(std::ostream&,
                                  const socket_address& address);

  std::strong_ordering operator<=>(const socket_address& rhs) const
  {
    if (auto cmp = this <=> &rhs; std::is_eq(cmp)) return cmp;
    if (auto cmp = m_sockaddr.sin_addr.s_addr <=> rhs.m_sockaddr.sin_addr.s_addr; std::is_eq(cmp)) return cmp;
    return m_sockaddr.sin_port <=> rhs.m_sockaddr.sin_port;
  }

private:
  ::sockaddr_in m_sockaddr;

};
}

namespace xynet::detail
{
template <typename UnsignedInt>
static UnsignedInt host_to_network(UnsignedInt host);

template <typename UnsignedInt>
static UnsignedInt network_to_host(UnsignedInt net);

template <typename T> struct dependentFalse : std::false_type {};

template <typename UnsignedInt>
UnsignedInt host_to_network(UnsignedInt host [[maybe_unused]]) {
  static_assert(dependentFalse<UnsignedInt>::value,
                "SocketAddress::host_to_network instantiation failed: "
                "host must be uint32_t or uint16_t");
}

template <typename UnsignedInt>
UnsignedInt network_to_host(UnsignedInt net [[maybe_unused]]) {
  static_assert(dependentFalse<UnsignedInt>::value,
                "SocketAddress::network_to_host instantiation failed: "
                "host must be uint32_t or uint16_t");
}

template<>
uint64_t host_to_network<uint64_t>(uint64_t host)
{
  return ::htobe64(host);
}

template <> uint32_t host_to_network<uint32_t>(uint32_t host) {
  return ::htonl(host);
}

template <> uint16_t host_to_network<uint16_t>(uint16_t host) {
  return ::htons(host);
}

template<>
uint64_t network_to_host<uint64_t>(uint64_t net)
{
  return ::be64toh(net);
}

template <> uint32_t network_to_host<uint32_t>(uint32_t net) {
  return ::ntohl(net);
}

template <> uint16_t network_to_host<uint16_t>(uint16_t net) {
  return ::ntohs(net);
}

::in_addr ip_str_to_in_addr(const std::string& str)
{
  auto ret = ::in_addr{};
  if(::inet_pton(AF_INET, str.c_str(), &ret) == 1)
  {
    return ret;
  }
  else
  {
    // LOG(FATAL)<<"ip_str_to_in_addr() failed";
    return{};
  }
}

void sockaddr_in_to_c_str(char* buf, size_t size, const ::sockaddr_in& addr)
{
  char host[INET_ADDRSTRLEN] = "INVALID";
  ::inet_ntop(AF_INET, &addr.sin_addr, host, sizeof host);
  uint16_t port = network_to_host(addr.sin_port);
  snprintf(buf, size, "%s:%u", host, port);
}

}

namespace xynet
{

using namespace detail;

socket_address::socket_address() noexcept
  :m_sockaddr
     {
       .sin_family = AF_INET
     }
{}

socket_address::socket_address(uint16_t port) noexcept
  :m_sockaddr
     {
       .sin_family = AF_INET,
       .sin_port = host_to_network(port),
       .sin_addr = {host_to_network(INADDR_ANY)}
     }
{}

socket_address::socket_address(const ::sockaddr_in& addr) noexcept
  :m_sockaddr{addr}
{
}

socket_address::socket_address(const std::string& ip, uint16_t port) noexcept
  :m_sockaddr
     {
       .sin_family = AF_INET,
       .sin_port = host_to_network(port),
       .sin_addr = ip_str_to_in_addr(ip)
     }
{}

socket_address::socket_address (socket_address&& other) noexcept
  :m_sockaddr{other.m_sockaddr}
{
  ::bzero(&other.m_sockaddr, sizeof(other.m_sockaddr));
}

socket_address& socket_address::operator=(socket_address&& other) noexcept
{
  m_sockaddr = other.m_sockaddr;
  std::memset(&other.m_sockaddr, 0, sizeof(other.m_sockaddr));
  return *this;
}

std::string socket_address::to_str() const noexcept
{
  char buf[32];
  sockaddr_in_to_c_str(buf, sizeof(buf), m_sockaddr);
  return buf;
}

std::ostream& operator<<(std::ostream& stream, const socket_address& address)
{
  stream<<address.to_str();
  return stream;
}

}
#endif // XYNET_SOCKET_ADDRESS_H
