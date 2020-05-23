//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_READ_H
#define XYNET_SOCKET_READ_H

#include <type_traits>
#include "xynet/buffer.h"
#include "xynet/detail/async_operation.h"
#include "xynet/detail/file_descriptor_traits.h"

namespace xynet
{

template<bool enable_recv_some, typename BufferType, typename BasePolicy>
struct async_recvmsg_policy : public BasePolicy::policy_type
{
  using recv_some_type = std::conditional_t<enable_recv_some, std::true_type, std::false_type>;
  using buffer_type = BufferType;
};

template<typename Policy, typename F>
struct async_recvmsg : public async_operation<Policy, async_recvmsg<Policy, F>>
{
  template<typename... Args>
  async_recvmsg(F& socket, Args&&... args) noexcept
    :async_operation<Policy, async_recvmsg<Policy, F>>{&async_recvmsg::on_recv_completed}
    ,m_socket{socket}
    ,m_buffers{std::forward<Args>(args)...}
    ,m_bytes_transferred{}
    ,m_msghdr{.msg_iov = m_buffers.get_iov_ptr(), .msg_iovlen = m_buffers.get_iov_cnt()}
  {}

  template<typename... Args>
  async_recvmsg(F& socket, std::error_code& error, Args&&... args) noexcept
    :async_operation<Policy, async_recvmsg<Policy, F>>{&async_recvmsg::on_recv_completed, error}
    ,m_socket{socket}
    ,m_buffers{std::forward<Args>(args)...}
    ,m_bytes_transferred{}
    ,m_msghdr{.msg_iov = m_buffers.get_iov_ptr(), .msg_iovlen = m_buffers.get_iov_cnt()}
  {}

  template<DurationType Duration, typename... Args>
  async_recvmsg(F& socket, Duration&& duration, Args&&... args) noexcept
    :async_operation<Policy, async_recvmsg<Policy, F>>
      {&async_recvmsg::on_recv_completed, std::forward<Duration>(duration)}
    ,m_socket{socket}
    ,m_buffers{std::forward<Args>(args)...}
    ,m_bytes_transferred{}
    ,m_msghdr{.msg_iov = m_buffers.get_iov_ptr(), .msg_iovlen = m_buffers.get_iov_cnt()}
  {}

  template<DurationType Duration, typename... Args>
  async_recvmsg(F& socket, Duration&& duration, std::error_code& error, Args&&... args) noexcept
    :async_operation<Policy, async_recvmsg<Policy, F>>
      {&async_recvmsg::on_recv_completed, std::forward<Duration>(duration), error}
    ,m_socket{socket}
    ,m_buffers{std::forward<Args>(args)...}
    ,m_bytes_transferred{}
    ,m_msghdr{.msg_iov = m_buffers.get_iov_ptr(), .msg_iovlen = m_buffers.get_iov_cnt()}
  {}

  auto initial_check() const noexcept
  {
    return true;
  }


  static void on_recv_completed(async_operation_base *base) noexcept
  {
    auto *op = static_cast<async_recvmsg*>(base);
    if constexpr (Policy::recv_some_type::value)
    {
      auto ret = base->get_res();
      op->m_bytes_transferred = ret >= 0 ? ret : 0;
      async_operation_base::on_operation_completed(base);
    }
    else
    {
      op->update_result();
    }
  }

  [[nodiscard]]
  auto try_start() noexcept
  {
    return [this](::io_uring_sqe* sqe)
    {
      ::io_uring_prep_recvmsg(sqe,
                              m_socket.get(),
                              &m_msghdr,
                              0);

      sqe->user_data = reinterpret_cast<uintptr_t>(this);
    };
  }

  void update_result()
  {
    if(async_operation_base::get_error_code() 
      || async_operation_base::get_res() == 0)
    {
      async_operation_base::get_awaiting_coroutine().resume();
    }
    else
    {
      m_bytes_transferred += async_operation_base::get_res();
      m_buffers.commit(async_operation_base::get_res());
      std::tie(m_msghdr.msg_iov,
                m_msghdr.msg_iovlen) = m_buffers.get_iov_span();
      if(m_msghdr.msg_iov == nullptr)
      {
        async_operation_base::get_awaiting_coroutine().resume();
      }
      else
      {
        async_operation<Policy, async_recvmsg<Policy, F>>::submit();
      }
    }
  }

