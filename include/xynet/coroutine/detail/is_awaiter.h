//
// Created by xuanyi on 4/22/20.
//

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

#ifndef XYNET_COROUTINE_DETAIL_IS_AWAITER_H
#define XYNET_COROUTINE_DETAIL_IS_AWAITER_H

#include <type_traits>
#include <coroutine>

namespace xynet
{
namespace detail
{
template<typename T>
struct is_coroutine_handle
  : std::false_type
{};

template<typename PROMISE>
struct is_coroutine_handle<std::coroutine_handle<PROMISE>>
  : std::true_type
{};

// NOTE: We're accepting a return value of coroutine_handle<P> here
// which is an extension supported by Clang which is not yet part of
// the C++ coroutines TS.
template<typename T>
struct is_valid_await_suspend_return_value : std::disjunction<
  std::is_void<T>,
  std::is_same<T, bool>,
  is_coroutine_handle<T>>
{};

template<typename T, typename = std::void_t<>>
struct is_awaiter : std::false_type {};

// NOTE: We're testing whether await_suspend() will be callable using an
// arbitrary coroutine_handle here by checking if it supports being passed
// a coroutine_handle<void>. This may result in a false-result for some
// types which are only awaitable within a certain context.
template<typename T>
struct is_awaiter<T, std::void_t<
  decltype(std::declval<T>().await_ready()),
  decltype(std::declval<T>().await_suspend(std::declval<std::coroutine_handle<>>())),
  decltype(std::declval<T>().await_resume())>> :
  std::conjunction<
    std::is_constructible<bool, decltype(std::declval<T>().await_ready())>,
    detail::is_valid_await_suspend_return_value<
      decltype(std::declval<T>().await_suspend(std::declval<std::coroutine_handle<>>()))>>
{};
}
}

#endif //XYNET_IS_AWAITER_H
