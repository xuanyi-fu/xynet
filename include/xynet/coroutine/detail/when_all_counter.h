//
// Created by xuanyi on 4/22/20.
//

#ifndef XYNET_WHEN_ALL_COUNTER_H
#define XYNET_WHEN_ALL_COUNTER_H

///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_DETAIL_WHEN_ALL_COUNTER_HPP_INCLUDED
#define CPPCORO_DETAIL_WHEN_ALL_COUNTER_HPP_INCLUDED

#include <coroutine>
#include <atomic>
#include <cstdint>

namespace xynet
{
namespace detail
{
class when_all_counter
{
public:

  when_all_counter(std::size_t count) noexcept
    : m_count(count + 1)
    , m_awaitingCoroutine(nullptr)
  {}

  bool is_ready() const noexcept
  {
    // We consider this complete if we're asking whether it's ready
    // after a coroutine has already been registered.
    return static_cast<bool>(m_awaitingCoroutine);
  }

  bool try_await(std::coroutine_handle<> awaitingCoroutine) noexcept
  {
    m_awaitingCoroutine = awaitingCoroutine;
    return m_count.fetch_sub(1, std::memory_order_acq_rel) > 1;
  }

  void notify_awaitable_completed() noexcept
  {
    if (m_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
      m_awaitingCoroutine.resume();
    }
  }

protected:

  std::atomic<std::size_t> m_count;
  std::coroutine_handle<> m_awaitingCoroutine;

};
}
}

#endif


#endif //XYNET_WHEN_ALL_COUNTER_H
