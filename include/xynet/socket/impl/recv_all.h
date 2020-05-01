//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_READ_H
#define XYNET_SOCKET_READ_H

#include <type_traits>
#include "xynet/buffer.h"
#include "xynet/socket/detail/async_operation.h"
#include "xynet/detail/file_descriptor_traits.h"

namespace xynet
{

template<detail::FileDescriptorPolicy P, typename F>
struct operation_recv
{
  template<typename BufferSequence, bool enable_timeout_each>
  struct async_recvmsg 
  : public async_operation<P, 
  async_recvmsg<BufferSequence, enable_timeout_each>, enable_timeout_each>
  {
    template<typename... Args>
    async_recvmsg(F& socket, Args&&... args) noexcept
    requires (enable_timeout_each == false)
      :async_operation<P, async_recvmsg, enable_timeout_each>{&async_recvmsg::on_recv_completed}
      ,m_socket{socket}
      ,m_buffers{static_cast<Args&&>(args)...}
      ,m_bytes_transferred{}
      ,m_msghdr{}
    {
      std::tie(m_msghdr.msg_iov,
               m_msghdr.msg_iovlen) = m_buffers.get_iov_span();
    }

    template<
    typename Duration, 
    bool enable_timeout_each2 = enable_timeout_each,
    typename... Args>
    async_recvmsg(F& socket, Duration&& duration, Args&&... args) noexcept
    requires (enable_timeout_each == true)
    :async_operation<P, async_recvmsg, enable_timeout_each>
    {
      &async_recvmsg::on_recv_completed,
      std::forward<Duration>(duration)
    }
    ,m_socket{socket}
    ,m_buffers{static_cast<Args&&>(args)...}
    ,m_bytes_transferred{}
    ,m_msghdr{}
    {
      std::tie(m_msghdr.msg_iov,
               m_msghdr.msg_iovlen) = m_buffers.get_iov_span();
    }

    auto initial_check() const noexcept
    {
      return true;
    }


    static void on_recv_completed(async_operation_base *base) noexcept
    {
      auto *op = static_cast<async_recvmsg*>(base);
      op->update_result();
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
      if
      (
      !  async_operation_base::no_error_in_result()
      || async_operation_base::get_res() == 0
      )
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
          async_operation<P, async_recvmsg<BufferSequence, enable_timeout_each>, 
          enable_timeout_each>::submit();
        }
      }
    }

    auto get_result()
    noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
    ->detail::file_descriptor_operation_return_type_t<P, int>
    {
      if(async_operation_base::get_res() == 0)
      {
        return async_throw_or_return<P>(
          xynet_error_instance::make_error_code(xynet_error::eof),
          m_bytes_transferred);
      }
      return async_throw_or_return<P>(async_operation_base::get_error_code(), m_bytes_transferred);
    }

  private:
    F& m_socket;
    BufferSequence m_buffers;
    int m_bytes_transferred;
    ::msghdr m_msghdr;
  };

  template<typename BufferSequence, bool enable_timeout>
  struct async_recvmsg_some
  : public async_operation<P, 
  async_recvmsg_some<BufferSequence, enable_timeout>, enable_timeout>
  {
    template<typename... Args>
    async_recvmsg_some(F& socket, Args&&... args) noexcept
    requires (enable_timeout == false)
      :async_operation<P, async_recvmsg_some, enable_timeout>{}
      ,m_socket{socket}
      ,m_buffers{static_cast<Args&&>(args)...}
      ,m_msghdr{}
    {
      std::tie(m_msghdr.msg_iov,
               m_msghdr.msg_iovlen) = m_buffers.get_iov_span();
    }

    template<
    typename Duration, 
    bool enable_timeout2 = enable_timeout,
    typename... Args>
    async_recvmsg_some(F& socket, Duration&& duration, Args&&... args) noexcept
    requires (enable_timeout == true)
    :async_operation<P, async_recvmsg_some, enable_timeout>{std::forward<Duration>(duration)}
    ,m_socket{socket}
    ,m_buffers{static_cast<Args&&>(args)...}
    ,m_msghdr{}
    {
      std::tie(m_msghdr.msg_iov,
               m_msghdr.msg_iovlen) = m_buffers.get_iov_span();
    }

    auto initial_check() const noexcept
    {
      return true;
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

    auto get_result()
    noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
    ->detail::file_descriptor_operation_return_type_t<P, int>
    {
      if(async_operation_base::get_res() == 0)
      {
        return async_throw_or_return<P>(
          xynet_error_instance::make_error_code(xynet_error::eof), 0);
      }
      return async_throw_or_return<P>(async_operation_base::get_error_code()
      , async_operation_base::get_res());
    }

  private:
    F& m_socket;
    BufferSequence m_buffers;
    ::msghdr m_msghdr;
  };


  template<typename... Args>
  [[nodiscard]]
  decltype(auto) recv(Args&&... args)
  noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
  {
    return async_recvmsg<decltype(buffer_sequence{std::forward<Args>(args)...}), false>
    {*static_cast<F*>(this), std::forward<Args>(args)...};
  }

  template<typename Duration, typename... Args>
  [[nodiscard]]
  decltype(auto) recv_timeout_each(Duration&& duration, Args&&... args)
  {
    return async_recvmsg<decltype(buffer_sequence{std::forward<Args>(args)...}), true>
    {*static_cast<F*>(this), std::forward<Duration>(duration), std::forward<Args>(args)...};
  }

  template<typename... Args>
  [[nodiscard]]
  decltype(auto) recv_some(Args&&... args)
  noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
  {
    return async_recvmsg_some<decltype(buffer_sequence{std::forward<Args>(args)...}), false>
    {*static_cast<F*>(this), std::forward<Args>(args)...};
  }

  template<typename Duration, typename... Args>
  [[nodiscard]]
  decltype(auto) recv_some_timeout(Duration&& duration, Args&&... args)
  {
    return async_recvmsg_some<decltype(buffer_sequence{std::forward<Args>(args)...}), true>
    {*static_cast<F*>(this), std::forward<Duration>(duration), std::forward<Args>(args)...};
  }

};

}

#endif //XYNET_READ_H
