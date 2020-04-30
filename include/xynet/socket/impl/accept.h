//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_ACCEPT_H
#define XYNET_SOCKET_ACCEPT_H

#include "xynet/socket/detail/async_operation.h"

namespace xynet
{

template<detail::FileDescriptorPolicy P, typename F>
struct operation_accept
{
  template<typename F2>
  struct async_accept : public async_operation<P, async_accept<F2>>
  {
    using base = async_operation<P, async_accept<F2>>;

    async_accept(F& listen_socket, F2& peer_socket) noexcept
      : async_operation<P, async_accept<F2>>{}
      , m_listen_socket{listen_socket}
      , m_peer_socket{peer_socket}
      , m_peer_addr{}
      , m_addrlen{sizeof(::sockaddr_in)}
    {}

    void try_start()
    noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
    {
      auto accept = [this](::io_uring_sqe *sqe)
      {
        ::io_uring_prep_accept(sqe,
                               m_listen_socket.get(),
                               reinterpret_cast<sockaddr *>(&m_peer_addr),
                               &m_addrlen,
                               SOCK_CLOEXEC);

        sqe->user_data = reinterpret_cast<uintptr_t>(this);
      };
      async_operation_base::get_service()->try_submit_io(accept);
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
  decltype(auto) accept(F2& peer_socket)
  noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
  {
    return async_accept<F2>{*static_cast<F*>(this), peer_socket};
  }

};

}

#endif //XYNET_ACCEPT_H
