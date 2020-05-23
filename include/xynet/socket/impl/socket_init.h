//
// Created by Xuanyi Fu on 4/15/20.
//

#ifndef XYNET_TCP_SOCKET_H
#define XYNET_TCP_SOCKET_H

#include <sys/socket.h>
#include <netinet/ip.h>

#include "xynet/detail/file_descriptor_traits.h"
#include "xynet/detail/sync_operation.h"

namespace xynet
{

template<typename F>
struct socket_init
{
  auto init(std::error_code& error) noexcept -> void
  {
    // we have to close the open socket first
    if(auto fd = static_cast<F*>(this)->get(); fd != -1)
    {
      ::close(fd);
    }
    
    detail::sync_operation
    (
      []()
      {
        return ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
      }, 
      [file = static_cast<F*>(this)](int fd)
      {
        file->set(fd);
      },
      error
    );
  }

  auto init() -> void
  {
    auto error = std::error_code{};
    init(error);
    if(error)
    {
      throw std::system_error{error};
    }
  }

};

}

#endif //XYNET_TCP_SOCKET_H



