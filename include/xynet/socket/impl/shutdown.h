//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SHUTDOWN_H
#define XYNET_SHUTDOWN_H

#include "xynet/detail/file_descriptor_traits.h"
#include "xynet/socket/detail/throw_or_return.h"

namespace xynet
{

template <detail::FileDescriptorPolicy P, typename T>
struct operation_shutdown
{
  decltype(auto) shutdown(int how = SHUT_WR)
  noexcept(detail::FileDescriptorPolicyUseErrorCode<P>)
  {
    int ret = ::shutdown(static_cast<const T*>(this)->get(), how);
    return sync_throw_or_return<P>(ret);
  }
};

}

#endif //XYNET_SHUTDOWN_H
