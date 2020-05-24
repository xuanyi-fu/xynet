//
// Created by xuanyi on 5/4/20.
//

#ifndef XYNET_SOCKET_CONNECT_H
#define XYNET_SOCKET_CONNECT_H

#include "xynet/detail/async_operation.h"

namespace xynet
{

template<typename Policy, typename F>
class async_connect : public async_operation<Policy, async_connect<Policy, F>>
{
public:
  template<typename... Args>
  async_connect(F& socket, sockaddr_in addr, Args&&... args) noexcept
  : async_operation<Policy, async_connect<Policy, F>>{std::forward<Args>(args)...}
  , m_socket{socket}
  , m_addr{addr}
  {}
private:
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
    else[[unlikely]]
    {
      if constexpr (!Policy::error_code_type::value)
      {
        throw std::system_error{async_operation_base::get_error_code()};
      }
    }
  }

  friend async_operation<Policy, async_connect<Policy, F>>;
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
  /// \brief      Create an awaiter to establishe a socket connection
  /// \param[in]  address the address with which the connection will be established.
  /// \param      args        
  /// 
  /// 1. Args is void: 
  ///   The awaiter returned by the function will throw exception to report the error.
  ///   A std::system_error constructed with the corresponding std::error_code will be 
  ///   throwed if the accept operation is failed after being co_await'ed.
  ///  
  /// 2. Args is a Duration, i.e. std::chrono::duration<Rep, Period>. 
  ///   This duration will be treated as the timeout for the operation. If the operation 
  ///   does not finish within the given duration, the operation will be canneled and an error_code
  ///   (std::errc::operation_canceled) will be returned. 
  ///   
  /// 3. Args is an lvalue reference of a std::error_code
  ///   The awaiter returned by the function will use std::error_code to report the error.
  ///   Should there be an error in the operation, the std::error_code passed by lvalue reference will 
  ///   be reset. Otherwise, it will be clear.
  /// 
  /// 4. Args are first a Duration, second an lvalue reference of a std::error_code
  ///   The operation will have the features described in 2 and 3.

  template<typename... Args>
  [[nodiscard]]
  decltype(auto) connect(const socket_address& address, Args&&... args) noexcept 
  {
    return async_connect{*static_cast<F*>(this), *address.as_sockaddr_in(), std::forward<Args>(args)...};
  }
};

}

#endif //XYNET_ACCEPT_H
