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
  /// \brief initialize the socket with IPv4 & TCP. That is, call ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP)
  ///         and set the fd of file_descriptor.
  /// \param[out] the std::error_code will be reset if there is an error. Otherwise, if
  ///             will be cleared.
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

  /// \brief initialize the socket with IPv4 & TCP. That is, call ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP)
  ///         and set the fd of file_descriptor. Report error by exception.
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



