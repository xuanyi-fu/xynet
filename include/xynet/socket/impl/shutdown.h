//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SHUTDOWN_H
#define XYNET_SHUTDOWN_H

#include "xynet/detail/file_descriptor_traits.h"

namespace xynet
{

template <typename F>
struct operation_shutdown
{
  /// \brief same as shutdown(2). report error by error_code.
  /// \param[in]  flags indicates how will the socket be shutdown.
  /// \param[out] the std::error_code will be reset if there is an error. Otherwise, if
  ///             will be cleared.
  auto shutdown(int flags, std::error_code& error) -> void
  {
    detail::sync_operation
    (
      [fd = static_cast<F*>(this)->get(), flags]()
      {
        return ::shutdown(fd, flags);
      },
      []([[maybe_unused]]int){},
      error
    );
  }

  /// \brief same as shutdown(2), with how = SHUT_WR. report error by error_code.
  /// \param[out] the std::error_code will be reset if there is an error. Otherwise, if
  ///             will be cleared.
  auto shutdown(std::error_code& error) -> void
  {
    shutdown(SHUT_WR);
  }

  /// \brief same as shutdown(2), with how = SHUT_WR. report error by exception.
  auto shutdown(int flags = SHUT_WR) -> void
  {
    auto error = std::error_code{};
    shutdown(flags, error);
    if(error)
    {
      throw std::system_error{error};
    }
  }
};

}

#endif //XYNET_SHUTDOWN_H
