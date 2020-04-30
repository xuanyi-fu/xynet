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

#ifndef XYNET_COROUTINE_SYNC_WAIT_H
#define XYNET_COROUTINE_SYNC_WAIT_H

#include "xynet/coroutine/detail/lightweight_manual_reset_event.h"
#include "xynet/coroutine/detail/sync_wait_task.h"
#include "xynet/coroutine/awaitable_traits.h"

#include <cstdint>
#include <atomic>

namespace xynet
{
template<typename AWAITABLE>
auto sync_wait(AWAITABLE&& awaitable)
-> typename xynet::awaitable_traits<AWAITABLE&&>::await_result_t
{
#if CPPCORO_COMPILER_MSVC
  // HACK: Need to explicitly specify template argument to make_sync_wait_task
		// here to work around a bug in MSVC when passing parameters by universal
		// reference to a coroutine which causes the compiler to think it needs to
		// 'move' parameters passed by rvalue reference.
		auto task = detail::make_sync_wait_task<AWAITABLE>(awaitable);
#else
  auto task = detail::make_sync_wait_task(std::forward<AWAITABLE>(awaitable));
#endif
  detail::lightweight_manual_reset_event event;
  task.start(event);
  event.wait();
  return task.result();
}
}

#endif //XYNET_SYNC_WAIT_H
