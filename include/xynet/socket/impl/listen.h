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

  auto listen(std::error_code& error) -> void
  {
    listen(SOMAXCONN, error);
  }

  auto listen(int backlog) -> void
  {
    auto error = std::error_code{};
    listen(backlog, error);
    if(error)
    {
      throw std::system_error{error};
    }
  }

  auto listen() -> void
  {
    listen(SOMAXCONN);
  }

};

}

#endif //XYNET_LISTEN_H
