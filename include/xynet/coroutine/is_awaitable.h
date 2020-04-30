//
// Created by xuanyi on 4/22/20.
//

#ifndef XYNET_COROUTINE_IS_AWAITABLE_H
#define XYNET_COROUTINE_IS_AWAITABLE_H

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

#include <xynet/coroutine/detail/get_awaiter.h>

#include <type_traits>

namespace xynet
{
template<typename T, typename = std::void_t<>>
struct is_awaitable : std::false_type {};

template<typename T>
struct is_awaitable<T, std::void_t<decltype(xynet::detail::get_awaiter(std::declval<T>()))>>
  : std::true_type
{};

template<typename T>
constexpr bool is_awaitable_v = is_awaitable<T>::value;
}

#endif //XYNET_IS_AWAITABLE_H
