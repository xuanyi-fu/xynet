//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_SETSOCKOPT_H
#define XYNET_SOCKET_SETSOCKOPT_H

#include "xynet/detail/file_descriptor_traits.h"
#include "xynet/socket/detail/throw_or_return.h"

namespace xynet
{

template <detail::FileDescriptorPolicy P, typename T>
struct operation_set_options
{
private:
  decltype(auto) set_options(int level, int optname, const void* optval, socklen_t optlen)
  noexcept(detail::FileDescriptorPolicyUseErrorCode<P>)
  {
    int ret = ::setsockopt(
      static_cast<const T*>(this)->get(),
      level,
      optname,
      optval,
      optlen
    );

    return sync_throw_or_return<P>(ret);
  }

public:
  decltype(auto) reuse_address()
  noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
  {
    int optval = 1;
    return set_options(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  }
};

}

#endif //XYNET_SETSOCKOPT_H
