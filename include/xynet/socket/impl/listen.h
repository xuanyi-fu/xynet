//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_LISTEN_H
#define XYNET_SOCKET_LISTEN_H

#include "xynet/detail/file_descriptor_traits.h"
#include "xynet/socket/detail/throw_or_return.h"

namespace xynet
{

template<detail::FileDescriptorPolicy P, typename F>
struct operation_listen
{

  decltype(auto) listen(int backlog = SOMAXCONN)
  noexcept(detail::FileDescriptorPolicyUseErrorCode < P > )
  {
    int ret = ::listen(static_cast<const F *>(this)->get(), backlog);
    return sync_throw_or_return<P>(ret);
  }
};

}

#endif //XYNET_LISTEN_H
