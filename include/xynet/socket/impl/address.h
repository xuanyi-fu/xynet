//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_ADDRESS_H
#define XYNET_SOCKET_ADDRESS_H

#include "xynet/detail/endian.h"

namespace xynet
{
class socket_address
{
public:
  socket_address() noexcept
  :m_sockaddr
     {
       .sin_family = AF_INET
     }
  {}

  socket_address(uint16_t port) noexcept
  :m_sockaddr
     {
       .sin_family = AF_INET,
       .sin_port = detail::host_to_network(port),
       .sin_addr = {detail::host_to_network(INADDR_ANY)}
     }
  {}

  socket_address(const ::sockaddr_in& addr) noexcept
  :m_sockaddr{addr}
  {}

  socket_address(const std::string& ip, uint16_t port) noexcept
    :m_sockaddr
      {
        .sin_family = AF_INET,
        .sin_port = detail::host_to_network(port),
        .sin_addr = detail::ip_str_to_in_addr(ip)
      }
  {}

  socket_address (socket_address&& other) noexcept
    :m_sockaddr{other.m_sockaddr}
  {
    other.m_sockaddr = {};
  }

  socket_address (const socket_address&) noexcept = default;
  socket_address& operator=(const socket_address&) noexcept = default;

  socket_address& operator=(socket_address&& other) noexcept
  {
    m_sockaddr = other.m_sockaddr;
    other.m_sockaddr = {};
    return *this;
  }

  std::string to_str() const noexcept
  {
    char buf[32];
    detail::sockaddr_in_to_c_str(buf, sizeof(buf), m_sockaddr);
    return buf;
  }

  friend std::ostream& operator<<(std::ostream& stream, const socket_address& address)
  {
    stream<<address.to_str();
    return stream;
  }

  [[nodiscard]]
  const ::sockaddr_in* as_sockaddr_in() const noexcept
  {
    return reinterpret_cast<const ::sockaddr_in*>(this);
  }

  [[nodiscard]]
  uint16_t port() const
  {
    return ::ntohs(m_sockaddr.sin_port);
  } 

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

namespace xynet
{

template <typename T>
class address
{
public:
  void set_local_address(const socket_address& address) noexcept
  {
    m_local_address = address;
  }

  void set_peer_address(const socket_address& address) noexcept
  {
    m_peer_address = address;
  }

  [[nodiscard]]
  const socket_address& get_local_address() const noexcept
  {
    return m_local_address;
  }

  [[nodiscard]]
  const socket_address& get_peer_address() const noexcept
  {
    return m_peer_address;
  }

private:
  xynet::socket_address m_local_address;
  xynet::socket_address m_peer_address;
};

}
#endif //XYNET_SOCKET_ADDRESS_H
