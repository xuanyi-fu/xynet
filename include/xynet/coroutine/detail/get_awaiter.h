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

#ifndef XYNET_COROUTINE_DETAIL_GET_AWAITER_H
#define XYNET_COROUTINE_DETAIL_GET_AWAITER_H

#include "xynet/coroutine/detail/is_awaiter.h"
#include "xynet/coroutine/detail/any.h"

namespace xynet
{
namespace detail
{
template<typename T>
auto get_awaiter_impl(T&& value, int)
noexcept(noexcept(static_cast<T&&>(value).operator co_await()))
-> decltype(static_cast<T&&>(value).operator co_await())
{
  return static_cast<T&&>(value).operator co_await();
}

template<typename T>
auto get_awaiter_impl(T&& value, long)
noexcept(noexcept(operator co_await(static_cast<T&&>(value))))
-> decltype(operator co_await(static_cast<T&&>(value)))
{
  return operator co_await(static_cast<T&&>(value));
}

template<
  typename T,
  std::enable_if_t<xynet::detail::is_awaiter<T&&>::value, int> = 0>
T&& get_awaiter_impl(T&& value, xynet::detail::any) noexcept
{
  return static_cast<T&&>(value);
}

template<typename T>
auto get_awaiter(T&& value)
noexcept(noexcept(detail::get_awaiter_impl(static_cast<T&&>(value), 123)))
-> decltype(detail::get_awaiter_impl(static_cast<T&&>(value), 123))
{
  return detail::get_awaiter_impl(static_cast<T&&>(value), 123);
}
}
}

#endif //XYNET_GET_AWAITER_H