  auto get_result() noexcept (Policy::error_code_type::value) -> std::size_t
  {
    if(async_operation_base::get_res() == 0)
    {
      async_operation_base::get_error_code() = 
        xynet_error_instance::make_error_code(xynet_error::eof);
    }
    
    if constexpr (!Policy::error_code_type::value)
    {
      if(auto& error = async_operation_base::get_error_code(); error)
      {
        throw std::system_error{error};
      }
    }

    return m_bytes_transferred;
  }

private:
  F& m_socket;
  Policy::buffer_type m_buffers;
  int m_bytes_transferred;
  ::msghdr m_msghdr;
};

template<typename F>
struct operation_recv
{
  template<typename... Args>
  [[nodiscard]]
  decltype(auto) recv(Args&&... args) noexcept
  {
    using policy = async_recvmsg_policy<false, decltype(buffer_sequence{std::forward<Args>(args)...}), 
      async_operation_traits<>>;
    return async_recvmsg<policy, F>{*static_cast<F*>(this), std::forward<Args>(args)...};
  }

  template<typename... Args>
  [[nodiscard]]
  decltype(auto) recv(std::error_code& error, Args&&... args) noexcept
  {
    using policy = async_recvmsg_policy<false, decltype(buffer_sequence{std::forward<Args>(args)...}),
      async_operation_traits<std::error_code>>;
    return async_recvmsg<policy, F>{*static_cast<F*>(this), error, std::forward<Args>(args)...};
  }

  template<DurationType Duration, typename... Args>
  [[nodiscard]]
  decltype(auto) recv(Duration&& duration, Args&&... args) noexcept
  {
    using policy = async_recvmsg_policy<false, decltype(buffer_sequence{std::forward<Args>(args)...}), 
        async_operation_traits<Duration>>;
    return async_recvmsg<policy, F>{*static_cast<F*>(this), std::forward<Duration>(duration), std::forward<Args>(args)...};
  }

  template<DurationType Duration, typename... Args>
  [[nodiscard]]
  decltype(auto) recv(Duration&& duration, std::error_code& error, Args&&... args) noexcept
  {
    using policy = async_recvmsg_policy<false, decltype(buffer_sequence{std::forward<Args>(args)...}), 
        async_operation_traits<Duration, std::error_code>>;
    return async_recvmsg<policy, F>{*static_cast<F*>(this), std::forward<Duration>(duration), error, std::forward<Args>(args)...};
  }

  // recv some

  template<typename... Args>
  [[nodiscard]]
  decltype(auto) recv_some(Args&&... args) noexcept
  {
    using policy = async_recvmsg_policy<true, decltype(buffer_sequence{std::forward<Args>(args)...}), 
      async_operation_traits<>>;
    return async_recvmsg<policy, F>{*static_cast<F*>(this), std::forward<Args>(args)...};
  }

  template<typename... Args>
  [[nodiscard]]
  decltype(auto) recv_some(std::error_code& error, Args&&... args) noexcept
  {
    using policy = async_recvmsg_policy<true, decltype(buffer_sequence{std::forward<Args>(args)...}), 
      async_operation_traits<std::error_code>>;
    return async_recvmsg<policy, F>{*static_cast<F*>(this), error, std::forward<Args>(args)...};
  }

  template<DurationType Duration, typename... Args>
  [[nodiscard]]
  decltype(auto) recv_some(Duration&& duration, Args&&... args) noexcept
  {
    using policy = async_recvmsg_policy<true, decltype(buffer_sequence{std::forward<Args>(args)...}), 
      async_operation_traits<Duration>>;
    return async_recvmsg<policy, F>{*static_cast<F*>(this), std::forward<Duration>(duration), std::forward<Args>(args)...};
  }

  template<DurationType Duration, typename... Args>
  [[nodiscard]]
  decltype(auto) recv_some(Duration&& duration, std::error_code& error, Args&&... args) noexcept
  {
    using policy = async_recvmsg_policy<true, decltype(buffer_sequence{std::forward<Args>(args)...}), 
      async_operation_traits<Duration, std::error_code>>;
    return async_recvmsg<policy, F>{*static_cast<F*>(this), std::forward<Duration>(duration), error, std::forward<Args>(args)...};
  }

};

}

#endif //XYNET_READ_H
