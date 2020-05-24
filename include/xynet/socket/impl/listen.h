//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_LISTEN_H
#define XYNET_SOCKET_LISTEN_H

#include <sys/socket.h>
#include "xynet/detail/file_descriptor_traits.h"

namespace xynet
{

template<typename F>
struct operation_listen
{
  /// \brief      put the socket into listen state by calling listen(2) on the fd of the socket.
  ///             Report error by error_code.
  /// 
  /// \param[in]  backlog defines the maximum length to which the queue of pending connections 
  ///             for sockfd may grow.
  /// \param[out] the std::error_code will be reset if there is an error. Otherwise, if
  ///             will be cleared.
  auto listen(int backlog, std::error_code& error) -> void
  {
    detail::sync_operation
    (
      [fd = static_cast<const F *>(this)->get(), backlog]()
      {
        return ::listen(fd, backlog);
      },
      []([[maybe_unused]]int ret){},
      error
    );
  }

  /// \brief      put the socket into listen state by calling listen(2) on the fd of the socket.
  ///             Backlog is SOMAXCONN. Report error by error_code.
  ///
  /// \param[out] the std::error_code will be reset if there is an error. Otherwise, if
  ///             will be cleared.
  auto listen(std::error_code& error) -> void
  {
    listen(SOMAXCONN, error);
  }

  /// \brief      put the socket into listen state by calling listen(2) on the fd of the socket.
  ///             Report error by exception.
  ///
  /// \param[in]  backlog defines the maximum length to which the queue of pending connections 
  ///             for sockfd may grow.
  auto listen(int backlog) -> void
  {
    auto error = std::error_code{};
    listen(backlog, error);
    if(error)
    {
      throw std::system_error{error};
    }
  }

  /// \brief      put the socket into listen state by calling listen(2) on the fd of the socket.
  ///             Backlog is SOMAXCONN. Report error by exception.
  auto listen() -> void
  {
    listen(SOMAXCONN);
  }

};

}

#endif //XYNET_LISTEN_H
