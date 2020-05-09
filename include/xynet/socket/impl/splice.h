//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_SPLICE_H
#define XYNET_SOCKET_SPLICE_H

#include "xynet/detail/async_operation.h"
#include <fcntl.h>

namespace xynet
{

template<detail::FileDescriptorPolicy P, typename F>
struct operation_splice
{
  template<typename F2, bool enable_timeout = false>
  struct async_splice : public async_operation<P, async_splice<F2, enable_timeout>, enable_timeout>
  {

    async_splice(F& socket, F2& file, long int len) noexcept
    requires(enable_timeout == false)
    : async_operation<P, async_splice<F2, enable_timeout>, enable_timeout>{}
    , m_socket{socket}
    , m_file{file}
    , m_len{static_cast<::size_t>(len)}
    {}

    template<typename Duration, bool enable_timeout2 = enable_timeout>
    requires (enable_timeout2 == true)
    async_splice(F& socket, F2& file, long int len, Duration&& duration)
    noexcept
    : async_operation<P, async_splice<F2, enable_timeout>, enable_timeout>{std::forward<Duration>(duration)}
    , m_socket{socket}
    , m_file{file}
    , m_len{static_cast<::size_t>(len)}
    {}

    auto initial_check() const noexcept
    {  
      return true;
    }

    static void on_splice_completed(async_operation_base *base) noexcept
    {
      auto *op = static_cast<async_splice*>(base);
      op->update_result();
    }

    void update_result()
    {
      if(!async_operation_base::no_error_in_result() || async_operation_base::get_res() == 0)
      {
        async_operation_base::get_awaiting_coroutine().resume();
      }
      else
      {
        m_bytes_transferred += async_operation_base::get_res();
        async_operation<P, async_splice<F2, enable_timeout>, enable_timeout>::submit();
      }
    }

    auto try_start() noexcept
    {
      return [this](::io_uring_sqe *sqe)
      {
        ::io_uring_prep_splice
        (sqe, 
        m_file.get(),   -1,
        m_socket.get(), -1,
        m_len, m_splice_flags);

        sqe->user_data = reinterpret_cast<uintptr_t>(this);
      };
    }

    auto get_result()
    noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
    -> detail::file_descriptor_operation_return_type_t<P, std::size_t>
    {
      return async_throw_or_return<P, std::size_t>
        (async_operation_base::get_error_code(), m_bytes_transferred);
    }
  private:
    F&  m_socket;
    F2& m_file;
    std::size_t m_bytes_transferred;
    ::size_t m_len;
    unsigned int m_splice_flags = 0;
  };


  template<typename F2, typename... Args>
  [[nodiscard]]
  decltype(auto) splice(F2&& file, Args&&... args) noexcept 
  {
    return async_splice<F2, false>{*static_cast<F*>(this), 
    std::forward<F2>(file), 
    std::forward<Args>(args)...};
  }
};

}

#endif //XYNET_ACCEPT_H
