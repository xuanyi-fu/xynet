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


#include "doctest/doctest.h"

#include "xynet/coroutine/task.h"
#include "xynet/coroutine/sync_wait.h"
#include "xynet/coroutine/when_all.h"

TEST_CASE("task doesn't start until awaited")
{
  bool started = false;
  auto func = [&]() -> xynet::task<>
  {
    started = true;
    co_return;
  };

  xynet::sync_wait([&]() -> xynet::task<>
                     {
                       auto t = func();
                         CHECK(!started);

                       co_await t;

                         CHECK(started);
                     }());
}

TEST_CASE("awaiting default-constructed task throws broken_promise")
{
  xynet::sync_wait([&]() -> xynet::task<>
                     {
                       xynet::task<> t;
                         CHECK_THROWS_AS(co_await t, const xynet::broken_promise&);
                     }());
}

TEST_CASE("lots of synchronous completions doesn't result in stack-overflow")
{
  auto completesSynchronously = []() -> xynet::task<int>
  {
    co_return 1;
  };

  auto run = [&]() -> xynet::task<>
  {
    int sum = 0;
    for (int i = 0; i < 1'000'000; ++i)
    {
      sum = sum + co_await completesSynchronously();
    }
  };

  xynet::sync_wait(run());
}