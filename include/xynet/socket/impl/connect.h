//
// Created by xuanyi on 5/4/20.
//

#ifndef XYNET_SOCKET_CONNECT_H
#define XYNET_SOCKET_CONNECT_H

#include "xynet/socket/detail/async_operation.h"

namespace xynet
{

template<detail::FileDescriptorPolicy P, typename F>
struct operation_connect
{
  template<bool enable_timeout = false>
  struct async_connect : public async_operation<P, async_connect<enable_timeout>, enable_timeout>
  {

    async_connect(F& socket, sockaddr_in addr) noexcept
    requires(enable_timeout == false)
    : async_operation<P, async_connect<enable_timeout>, enable_timeout>{}
    , m_socket{socket}
    , m_addr{addr}
    {}

    template<typename Duration, bool enable_timeout2 = enable_timeout>
    requires (enable_timeout2 == true)
    async_connect(F& socket, sockaddr_in addr, Duration&& duration)
    : async_operation<P, async_connect<enable_timeout>, enable_timeout>{std::forward<Duration>(duration)}
    , m_socket{socket}
    , m_addr{addr}
    {}

    auto initial_check() const noexcept
    {
      return true;
    }

    auto try_start() noexcept
    {
      return [this](::io_uring_sqe *sqe)
      {
        ::io_uring_prep_connect(sqe,
                               m_socket.get(),
                               reinterpret_cast<sockaddr *>(&m_addr),
                               m_addrlen);

        sqe->user_data = reinterpret_cast<uintptr_t>(this);
      };
    }

    auto get_result()
    noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
    -> detail::file_descriptor_operation_return_type_t<P>
    {
      if (async_operation_base::no_error_in_result())
      {
        if constexpr(file_descriptor_has_module_v<std::decay_t<F>, xynet::template address>)
        {
          m_socket.set_peer_address(socket_address{m_addr});
        }
      }
      return async_throw_or_return<P>(async_operation_base::get_error_code());
    }
  private:
    F&  m_socket;
    ::sockaddr_in m_addr;
    ::socklen_t m_addrlen = sizeof(::sockaddr_in);
  };


  [[nodiscard]]
  decltype(auto) connect(const socket_address& address) noexcept 
  {
    return async_connect<false>{*static_cast<F*>(this), *address.as_sockaddr_in()};
  }

  template<typename Duration>
  [[nodiscard]]
  decltype(auto) connect_timeout(const socket_address& address, Duration&& duration) noexcept 
  {
    return async_connect<true>{*static_cast<F*>(this), *address.as_sockaddr_in(), std::forward<Duration>(duration)};
  }

};

}

#endif //XYNET_ACCEPT_H
