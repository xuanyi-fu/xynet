//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_DETAIL_THROW_OR_RETURN_H
#define XYNET_SOCKET_DETAIL_THROW_OR_RETURN_H

#include "xynet/detail/file_descriptor_traits.h"

namespace xynet
{
class io_service;
}

namespace xynet::detail
{

template <detail::FileDescriptorPolicy P, typename... R>
auto async_throw_or_return(std::error_code errc, R... args)
-> detail::file_descriptor_operation_return_type_t<P, R...>
{
  if constexpr (detail::FileDescriptorPolicyUseErrorCode<P>)
  {
    return {errc, args...};
  }
  else
  {
    if(errc)
    {
      throw std::system_error{errc};
    }
    else
    {
      if constexpr (sizeof...(R) == 0)
      {
        return;
      }
      else
      {
        return {args...};
      }
    }
  }
}

template<detail::FileDescriptorPolicy P>
auto sync_throw_or_return(int ret)
noexcept(detail::FileDescriptorPolicyUseErrorCode<P>)
-> detail::file_descriptor_operation_return_type_t<P>
{
  if(ret < 0)
  {
    int saved_errno = errno;
    auto errc = std::error_code{saved_errno, std::system_category()};

    if constexpr (detail::FileDescriptorPolicyUseErrorCode<P>)
    {
      return errc;
    }
    else
    {
      throw std::system_error{errc};
    }
  }
  else
  {
    if constexpr (detail::FileDescriptorPolicyUseErrorCode<P>)
    {
      return std::error_code{};
    }
  }
}

}

#endif //XYNET_THROW_OR_RETURN_H
