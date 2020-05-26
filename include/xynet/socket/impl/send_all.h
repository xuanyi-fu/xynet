//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_SEND_ALL_H
#define XYNET_SOCKET_SEND_ALL_H

#include <type_traits>
#include <cstdio>
#include "xynet/buffer.h"
#include "xynet/detail/async_operation.h"
#include "xynet/detail/file_descriptor_traits.h"

namespace xynet
{

template<typename BufferType, typename BasePolicy>
struct async_sendmsg_policy : public BasePolicy::policy_type
{
  using buffer_type = BufferType;
};

template<typename Policy, typename F>
struct async_sendmsg : public async_operation<Policy, async_sendmsg<Policy, F>>
{
  template<typename... Args>
  async_sendmsg(F& socket, Args&&... args) noexcept
  :async_operation<Policy, async_sendmsg<Policy, F>>
    {&async_sendmsg::on_send_completed}
  ,m_socket{socket}
  ,m_buffers{static_cast<Args&&>(args)...}
  ,m_bytes_transferred{}
  ,m_msghdr{.msg_iov = m_buffers.get_iov_ptr(), .msg_iovlen = m_buffers.get_iov_cnt()}
  {}

  template<typename... Args>
  async_sendmsg(F& socket, std::error_code& error, Args&&... args) noexcept
  :async_operation<Policy, async_sendmsg<Policy, F>>
    {&async_sendmsg::on_send_completed, error}
  ,m_socket{socket}
  ,m_buffers{static_cast<Args&&>(args)...}
  ,m_bytes_transferred{}
  ,m_msghdr{.msg_iov = m_buffers.get_iov_ptr(), .msg_iovlen = m_buffers.get_iov_cnt()}
  {}

  template<DurationType Duration, typename... Args>
  async_sendmsg(F& socket, Duration&& duration, Args&&... args) noexcept
  :async_operation<Policy, async_sendmsg<Policy, F>>
    {&async_sendmsg::on_send_completed, std::forward<Duration>(duration)}
  ,m_socket{socket}
  ,m_buffers{static_cast<Args&&>(args)...}
  ,m_bytes_transferred{}
  ,m_msghdr{.msg_iov = m_buffers.get_iov_ptr(), .msg_iovlen = m_buffers.get_iov_cnt()}
  {}

  template<DurationType Duration, typename... Args>
  async_sendmsg(F& socket, std::error_code& error, Duration&& duration, Args&&... args) noexcept
  :async_operation<Policy, async_sendmsg<Policy, F>>
    {&async_sendmsg::on_send_completed, std::forward<Duration>(duration), error}
  ,m_socket{socket}
  ,m_buffers{static_cast<Args&&>(args)...}
  ,m_bytes_transferred{}
  ,m_msghdr{.msg_iov = m_buffers.get_iov_ptr(), .msg_iovlen = m_buffers.get_iov_cnt()}
  {}

  auto initial_check() const noexcept
  {
    return true;
  }

  static void on_send_completed(async_operation_base *base) noexcept
  {
    auto *op = static_cast<async_sendmsg*>(base);
    op->update_result();
  }

  [[nodiscard]]
  auto try_start() noexcept 
  {
    return [this](::io_uring_sqe* sqe)
    {
      ::io_uring_prep_sendmsg(sqe,
                              m_socket.get(),
                              &m_msghdr,
                              0);

      sqe->user_data = reinterpret_cast<uintptr_t>(this);
    };
  }

  void update_result()
  {
    if(async_operation_base::get_error_code() 
     ||async_operation_base::get_res() == 0 )
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
        async_operation<Policy, async_sendmsg<Policy, F>>::submit();
      }
    }
  }

  auto get_result() noexcept (Policy::error_code_type::value) -> std::size_t
  {
    if constexpr (Policy::error_code_type::value)
    {
      return m_bytes_transferred;
    }
    else
    {
      if(!async_operation_base::get_error_code())[[likely]]
      {
        return m_bytes_transferred;
      }
      else[[unlikely]]
      {
        throw std::system_error{async_operation_base::get_error_code()};
      }
      
    }

  }

