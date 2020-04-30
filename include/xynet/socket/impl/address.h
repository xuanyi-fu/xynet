//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_ADDRESS_H
#define XYNET_SOCKET_ADDRESS_H

#include "xynet/socket/impl/socket_address.h"

namespace xynet
{

template <detail::FileDescriptorPolicy P, typename T>
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
