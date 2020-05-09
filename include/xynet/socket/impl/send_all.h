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

template<detail::FileDescriptorPolicy P, typename F>
struct operation_send
{
  template<typename BufferSequence, bool enable_timeout_each>
  struct async_sendmsg 
  : public async_operation<P, 
  async_sendmsg<BufferSequence, enable_timeout_each>, enable_timeout_each>
  {
    template<typename... Args>
    async_sendmsg(F& socket, Args&&... args) noexcept
    requires (enable_timeout_each == false)
      :async_operation<P, async_sendmsg, enable_timeout_each>
      {&async_sendmsg::on_send_completed}
      ,m_socket{socket}
      ,m_buffers{static_cast<Args&&>(args)...}
      ,m_bytes_transferred{}
    {
      std::tie(m_msghdr.msg_iov,
               m_msghdr.msg_iovlen) = m_buffers.get_iov_span();
    }

    template<
    typename Duration, 
    bool enable_timeout_each2 = enable_timeout_each,
    typename... Args
    >
    async_sendmsg(F& socket, Duration&& duration, Args&&... args) noexcept
    requires (enable_timeout_each2 == true)
    :async_operation<P, async_sendmsg, enable_timeout_each>
    {&async_sendmsg::on_send_completed, 
    std::forward<Duration>(duration)}
    ,m_socket{socket}
    ,m_buffers{static_cast<Args&&>(args)...}
    ,m_bytes_transferred{}
    {
      std::tie(m_msghdr.msg_iov,
          m_msghdr.msg_iovlen) = m_buffers.get_iov_span();
    }

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
    auto try_start()
    noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
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
      if(!async_operation_base::no_error_in_result() || 
      async_operation_base::get_res() == 0)
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
          async_operation<P, async_sendmsg<BufferSequence, enable_timeout_each>, 
  enable_timeout_each>::submit();
        }
      }
    }

    auto get_result()
    noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
    ->detail::file_descriptor_operation_return_type_t<P, std::size_t>
    {
      return async_throw_or_return<P>(async_operation_base::get_error_code(), 
      static_cast<std::size_t>(m_bytes_transferred));
    }

  private:
    F& m_socket;
    BufferSequence m_buffers;
    int m_bytes_transferred;
    ::msghdr m_msghdr = ::msghdr{.msg_flags = MSG_NOSIGNAL};
  };


  template<typename... Args>
  [[nodiscard]]
  decltype(auto) send(Args&&... args)
  noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
  {
    return 
    async_sendmsg<decltype(const_buffer_sequence{std::forward<Args>(args)...}),
     false>
    {*static_cast<F*>(this), 
    std::forward<Args>(args)...};
  }

  template<typename Duration, typename... Args>
  [[nodiscard]]
  decltype(auto) send_timeout_each(Duration&& duration, Args&&... args)
  noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
  {
    return 
    async_sendmsg<decltype(const_buffer_sequence{std::forward<Args>(args)...}),
     true>
    {*static_cast<F*>(this), 
    std::forward<Duration>(duration), 
    std::forward<Args>(args)...};
  }

};

}
#endif //XYNET_SEND_ALL_H
