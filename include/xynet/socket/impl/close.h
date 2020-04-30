//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_CLOSE_H
#define XYNET_SOCKET_CLOSE_H

#include <type_traits>
#include <array>
#include "xynet/buffer.h"
#include "xynet/socket/detail/async_operation.h"
#include "xynet/detail/file_descriptor_traits.h"

namespace xynet
{

struct null_buffer
{
  constexpr inline static std::size_t buffer_size = 1024;
  inline static std::array<std::byte, buffer_size> buffer = std::array<std::byte, 1024>{};
  static void * data() noexcept
  {
    return static_cast<void *>(buffer.data());
  }

  static int size() noexcept
  {
    return static_cast<int>(buffer.size());
  }
};

template<detail::FileDescriptorPolicy P, typename F>
struct operation_close
{
  struct async_close_impl : public async_operation<P, async_close_impl>
  {
    async_close_impl(F& socket) noexcept
      :async_operation<P, async_close_impl>{&async_close_impl::on_recv_completed}
      ,m_socket{socket}
    {}

    static void on_recv_completed(async_operation_base *base) noexcept
    {
      auto *op = static_cast<async_close_impl*>(base);
      op->update_result();
    }

    void try_start()
    noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
    {
      auto recv = [this](::io_uring_sqe* sqe)
      {
        ::io_uring_prep_recv(sqe, m_socket.get(),
          null_buffer::data(), null_buffer::size(), 0);

        sqe->user_data = reinterpret_cast<uintptr_t>(this);
      };
      async_operation_base::get_service()->try_submit_io(recv);
    }

    void update_result()
    {
      if(!async_operation_base::no_error_in_result())
      {
        async_operation_base::get_awaiting_coroutine().resume();
      }
      else if(async_operation_base::get_res() == 0)
      {
        async_operation_base::set_res(::close(m_socket.get()));
        async_operation_base::get_awaiting_coroutine().resume();
      }
      else
      {
        try_start();
      }
    }

    auto get_result()
    noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
    ->detail::file_descriptor_operation_return_type_t<P>
    {
      return async_throw_or_return<P>(async_operation_base::get_error_code());
    }

  private:
    F& m_socket;
  };


  [[nodiscard]]
  decltype(auto) async_close()
  noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
  {
    return async_close_impl{*static_cast<F*>(this)};
  }

};

}

#endif //XYNET_CLOSE_H
