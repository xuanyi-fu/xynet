//
// Created by Xuanyi Fu on 4/15/20.
//

#ifndef XYNET_TCP_SOCKET_H
#define XYNET_TCP_SOCKET_H

#include <sys/socket.h>
#include <system_error>

#include "socket_address.h"
#include "xynet/socket/detail/throw_or_return.h"
#include "xynet/file_descriptor.h"
#include "xynet/detail/file_descriptor_traits.h"

namespace xynet
{

template<detail::FileDescriptorPolicy P, typename F>
struct socket_init
{
  [[nodiscard]]
  decltype(auto) init()
  noexcept(detail::FileDescriptorPolicyUseErrorCode<P>)
  {
    auto fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if(fd >= 0)
    {
      static_cast<F*>(this)->set(fd);

      /* TODO:
       * Do we need to do the syscall getsockname() to get the local
       * socket_init port and fill in the address ?
       * This is going to be very expensive.
       * Another Choice here is to use another policy to decide whether
       * fill in it or not ...
       */
    }

    return sync_throw_or_return<P>(fd);

  }
};

}

#endif //XYNET_TCP_SOCKET_H



