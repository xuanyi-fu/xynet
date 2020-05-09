//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_ACCEPT_H
#define XYNET_SOCKET_ACCEPT_H

#include "xynet/detail/async_operation.h"

namespace xynet
{

template<detail::FileDescriptorPolicy P, typename F>
struct operation_accept
{
  template<typename F2, bool enable_timeout = false>
  struct async_accept : public async_operation<P, async_accept<F2, enable_timeout>, enable_timeout>
  {

    async_accept(F& listen_socket, F2& peer_socket) noexcept
    requires(enable_timeout == false)
    : async_operation<P, async_accept<F2, enable_timeout>, enable_timeout>{}
    , m_listen_socket{listen_socket}
    , m_peer_socket{peer_socket}
    , m_peer_addr{}
    , m_addrlen{sizeof(::sockaddr_in)}
    {}

    template<typename Duration, bool enable_timeout2 = enable_timeout>
    requires (enable_timeout2 == true)
    async_accept(F& listen_socket, F2& peer_socket, Duration&& duration)
    : async_operation<P, async_accept<F2, enable_timeout>, enable_timeout>{std::forward<Duration>(duration)}
    , m_listen_socket{listen_socket}
    , m_peer_socket{peer_socket}
    , m_peer_addr{}
    , m_addrlen{sizeof(::sockaddr_in)}
    {}

    auto initial_check() const noexcept
    {
      return true;
    }

    auto try_start() noexcept
    {
      return [this](::io_uring_sqe *sqe)
      {
        ::io_uring_prep_accept(sqe,
                               m_listen_socket.get(),
                               reinterpret_cast<sockaddr *>(&m_peer_addr),
                               &m_addrlen,
                               SOCK_CLOEXEC);

        sqe->user_data = reinterpret_cast<uintptr_t>(this);
      };
    }

    auto get_result()
    noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
    -> detail::file_descriptor_operation_return_type_t<P>
    {
      if (async_operation_base::no_error_in_result())
      {
        m_peer_socket.set(async_operation_base::get_res());
        if constexpr(file_descriptor_has_module_v<std::decay_t<F2>, xynet::template address>)
        {
          m_peer_socket.set_peer_address(socket_address{m_peer_addr});
        }
      }
      return async_throw_or_return<P>(async_operation_base::get_error_code());
    }
  private:
    F&  m_listen_socket;
    F2& m_peer_socket;
    ::sockaddr_in m_peer_addr;
    ::socklen_t m_addrlen;
  };


  template<typename F2>
  [[nodiscard]]
  decltype(auto) accept(F2& peer_socket) noexcept 
  {
    return async_accept<F2, false>{*static_cast<F*>(this), peer_socket};
  }

  template<typename F2, typename Duration>
  [[nodiscard]]
  decltype(auto) accept_timeout(F2& peer_socket, Duration&& duration) noexcept 
  {
    return async_accept<F2, true>{*static_cast<F*>(this), peer_socket, std::forward<Duration>(duration)};
  }

};

}

#endif //XYNET_ACCEPT_H
