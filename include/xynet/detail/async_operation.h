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

namespace detail
{

template<bool enable_error_code>
struct error_code_storage;

template<>
struct error_code_storage<false>
{
  std::error_code m_error_code = std::error_code{};

  auto ptr() noexcept
  {
    return &m_error_code;
  }
};

template<>
struct error_code_storage<true>
{
  constexpr auto ptr() noexcept -> std::error_code*
  {
    return nullptr;
  }
};

}


template<bool enable_timeout, bool enable_error_code>
struct async_operation_policy
{
  using timeout_type    = std::conditional_t<enable_timeout, std::true_type, std::false_type>;
  using error_code_type = std::conditional_t<enable_error_code, std::true_type, std::false_type>; 
};

template<typename... Ps>
struct async_operation_traits;


template<typename P1, typename P2>
struct async_operation_traits<P1, P2>
{
  using policy_type = async_operation_policy<true, true>;
};

template<>
struct async_operation_traits<>
{
  using policy_type = async_operation_policy<false, false>;
};

template<typename Rep, typename Period>
struct async_operation_traits<std::chrono::duration<Rep, Period>>
{
  using policy_type = async_operation_policy<true, false>;
};

template<>
struct async_operation_traits<std::error_code>
{
  using policy_type = async_operation_policy<false, true>;
};

template<typename Duration>
struct is_duration
  : public std::false_type{};

template<typename Rep, typename Period>
struct is_duration<std::chrono::duration<Rep, Period>> 
  : public std::true_type{};

template<typename T>
inline constexpr bool is_duration_v = is_duration<T>::value;

template<typename T>
concept DurationType = is_duration_v<T>;



template<typename Policy, typename T>
class async_operation : public async_operation_base
{
public:

  async_operation() noexcept
  :async_operation_base{xynet::io_service::get_thread_io_service()}
  ,m_timeout{}
  ,m_error_code{}
  {
    async_operation_base::set_error_ptr(m_error_code.ptr());
  }

  async_operation(std::error_code& error) noexcept
  :async_operation_base{xynet::io_service::get_thread_io_service()}
  ,m_timeout{}
  ,m_error_code{}
  {
    async_operation_base::set_error_ptr(&error);
  }

  template<DurationType Duration>
  async_operation(Duration&& duration) noexcept
  :async_operation_base{xynet::io_service::get_thread_io_service()}
  ,m_timeout{std::forward<Duration&&>(duration)}
  ,m_error_code{}
  {
    async_operation_base::set_error_ptr(m_error_code.ptr());
  }

  template<DurationType Duration>
  async_operation(Duration&& duration, std::error_code& error)
  :async_operation_base{xynet::io_service::get_thread_io_service()}
  ,m_timeout{std::forward<Duration&&>(duration)}
  ,m_error_code{}
  {
    async_operation_base::set_error_ptr(&error);
  }

  async_operation(async_operation_base::callback_t callback) noexcept
  :async_operation_base{xynet::io_service::get_thread_io_service(), callback}
  ,m_timeout{}
  ,m_error_code{}
  {
    async_operation_base::set_error_ptr(m_error_code.ptr());
  }

  async_operation(async_operation_base::callback_t callback, std::error_code& error) noexcept
  :async_operation_base{xynet::io_service::get_thread_io_service(), callback}
  ,m_timeout{}
  ,m_error_code{}
  {
    async_operation_base::set_error_ptr(&error);
  }

  template<DurationType Duration>
  async_operation(async_operation_base::callback_t callback, Duration&& duration) noexcept
  :async_operation_base{xynet::io_service::get_thread_io_service(), callback}
  ,m_timeout{std::forward<Duration&&>(duration)}
  ,m_error_code{}
  {
    async_operation_base::set_error_ptr(m_error_code.ptr());
  }

  template<DurationType Duration>
  async_operation(async_operation_base::callback_t callback, Duration&& duration, std::error_code& error)
  :async_operation_base{xynet::io_service::get_thread_io_service(), callback}
  ,m_timeout{std::forward<Duration&&>(duration)}
  ,m_error_code{}
  {
    async_operation_base::set_error_ptr(&error);
  }

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
    if constexpr (!Policy::timeout_type::value)
    {
      async_operation_base::get_service()->try_submit_io(static_cast<T *>(this)->try_start());
    }
    else
    {
      static_assert(Policy::timeout_type::value);
      async_operation_base::get_service()->try_submit_io(static_cast<T *>(this)->try_start(), m_timeout.get_timespec_ptr());
    }
  }

  decltype(auto) await_resume()
  noexcept (Policy::error_code_type::value)
  {
    return static_cast<T *>(this)->get_result();
  }
private:
  [[no_unique_address]] 
  detail::timeout_storage<Policy::timeout_type::value>       m_timeout;
  [[no_unique_address]] 
  detail::error_code_storage<Policy::error_code_type::value> m_error_code;
};

}

#endif //XYNET_ASYNC_OPERATION_H
