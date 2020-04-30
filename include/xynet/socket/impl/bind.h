//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_BIND_H
#define XYNET_SOCKET_BIND_H

#include "xynet/detail/file_descriptor_traits.h"
#include "xynet/socket/detail/throw_or_return.h"
#include "xynet/socket/impl/address.h"

namespace xynet
{

template<detail::FileDescriptorPolicy P, typename F>
struct operation_bind
{
  template<typename F2 = F>
  decltype(auto) bind(const socket_address &socketAddress)
  noexcept(detail::FileDescriptorPolicyUseErrorCode < P > )
  {
    int ret = ::bind(
      static_cast<const F2 *>(this)->get(),
      reinterpret_cast<::sockaddr *>(const_cast<socket_address *>(&socketAddress)),
      sizeof(socket_address));

    if (ret == 0)
    {
      if constexpr (file_descriptor_has_module_v<std::decay_t<F2>, xynet::template address>)
      {
        static_cast<F2 *>(this)->set_local_address(socketAddress);
      }
    }

    return sync_throw_or_return<P>(ret);
  }
};

}

#endif //XYNET_BIND_H
