//
// Created by xuanyi on 5/5/20.
//
#ifndef XYNET_TIMEOUT_STORAGE_H
#define XYNET_TIMEOUT_STORAGE_H
#include <linux/time_types.h>

namespace xynet::detail
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

  auto is_zero_timeout() -> bool
  {
    return m_timespec.tv_sec == 0 && m_timespec.tv_nsec == 0;
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

}

#endif
