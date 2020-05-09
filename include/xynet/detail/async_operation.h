//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_ASYNC_OPERATION_H
#define XYNET_SOCKET_ASYNC_OPERATION_H

#include <chrono>
#include "xynet/detail/timeout_storage.h"
#include "xynet/detail/file_descriptor_traits.h"
#include "xynet/async_operation_base.h"
#include "xynet/io_service.h"

namespace xynet
{

template<
detail::FileDescriptorPolicy P, 
typename T, 
bool enable_timeout>
class async_operation : public async_operation_base
{
public:
  async_operation() noexcept
  :async_operation_base{xynet::io_service::get_thread_io_service()}
  {}

  explicit async_operation(async_operation_base::callback_t callback) noexcept
  :async_operation_base{xynet::io_service::get_thread_io_service(), callback}
  {}

  template<typename Duration, bool enable_timeout2 = enable_timeout>
  requires (enable_timeout2 == true)
  explicit async_operation(Duration&& duration) noexcept
  :async_operation_base{xynet::io_service::get_thread_io_service()}
  ,m_timeout{std::forward<Duration&&>(duration)}
  {}

  template<typename Duration, bool enable_timeout2 = enable_timeout>
  requires (enable_timeout2 == true)
  async_operation(async_operation_base::callback_t callback, Duration&& duration)
  :async_operation_base{xynet::io_service::get_thread_io_service(), callback}
  ,m_timeout{std::forward<Duration&&>(duration)}
  {}

  bool await_ready() noexcept
  {
    return (get_service() == nullptr)
    && !(static_cast<T*>(this)->initial_check());
  };

  void await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept
  {
    set_awaiting_coroutine(awaiting_coroutine);
    submit();
  }

  void submit() noexcept
  {
    if constexpr (!enable_timeout)
    {
      async_operation_base::get_service()->try_submit_io(static_cast<T *>(this)->try_start());
    }
    else
    {
      static_assert(enable_timeout);
      async_operation_base::get_service()->try_submit_io(static_cast<T *>(this)->try_start(), m_timeout.get_timespec_ptr());
    }
  }

  decltype(auto) await_resume()
  noexcept (detail::FileDescriptorPolicyUseErrorCode<P>)
  {
    return static_cast<T *>(this)->get_result();
  }
private:
  [[no_unique_address]] detail::timeout_storage<enable_timeout> m_timeout;
};

}

#endif //XYNET_ASYNC_OPERATION_H
