//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_CLOSE_H
#define XYNET_SOCKET_CLOSE_H

#include <type_traits>
#include <array>
#include "xynet/buffer.h"
#include "xynet/detail/async_operation.h"
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

template<typename Policy, typename F>
struct async_close : public async_operation<Policy, async_close<Policy, F>>
{

  template<typename... Args>
  async_close(F& socket, Args&&... args) noexcept
  : async_operation<Policy, async_close<Policy, F>>
    {&async_close::on_recv_completed, std::forward<Args>(args)...}
  , m_socket{socket}
  {}

  auto initial_check() const noexcept
  {
    return m_socket.valid();
  }

  static void on_recv_completed(async_operation_base *base) noexcept
  {
    auto *op = static_cast<async_close*>(base);
    op->update_result();
  }

  [[nodiscard]]
  auto try_start() noexcept
  {
    return [this](::io_uring_sqe* sqe)
    {
      ::io_uring_prep_recv(sqe, m_socket.get(),
        null_buffer::data(), null_buffer::size(), 0);

      sqe->user_data = reinterpret_cast<uintptr_t>(this);
    };
  }

  void update_result()
  {
    if(async_operation_base::get_error_code())[[unlikely]]
    {
      async_operation_base::get_awaiting_coroutine().resume();
    }
    else if(async_operation_base::get_res() == 0)[[likely]]
    {
      async_operation_base::set_value(::close(m_socket.get()), async_operation_base::get_flags());
      async_operation_base::get_awaiting_coroutine().resume();
    }
    else[[unlikely]]
    {
      async_operation<Policy, async_close<Policy, F>>::submit();
    }
  }

  decltype(auto) get_result()
  noexcept (Policy::error_code_type::value)
  {
    if constexpr (Policy::error_code_type::value)
    {
      return;
    }
    else
    {
      if(async_operation_base::get_error_code())[[unlikely]]
      {
        throw std::system_error{async_operation_base::get_error_code()};
      }
      else[[likely]]
      {
        return;
      }
    }
  }

private:
  F& m_socket;
};

template<typename F, typename... Args>
async_close(F& socket, Args&&... args) noexcept
-> async_close<typename async_operation_traits<std::decay_t<Args>...>::policy_type, F>;

template<typename F>
struct operation_close
{
  template<typename... Args>
  [[nodiscard]]
  decltype(auto) close(Args&&... args) noexcept 
  {
    return async_close{*static_cast<F*>(this), std::forward<Args>(args)...};
  }

};

}

#endif //XYNET_CLOSE_H
