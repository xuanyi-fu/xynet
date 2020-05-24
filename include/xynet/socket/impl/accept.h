//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_ACCEPT_H
#define XYNET_SOCKET_ACCEPT_H

#include "xynet/detail/async_operation.h"

namespace xynet
{

template<typename Policy, typename F, typename F2>
class async_accept : public async_operation<Policy, async_accept<Policy, F, F2>>
{
public:
  template<typename... Args>
  async_accept(F& listen_socket, F2& peer_socket, Args&&... args) noexcept
  : async_operation<Policy, async_accept<Policy, F, F2>>{std::forward<Args>(args)...}
  , m_listen_socket{listen_socket}
  , m_peer_socket{peer_socket}
  , m_peer_addr{}
  , m_addrlen{sizeof(::sockaddr_in)}
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

      // reset the address module of the peer_socket if it has one.
      if constexpr(file_descriptor_has_module_v<std::decay_t<F2>, xynet::template address>)
      {
        m_peer_socket.set_peer_address(socket_address{m_peer_addr});
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

  friend async_operation<Policy, async_accept<Policy, F, F2>>;
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
  /// \brief      Create an awaiter to accept a new connection.
  /// \param[out] peer_socket The socket into which the new connection will be accpeted.
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

  template<typename F2, typename... Args>
  [[nodiscard]]
  decltype(auto) accept(F2& peer_socket, Args&&... args) noexcept 
  {
    return async_accept{*static_cast<F*>(this), peer_socket, std::forward<Args>(args)...};
  }
};

}

#endif //XYNET_ACCEPT_H
