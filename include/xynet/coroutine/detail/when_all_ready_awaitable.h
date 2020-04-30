//
// Created by xuanyi on 4/22/20.
//

#ifndef XYNET_WHEN_ALL_READY_AWAITABLE_H
#define XYNET_WHEN_ALL_READY_AWAITABLE_H

/*
 * Copyright 2017 Lewis Baker
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "xynet/coroutine/detail/when_all_counter.h"

#include <coroutine>
#include <tuple>

namespace xynet
{
namespace detail
{
template<typename TASK_CONTAINER>
class when_all_ready_awaitable;

template<>
class when_all_ready_awaitable<std::tuple<>>
{
public:

  constexpr when_all_ready_awaitable() noexcept {}
  explicit constexpr when_all_ready_awaitable(std::tuple<>) noexcept {}

  constexpr bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
  std::tuple<> await_resume() const noexcept { return {}; }

};

template<typename... TASKS>
class when_all_ready_awaitable<std::tuple<TASKS...>>
{
public:

  explicit when_all_ready_awaitable(TASKS&&... tasks)
  noexcept(std::conjunction_v<std::is_nothrow_move_constructible<TASKS>...>)
    : m_counter(sizeof...(TASKS))
    , m_tasks(std::move(tasks)...)
  {}

  explicit when_all_ready_awaitable(std::tuple<TASKS...>&& tasks)
  noexcept(std::is_nothrow_move_constructible_v<std::tuple<TASKS...>>)
    : m_counter(sizeof...(TASKS))
    , m_tasks(std::move(tasks))
  {}

  when_all_ready_awaitable(when_all_ready_awaitable&& other) noexcept
    : m_counter(sizeof...(TASKS))
    , m_tasks(std::move(other.m_tasks))
  {}

  auto operator co_await() & noexcept
  {
    struct awaiter
    {
      awaiter(when_all_ready_awaitable& awaitable) noexcept
        : m_awaitable(awaitable)
      {}

      bool await_ready() const noexcept
      {
        return m_awaitable.is_ready();
      }

      bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
      {
        return m_awaitable.try_await(awaitingCoroutine);
      }

      std::tuple<TASKS...>& await_resume() noexcept
      {
        return m_awaitable.m_tasks;
      }

    private:

      when_all_ready_awaitable& m_awaitable;

    };

    return awaiter{ *this };
  }

  auto operator co_await() && noexcept
  {
    struct awaiter
    {
      awaiter(when_all_ready_awaitable& awaitable) noexcept
        : m_awaitable(awaitable)
      {}

      bool await_ready() const noexcept
      {
        return m_awaitable.is_ready();
      }

      bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
      {
        return m_awaitable.try_await(awaitingCoroutine);
      }

      std::tuple<TASKS...>&& await_resume() noexcept
      {
        return std::move(m_awaitable.m_tasks);
      }

    private:

      when_all_ready_awaitable& m_awaitable;

    };

    return awaiter{ *this };
  }

private:

  bool is_ready() const noexcept
  {
    return m_counter.is_ready();
  }

  bool try_await(std::coroutine_handle<> awaitingCoroutine) noexcept
  {
    start_tasks(std::make_integer_sequence<std::size_t, sizeof...(TASKS)>{});
    return m_counter.try_await(awaitingCoroutine);
  }

  template<std::size_t... INDICES>
  void start_tasks(std::integer_sequence<std::size_t, INDICES...>) noexcept
  {
    (void)std::initializer_list<int>{
      (std::get<INDICES>(m_tasks).start(m_counter), 0)...
    };
  }

  when_all_counter m_counter;
  std::tuple<TASKS...> m_tasks;

};

template<typename TASK_CONTAINER>
class when_all_ready_awaitable
{
public:

  explicit when_all_ready_awaitable(TASK_CONTAINER&& tasks) noexcept
    : m_counter(tasks.size())
    , m_tasks(std::forward<TASK_CONTAINER>(tasks))
  {}

  when_all_ready_awaitable(when_all_ready_awaitable&& other)
  noexcept(std::is_nothrow_move_constructible_v<TASK_CONTAINER>)
    : m_counter(other.m_tasks.size())
    , m_tasks(std::move(other.m_tasks))
  {}

  when_all_ready_awaitable(const when_all_ready_awaitable&) = delete;
  when_all_ready_awaitable& operator=(const when_all_ready_awaitable&) = delete;

  auto operator co_await() & noexcept
  {
    class awaiter
    {
    public:

      awaiter(when_all_ready_awaitable& awaitable)
        : m_awaitable(awaitable)
      {}

      bool await_ready() const noexcept
      {
        return m_awaitable.is_ready();
      }

      bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
      {
        return m_awaitable.try_await(awaitingCoroutine);
      }

      TASK_CONTAINER& await_resume() noexcept
      {
        return m_awaitable.m_tasks;
      }

    private:

      when_all_ready_awaitable& m_awaitable;

    };

    return awaiter{ *this };
  }


  auto operator co_await() && noexcept
  {
    class awaiter
    {
    public:

      awaiter(when_all_ready_awaitable& awaitable)
        : m_awaitable(awaitable)
      {}

      bool await_ready() const noexcept
      {
        return m_awaitable.is_ready();
      }

      bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
      {
        return m_awaitable.try_await(awaitingCoroutine);
      }

      TASK_CONTAINER&& await_resume() noexcept
      {
        return std::move(m_awaitable.m_tasks);
      }

    private:

      when_all_ready_awaitable& m_awaitable;

    };

    return awaiter{ *this };
  }

private:

  bool is_ready() const noexcept
  {
    return m_counter.is_ready();
  }

  bool try_await(std::coroutine_handle<> awaitingCoroutine) noexcept
  {
    for (auto&& task : m_tasks)
    {
      task.start(m_counter);
    }

    return m_counter.try_await(awaitingCoroutine);
  }

  when_all_counter m_counter;
  TASK_CONTAINER m_tasks;

};
}
}

#endif //XYNET_WHEN_ALL_READY_AWAITABLE_H