private:
  F& m_socket;
  Policy::buffer_type m_buffers;
  int m_bytes_transferred;
  ::msghdr m_msghdr = ::msghdr{.msg_flags = MSG_NOSIGNAL};
};

template<typename F>
struct operation_send
{
  /// \brief      create an awaiter for calling sendmsg(2) asynchronously.
  /// \param[out] args buffers that satisfy the following requirements:
  ///             If there is one parameter in args, this parameter could be:
  ///               - An lvalue reference of a type 'T', which satisfies std::ranges::viewable_range<T&>
  ///                 && std::ranges::contiguous_range<std::ranges::range_value_t<T>>. That is, T must be an
  ///                 lvalue reference of a viewable range of contiguous ranges.
  ///               - A contiguous range that can be used to construct a std::span<byte, size or std::dynamic_extent>.
  ///             If there is more than one parameters in args, they must be contiguous ranges that can be used to 
  ///             construct std::span<const byte, size or std::dynamic_extent>'s.
  ///             
  /// \note       - The content of buffers will be sent in the same order as they were in the lvalue reference 
  ///               of a viewable range or the order they were in the parameter pack args. 
  ///             - The operation will finish if there is an error(including the socket reads EOF) or all 
  ///               the content of buffers are sent. That is, it may call sendmsg(2) more than once.
  ///             - After the operation is co_await'ed and then finish, it will return the bytes transferred. 
  template<typename... Args>
  [[nodiscard]]
  decltype(auto) send(Args&&... args) noexcept
  {
    using policy = 
      async_sendmsg_policy
      <
        decltype(const_buffer_sequence{std::forward<Args>(args)...}),
        async_operation_traits<>
      >;
    return async_sendmsg<policy, F>
      {*static_cast<F*>(this), std::forward<Args>(args)...};
  }

  /// \brief same as send(Args&&... args), but use std::error_code to report error.
  /// \param[out] the std::error_code will be reset if there is an error. Otherwise, if
  ///             will be cleared.
  template<typename... Args>
  [[nodiscard]]
  decltype(auto) send(std::error_code& error, Args&&... args) noexcept
  {
    using policy = 
      async_sendmsg_policy
      <
        decltype(const_buffer_sequence{std::forward<Args>(args)...}),
        async_operation_traits<std::error_code>
      >;
    return async_sendmsg<policy, F>
      {*static_cast<F*>(this), error, std::forward<Args>(args)...};
  }

  /// \brief same as send(Args&&... args), but imposes a timout on each single send operation.
  /// \param[in] duration If one single sendmsg(2) does not finish within the given duration, the operation will be 
  ///                     canneled and an error_code (std::errc::operation_canceled) will be returned. 
  template<DurationType Duration, typename... Args>
  [[nodiscard]]
  decltype(auto) send(Duration&& duration, Args&&... args) noexcept
  {
    using policy = 
      async_sendmsg_policy
      <
        decltype(const_buffer_sequence{std::forward<Args>(args)...}),
        async_operation_traits<Duration>
      >;
    return async_sendmsg<policy, F>
      {*static_cast<F*>(this), std::forward<Duration>(duration), std::forward<Args>(args)...};
  }


  /// \brief same as send(Args&&... args), but imposes a timout on each single recv operation and reports error
  ///        by std::error_code
  /// \param[out] the std::error_code will be reset if there is an error. Otherwise, if
  ///             will be cleared.
  /// \param[in] duration If one single sendmsg(2) does not finish within the given duration, the operation will be 
  ///                     canneled and an error_code (std::errc::operation_canceled) will be returned.
  template<DurationType Duration, typename... Args>
  [[nodiscard]]
  decltype(auto) send(Duration&& duration, std::error_code& error, Args&&... args) noexcept
  {
    using policy = 
      async_sendmsg_policy
      <
        decltype(const_buffer_sequence{std::forward<Args>(args)...}),
        async_operation_traits<Duration, std::error_code>
      >;
    return async_sendmsg<policy, F>
      {*static_cast<F*>(this), std::forward<Duration>(duration), error, std::forward<Args>(args)...};
  }

};

}
#endif //XYNET_SEND_ALL_H
