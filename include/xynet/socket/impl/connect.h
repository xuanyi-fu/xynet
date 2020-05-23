//
// Created by xuanyi on 5/4/20.
//

#ifndef XYNET_SOCKET_CONNECT_H
#define XYNET_SOCKET_CONNECT_H

#include "xynet/detail/async_operation.h"

namespace xynet
{

template<typename Policy, typename F>
struct async_connect : public async_operation<Policy, async_connect<Policy, F>>
{
  template<typename... Args>
  async_connect(F& socket, sockaddr_in addr, Args&&... args) noexcept
  : async_operation<Policy, async_connect<Policy, F>>{std::forward<Args>(args)...}
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

  decltype(auto) get_result()
  noexcept (Policy::error_code_type::value)
  {
    if (!async_operation_base::get_error_code())[[likely]]
    {
      if constexpr(file_descriptor_has_module_v<std::decay_t<F>, xynet::template address>)
      {
        m_socket.set_peer_address(socket_address{m_addr});
      }
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
  F&  m_socket;
  ::sockaddr_in m_addr;
  ::socklen_t m_addrlen = sizeof(::sockaddr_in);
};

template<typename F, typename... Args>
async_connect(F& socket, sockaddr_in addr, Args&&... args) noexcept
-> async_connect<typename async_operation_traits<std::decay_t<Args>...>::policy_type, F>;

template<typename F>
struct operation_connect
{
  template<typename... Args>
  [[nodiscard]]
  decltype(auto) connect(const socket_address& address, Args&&... args) noexcept 
  {
    return async_connect{*static_cast<F*>(this), *address.as_sockaddr_in(), std::forward<Args>(args)...};
  }
};

}

#endif //XYNET_ACCEPT_H
