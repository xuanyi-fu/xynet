//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_ACCEPT_H
#define XYNET_SOCKET_ACCEPT_H

#include "xynet/detail/async_operation.h"

namespace xynet
{

template<typename Policy, typename F, typename F2>
struct async_accept : public async_operation<Policy, async_accept<Policy, F, F2>>
{

  template<typename... Args>
  async_accept(F& listen_socket, F2& peer_socket, Args&&... args) noexcept
  : async_operation<Policy, async_accept<Policy, F, F2>>{std::forward<Args>(args)...}
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

  decltype(auto) get_result() noexcept (Policy::error_code_type::value)
  {
    if (!async_operation_base::get_error_code())[[likely]]
    {
      m_peer_socket.set(async_operation_base::get_res());
      if constexpr(file_descriptor_has_module_v<std::decay_t<F2>, xynet::template address>)
      {
        m_peer_socket.set_peer_address(socket_address{m_peer_addr});
      }

      return;
    }
    else
    {
      if constexpr (Policy::error_code_type::value)
      {
        return;
      }
      else
      {
        throw std::system_error{async_operation_base::get_error_code()};
      }
    }
  }
private:
  F&  m_listen_socket;
  F2& m_peer_socket;
  ::sockaddr_in m_peer_addr;
  ::socklen_t m_addrlen;
};

template<typename F, typename F2, typename... Args>
async_accept(F& listen_socket, F2& peer_socket, Args&&... args) noexcept 
-> async_accept<typename async_operation_traits<std::decay_t<Args>...>::policy_type, F, F2>;


template<typename F>
struct operation_accept
{
  template<typename F2, typename... Args>
  [[nodiscard]]
  decltype(auto) accept(F2& peer_socket, Args&&... args) noexcept 
  {
    return async_accept{*static_cast<F*>(this), peer_socket, std::forward<Args>(args)...};
  }
};

}

#endif //XYNET_ACCEPT_H
