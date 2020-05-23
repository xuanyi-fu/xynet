//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_BIND_H
#define XYNET_SOCKET_BIND_H

#include "xynet/detail/file_descriptor_traits.h"
#include "xynet/socket/impl/address.h"
#include "xynet/detail/sync_operation.h"

namespace xynet
{

template<typename F>
struct operation_bind
{
  template<typename F2 = F>
  auto bind(const socket_address& address, std::error_code& error) -> void
  {
    detail::sync_operation
    (
      [
       fd   = static_cast<F2 *>(this)->get(),
       addr = reinterpret_cast<::sockaddr *>(const_cast<socket_address *>(&address)),
       size = sizeof(socket_address)
      ]
      ()
      {
        return ::bind(fd, addr, size);
      },
      [file = static_cast<F2 *>(this), addr = address](int ret)
      {
        if constexpr 
        (file_descriptor_has_module_v
        <
          std::remove_pointer_t<decltype(file)>, 
          xynet::template address
        >)
        {
          // if bind to zero, a random port will be assigned
          if(addr.port() == uint16_t{0})
          {
            auto addr2 = ::sockaddr_in{};
            socklen_t len   = sizeof(addr2);
            ::getsockname(file->get(), reinterpret_cast<sockaddr*>(&addr2), &len);
            file->set_local_address(socket_address{addr2});
          }
          else
          {
            file->set_local_address(addr);
          }
          
        }
      },
      error
    );
  }

  auto bind(const socket_address& address) -> void
  {
    auto error = std::error_code{};
    bind(address, error);
    if(error)
    {
      throw std::system_error{error};
    }
  }

};

}

#endif //XYNET_BIND_H
