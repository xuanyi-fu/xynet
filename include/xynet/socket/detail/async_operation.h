//
// Created by xuanyi on 4/28/20.
//

#ifndef XYNET_SOCKET_ASYNC_OPERATION_H
#define XYNET_SOCKET_ASYNC_OPERATION_H


#include <linux/time_types.h>

#include <chrono>

#include "xynet/async_operation_base.h"
#include "xynet/io_service.h"

namespace xynet
{

template<bool enable_timeout>
struct timeout_storage
{
  template<typename Rep, typename Period>
  timeout_storage(std::chrono::duration<Rep, Period> duration)
  {
    auto duration_ns = 
    std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
    m_timespec.tv_sec  = duration_ns.count() / 1'000'000'000;
    m_timespec.tv_nsec = duration_ns.count() % 1'000'000'000;  
  }

  auto get_timespec_ptr() -> ::__kernel_timespec*
  {
    return &m_timespec;
  }

  ::__kernel_timespec m_timespec = ::__kernel_timespec{};
};

template<>
struct timeout_storage<false>
{
  constexpr auto get_timespec_ptr() -> ::__kernel_timespec*
  {
    return nullptr;
  }
};


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
  [[no_unique_address]] timeout_storage<enable_timeout> m_timeout;
};

}

#endif //XYNET_ASYNC_OPERATION_H
