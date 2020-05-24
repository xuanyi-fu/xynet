//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_SETSOCKOPT_H
#define XYNET_SOCKET_SETSOCKOPT_H

#include "xynet/detail/file_descriptor_traits.h"
#include "xynet/detail/sync_operation.h"

namespace xynet
{

template <typename T>
struct operation_set_options
{
  /// \brief same as setsockopt(2). report error by error_code.
  /// \param[out] the std::error_code will be reset if there is an error. Otherwise, if
  ///             will be cleared.
  auto setsockopt(int level, int optname, const void* optval, socklen_t optlen, std::error_code& error) noexcept 
  -> void
  {
    detail::sync_operation
    (
      [fd = static_cast<const T*>(this)->get(), level, optname, optval, optlen]()
      {
        return ::setsockopt(fd, level,
          optname, optval, optlen);
      },
      []([[maybe_unused]]int ret){},
      error
    );
  }

  /// \brief same as setsockopt(2). report error by exception.
  auto setsockopt(int level, int optname, const void* optval, socklen_t optlen) 
  -> void
  {
    auto error = std::error_code{};
    setsockopt(level, optname, optval, optlen, error);
    if(error)
    {
      throw std::system_error{error};
    }
  }

  /// \brief set the socket option SO_REUSEADDR. report error by error_code
  /// \param[out] the std::error_code will be reset if there is an error. Otherwise, if
  ///             will be cleared.
  auto reuse_address(std::error_code& error) noexcept -> void
  {
    int optval = 1;
    setsockopt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval), error);
  }

  /// \brief set the socket option SO_REUSEADDR. report error by exception
  auto reuse_address() -> void
  {
    int optval = 1;
    setsockopt(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  }
};

}

#endif //XYNET_SETSOCKOPT_H
